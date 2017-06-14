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
    echo NOTIFYALERT! steemdsync stacktrace from coredump: $STACKTRACE
  fi
  RUN_SV_PID=`pgrep -f /etc/service/steemd`
  kill -9 $RUN_SV_PID
fi

# check if we are synced, if so start the process of uploading to s3
# after uploading, kill runsv causing the container to exit
# and ecs-agent will start a new container starting the process over
BLOCKCHAIN_TIME=$(
    curl --silent --max-time 20 \
        --data '{"id":39,"method":"get_dynamic_global_properties","params":[]}' \
        localhost:8090 | jq -r .result.time
)

if [[ ! -z "$BLOCKCHAIN_TIME" ]]; then
  BLOCKCHAIN_SECS=`date -d $BLOCKCHAIN_TIME +%s`
  CURRENT_SECS=`date +%s`

  # if we're within 10 seconds of current time, call it synced and begin the upload
  BLOCK_AGE=$((${CURRENT_SECS} - ${BLOCKCHAIN_SECS}))
  if [[ ${BLOCK_AGE} -le 10 ]]; then
    STEEMD_PID=`pgrep -f p2p-endpoint`
    kill -SIGINT $STEEMD_PID
    echo steemdsync: waiting for steemd to exit cleanly
    while [ -e /proc/$STEEMD_PID ]; do sleep 0.1; done
    echo steemdsync: starting a new blockchainstate upload operation
    cd ${COMPRESSPATH:-$HOME}
    echo steemdsync: compressing blockchainstate...
    tar cf blockchain.tar.bz2 --use-compress-prog=pbzip2 -C $HOME blockchain
    if [[ ! $? -eq 0 ]]; then
      echo NOTIFYALERT! steemdsync was unable to compress shared memory file, check the logs.
      exit 1
    fi
    FILE_NAME=blockchain-$VERSION-`date '+%Y%m%d-%H%M%S'`.tar.bz2
    echo steemdsync: uploading $FILE_NAME to $S3_BUCKET
    aws s3 cp blockchain.tar.bz2 s3://$S3_BUCKET/$FILE_NAME
    if [[ ! $? -eq 0 ]]; then
    	echo NOTIFYALERT! steemdsync was unable to upload $FILE_NAME to s3://$S3_BUCKET
    	exit 1
    fi
    echo steemdsync: replacing current version of blockchain-latest.tar.bz2 with $FILE_NAME
    aws s3 cp s3://$S3_BUCKET/$FILE_NAME s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.bz2
    aws s3api put-object-acl --bucket $S3_BUCKET --key blockchain-$VERSION-latest.tar.bz2 --acl public-read 
    if [[ ! $? -eq 0 ]]; then
    	echo NOTIFYALERT! steemdsync was unable to overwrite the current blockchainstate with $FILE_NAME
    	exit 1
    fi
    # upload a current block_log
    cd $HOME
    aws s3 cp blockchain/block_log s3://$S3_BUCKET/block_log-intransit
    aws s3 cp s3://$S3_BUCKET/block_log-intransit s3://$S3_BUCKET/block_log-latest
    # kill the container starting the process over again
    echo steemdsync: stopping the container after a sync operation
    if [[ -e /tmp/isnewsync ]]; then
      echo notifysteemdsync: steemdsync: successfully generated and uploaded new blockchain-$VERSION-latest.tar.bz2 to s3://$S3_BUCKET
    fi
    RUN_SV_PID=`pgrep -f /etc/service/steemd`
    kill -9 $RUN_SV_PID
  fi
fi

# check on this every 60 seconds
sleep 60
