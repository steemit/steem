#!/bin/bash

VERSION=`cat /etc/steemdversion`

# if the writer node dies by itself, kill runsv causing the container to exit
STEEMD_PID=`pgrep -f p2p-endpoint`
if [[ ! $? -eq 0 ]]; then
  echo NOTIFYALERT! steemdsync has quit unexpectedly, checking for coredump and then starting a new instance..
  sleep 30
  SAVED_PID=`cat /tmp/steemdpid`
  if [[ -e /tmp/core.$SAVED_PID ]]; then
    gdb --batch --quiet -ex "thread apply all bt full" -ex "quit" /usr/local/steemd-full/bin/steemd /tmp/core.$SAVED_PID >> /tmp/stacktrace
    STACKTRACE=`cat /tmp/stacktrace`
    echo NOTIFYALERT! steemdsync stacktrace from coredump:
    for ((i=0;i<${#STACKTRACE};i+=120)); do
      echo "${STACKTRACE:i:120}"
    done
    CORE_FILE_NAME=coredump-`date '+%Y%m%d-%H%M%S'`.$SAVED_PID
    aws s3 cp /tmp/core.$SAVED_PID s3://$S3_BUCKET/$CORE_FILE_NAME
  fi
  RUN_SV_PID=`pgrep -f /etc/service/steemd`
  kill -9 $RUN_SV_PID
fi

# check if we are synced, if so start the process of uploading to s3
# after uploading, kill runsv causing the container to exit
# and ecs-agent will start a new container starting the process over
BLOCKCHAIN_TIME=$(
    curl --silent --max-time 20 \
        --data '{"jsonrpc":"2.0","id":39,"method":"database_api.get_dynamic_global_properties"}' \
        localhost:8090 | jq -r .result.time
)
echo "[info] BLOCKCHAIN_TIME=$BLOCKCHAIN_TIME"

if [[ "$IS_BROADCAST_NODE" ]]; then
  FILE_TYPE=broadcast
elif [[ "$IS_AH_NODE" ]]; then
  FILE_TYPE=ahnode
else
  FILE_TYPE=blockchain
fi
CHECKSUM_BLOCKCHAIN_TAR_FILE="checksum-tar-latest"

if [[ ! -z "$BLOCKCHAIN_TIME" ]]; then
  BLOCKCHAIN_SECS=`date -d $BLOCKCHAIN_TIME +%s`
  CURRENT_SECS=`date +%s`

  # if we're within 10 seconds of current time, call it synced and begin the upload
  BLOCK_AGE=$((${CURRENT_SECS} - ${BLOCKCHAIN_SECS}))
  echo "[info] BLOCK_AGE=$BLOCK_AGE"
  if [[ ${BLOCK_AGE} -le 10 ]]; then
    LAST_BACKUP_TIME=`aws s3 ls s3://$S3_BUCKET/${FILE_TYPE}-${CHECKSUM_BLOCKCHAIN_TAR_FILE} | awk '{print $1}'`
    echo "[info] LAST_BACKUP_TIME=$LAST_BACKUP_TIME"
    if [[ ! -z $LAST_BACKUP_TIME ]]; then
      LAST_BACKUP_TIMESTAMP=`date -d $LAST_BACKUP_TIME +%s`
      BACKUP_INTERVAL=$((${CURRENT_SECS} - ${LAST_BACKUP_TIMESTAMP}))
      # backup each 24 hours
      if [[ ${BACKUP_INTERVAL} -ge 86400 ]]; then
        STEEMD_PID=`pgrep -f p2p-endpoint`
        kill -SIGINT $STEEMD_PID
        echo steemdsync: waiting for steemd to exit cleanly

        # loop while the process is still running
        let WAIT_TIME=0
        while kill -0 $STEEMD_PID 2> /dev/null; do
           sleep 1
           let WAIT_TIME++

           # if we haven't stopped the process in 15 minutes break from the loop and emit a warning
           if [[ WAIT_TIME -gt 900 ]]; then
              echo warning: waited $WAIT_TIME seconds and process is still running
              break;
           fi
        done

        echo steemdsync: starting a new blockchainstate upload operation
        cd ${COMPRESSPATH:-$HOME}
        echo steemdsync: compressing blockchainstate...
        if [[ "$USE_RAMDISK" ]]; then
          tar vcf blockchain.tar.lz4 --use-compress-prog=lz4 -C $HOME blockchain -C /mnt/ramdisk blockchain
        else
          tar cf blockchain.tar.lz4 --use-compress-prog=lz4 -C $HOME blockchain
        fi
        CHECKSUM_BLOCKCHAIN_TAR=`sha256sum blockchain.tar.lz4 | awk '{print $1}'`
        echo The tar file checksum is: $CHECKSUM_BLOCKCHAIN_TAR
        echo $CHECKSUM_BLOCKCHAIN_TAR > $CHECKSUM_BLOCKCHAIN_TAR_FILE
        if [[ ! $? -eq 0 ]]; then
          echo NOTIFYALERT! steemdsync was unable to compress shared memory file, check the logs.
          exit 1
        fi
        if [[ "$IS_BROADCAST_NODE" ]]; then
          FILE_NAME=broadcast-$VERSION-`date '+%Y%m%d-%H%M%S'`-$CHECKSUM_BLOCKCHAIN_TAR.tar.lz4
        elif [[ "$IS_AH_NODE" ]]; then
          FILE_NAME=ahnode-$VERSION-`date '+%Y%m%d-%H%M%S'`-$CHECKSUM_BLOCKCHAIN_TAR.tar.lz4
        else
          FILE_NAME=blockchain-$VERSION-`date '+%Y%m%d-%H%M%S'`-$CHECKSUM_BLOCKCHAIN_TAR.tar.lz4
        fi
        echo steemdsync: uploading $FILE_NAME to $S3_BUCKET
        aws s3 cp blockchain.tar.lz4 s3://$S3_BUCKET/$FILE_NAME
        if [[ ! $? -eq 0 ]]; then
          echo NOTIFYALERT! steemdsync was unable to upload $FILE_NAME to s3://$S3_BUCKET
          exit 1
        fi
        echo steemdsync: replacing current version of blockchain state with $FILE_NAME
        if [[ "$IS_BROADCAST_NODE" ]]; then
          aws s3 cp s3://$S3_BUCKET/$FILE_NAME s3://$S3_BUCKET/broadcast-latest.tar.lz4
          aws s3api put-object-acl --bucket $S3_BUCKET --key broadcast-latest.tar.lz4 --acl public-read
        elif [[ "$IS_AH_NODE" ]]; then
          aws s3 cp s3://$S3_BUCKET/$FILE_NAME s3://$S3_BUCKET/ahnode-latest.tar.lz4
          aws s3api put-object-acl --bucket $S3_BUCKET --key ahnode-latest.tar.lz4 --acl public-read
        else
          aws s3 cp s3://$S3_BUCKET/$FILE_NAME s3://$S3_BUCKET/blockchain-latest.tar.lz4
          aws s3api put-object-acl --bucket $S3_BUCKET --key blockchain-latest.tar.lz4 --acl public-read
        fi
        if [[ ! $? -eq 0 ]]; then
          echo NOTIFYALERT! steemdsync was unable to overwrite the current blockchainstate with $FILE_NAME
          exit 1
        fi
        # Upload checksum after files uploaded
        aws s3 cp $CHECKSUM_BLOCKCHAIN_TAR_FILE s3://$S3_BUCKET/$FILE_TYPE-$CHECKSUM_BLOCKCHAIN_TAR_FILE
        aws s3api put-object-acl --bucket $S3_BUCKET --key $FILE_TYPE-$CHECKSUM_BLOCKCHAIN_TAR_FILE --acl public-read
        # upload a current block_log
        cd $HOME
        if [[ ! "$IS_BROADCAST_NODE" ]] && [[ ! "$IS_AH_NODE" ]]; then
          aws s3 cp blockchain/block_log s3://$S3_BUCKET/block_log-intransit
          aws s3 cp s3://$S3_BUCKET/block_log-intransit s3://$S3_BUCKET/block_log-latest
          aws s3api put-object-acl --bucket $S3_BUCKET --key block_log-latest --acl public-read
          CHECKSUM_BLOCK_LOG=`sha256sum blockchain/block_log | awk '{print $1}'`
          CHECKSUM_BLOCK_LOG_FILE="checksum-latest"
          echo $CHECKSUM_BLOCK_LOG > $CHECKSUM_BLOCK_LOG_FILE
          aws s3 cp $CHECKSUM_BLOCK_LOG_FILE s3://$S3_BUCKET/$CHECKSUM_BLOCK_LOG_FILE
          aws s3api put-object-acl --bucket $S3_BUCKET --key $CHECKSUM_BLOCK_LOG_FILE --acl public-read
        fi
        # kill the container starting the process over again
        echo steemdsync: stopping the container after a sync operation
        if [[ -e /tmp/isnewsync ]]; then
          echo notifysteemdsync: steemdsync: successfully generated and uploaded new blockchain-$VERSION-latest.tar.lz4 to s3://$S3_BUCKET
        fi
        RUN_SV_PID=`pgrep -f /etc/service/steemd`
        kill -9 $RUN_SV_PID
      else
        echo warning: last backup file is later less than 22 hours.
      fi
    else
      # if checksum file does not exist, create an empty one.
      echo "[info] Create empty checksum file, $CHECKSUM_BLOCKCHAIN_TAR_FILE"
      touch "${CHECKSUM_BLOCKCHAIN_TAR_FILE}"
      aws s3 cp $CHECKSUM_BLOCKCHAIN_TAR_FILE s3://$S3_BUCKET/$FILE_TYPE-$CHECKSUM_BLOCKCHAIN_TAR_FILE
    fi
  fi
fi

# check on this every 60 seconds
sleep 60
