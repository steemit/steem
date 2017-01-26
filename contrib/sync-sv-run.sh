#!/bin/bash

# if the writer node dies by itself, kill runsv causing the container to exit
STEEMD_PID=`pgrep -f p2p-endpoint`
if [[ ! $? -eq 0 ]]; then
  echo ALERT! steemdsync has quit unexpectedly, starting a new instance..
  RUN_SV_PID=`pgrep -f /etc/service/steemd`
  kill -9 $RUN_SV_PID
fi

# check if we are synced, if so start the process of uploading to s3
# after uploading, kill runsv causing the container to exit
# and ecs-agent will start a new container starting the process over
BLOCKCHAIN_TIME=`curl --max-time 20 --data '{"id":39,"method":"get_dynamic_global_properties","params":[]}' localhost:8090 | jq -r .result.time`

if [[ ! -z "$BLOCKCHAIN_TIME" ]]; then
  BLOCKCHAIN_SECS=`date -d $BLOCKCHAIN_TIME +%s`
  CURRENT_SECS=`date +%s`

  # if we're within 5 seconds of current time, call it synced and begin the upload
  BLOCK_AGE=$((${CURRENT_SECS} - ${BLOCKCHAIN_SECS}))
  if [[ ${BLOCK_AGE} -le 10 ]]; then
    STEEMD_PID=`pgrep -f p2p-endpoint`
    kill -SIGINT $STEEMD_PID
    echo steemdsync: waiting for steemd to exit cleanly
    while [ -e /proc/$STEEMD_PID ]; do sleep 0.1; done
    echo steemdsync: starting a new blockchainstate upload operation
    cd $HOME
    echo steemdsync: compressing blockchainstate...
    tar cf blockchain.tar.bz2 --use-compress-prog=pbzip2 blockchain
    FILE_NAME=blockchain`date '+%d'`.tar.bz2
    echo steemdsync: uploading $FILE_NAME to $S3_UPLOAD_BUCKET
    aws s3 cp blockchain.tar.bz2 s3://$S3_UPLOAD_BUCKET/$FILE_NAME
    if [[ ! $? -eq 0 ]]; then
    	echo ALERT! steemdsync was unable to upload $FILE_NAME to s3://$S3_UPLOAD_BUCKET
    	exit 1
    fi
    echo steemdsync: replacing current version of blockchain.tar.bz2 with $FILE_NAME
    aws s3 cp s3://$S3_UPLOAD_BUCKET/$FILE_NAME s3://$S3_UPLOAD_BUCKET/blockchain.tar.bz2
    if [[ ! $? -eq 0 ]]; then
    	echo ALERT! steemdsync was unable to overwrite blockchain.tar.bz2 with $FILE_NAME
    	exit 1
    fi
    # kill the container starting the process over again
    echo steemdsync: stopping the container after a sync operation
    RUN_SV_PID=`pgrep -f /etc/service/steemd`
    kill -9 $RUN_SV_PID
  fi
fi

# check on this every 60 seconds
sleep 60
