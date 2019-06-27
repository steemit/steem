#!/bin/bash

function echo(){ builtin echo $(basename $0 .sh): "$@"; }
pushd () { command pushd "$@" > /dev/null; }
popd () { command popd "$@" > /dev/null; }

if [ $# -lt 5 -o $# -gt 6 ]
then
   echo "Usage: path_to_tested_steemd path_to_reference_steemd"
   echo "       path_to_test_blockchain_directory path_to_reference_blockchain_directory"
   echo "       number_of_blocks_to_replay [--dont-copy-config]"
   echo "Example: ~/work/steemit/steem/build/programs/steemd/steemd ~/master/steemit/steem/build/programs/steemd/steemd ~/steemit/steem/work1 ~/steemit/steem/work2 2000000"
   echo "Note: Run this script from test group directory."
   exit -1
fi

STEEMD_CONFIG=config.ini
STEEMD_CONFIG_REF=config_ref.ini
STEEMD_CONFIG_TEST=config_test.ini
TEST_STEEMD_PATH=$1
REF_STEEMD_PATH=$2
TEST_WORK_PATH=$3
REF_WORK_PATH=$4
BLOCK_LIMIT=$5
COPY_CONFIG=$6
RET_VAL1=-1
RET_VAL2=-1
EXIT_CODE=0

function copy_config {
   if [ -e $STEEMD_CONFIG ]
   then
      echo Copying ./$STEEMD_CONFIG over $1/$STEEMD_CONFIG
      cp ./$STEEMD_CONFIG $1/$STEEMD_CONFIG
      [ $? -ne 0 ] && echo FATAL: Failed to copy ./$STEEMD_CONFIG over $1/$STEEMD_CONFIG file. && exit -1
   elif [ -e $2 ]
   then
      echo Copying ./$2 over $1/$STEEMD_CONFIG
      cp ./$2 $1/$STEEMD_CONFIG
      [ $? -ne 0 ] && echo FATAL: Failed to copy ./$2 over $1/$STEEMD_CONFIG file. && exit -1
   else
      echo WARNING: missing $STEEMD_CONFIG
   fi
}

function cleanup {
   ARG=$1
   if [ $REPLAY_PID1 ]
   then
     if [ $ARG -ne 0 ]; then
        sleep 0.1 && kill -s SIGINT $REPLAY_PID1 &
     fi
     wait -n $REPLAY_PID1
     [ $ARG -ne 0 -a $? -ne 0 ] && echo "FATAL: tested steemd failed to replay $BLOCK_LIMIT blocks." && EXIT_CODE=-1
   fi
   if [ $REPLAY_PID2 ]
   then
     if [ $ARG -ne 0 ]; then
        sleep 0.1 && kill -s SIGINT $REPLAY_PID2 &
     fi
     wait -n $REPLAY_PID2
     [ $ARG -ne 0 -a $? -ne 0 ] && echo "FATAL: reference steemd failed to replay $BLOCK_LIMIT blocks." && EXIT_CODE=-1
   fi

   if [ $ARG -ne 0 ]
   then
      EXIT_CODE=$ARG
   fi
}

trap cleanup SIGINT SIGPIPE

if [ "$COPY_CONFIG" != "--dont-copy-config" ]
then
   copy_config $TEST_WORK_PATH $STEEMD_CONFIG_TEST
   copy_config $REF_WORK_PATH  $STEEMD_CONFIG_REF
fi

echo Running "test instance" replay of $BLOCK_LIMIT blocks
( $TEST_STEEMD_PATH --replay --stop-replay-at-block $BLOCK_LIMIT -d $TEST_WORK_PATH ) & REPLAY_PID1=$!

echo Running "reference instance" replay of $BLOCK_LIMIT blocks
( $REF_STEEMD_PATH --replay --stop-replay-at-block $BLOCK_LIMIT -d $REF_WORK_PATH ) & REPLAY_PID2=$!

cleanup $EXIT_CODE
