#!/bin/bash

function echo(){ builtin echo $(basename $0 .sh): "$@"; }
pushd () { command pushd "$@" > /dev/null; }
popd () { command popd "$@" > /dev/null; }

if [ $# -ne 5 ]
then
   echo "Usage: path_to_tested_steemd path_to_reference_steemd path_to_test_blockchain_directory path_to_reference_blockchain_directory number_of_blocks_to_replay"
   echo "Example: ~/work/steemit/steem/build/programs/steemd/steemd ~/master/steemit/steem/build/programs/steemd/steemd ~/steemit/steem/work1 ~/steemit/steem/work2 2000000"
   echo "Note: Run this script from test group directory."
   exit -1
fi

STEEMD_CONFIG=config.ini
TEST_STEEMD_PATH=$1
REF_STEEMD_PATH=$2
TEST_WORK_PATH=$3
REF_WORK_PATH=$4
BLOCK_LIMIT=$5
RET_VAL1=-1
RET_VAL2=-1

function copy_config {
   echo Copying ./$STEEMD_CONFIG over $1/$STEEMD_CONFIG
   cp ./$STEEMD_CONFIG $1/$STEEMD_CONFIG
   [ $? -ne 0 ] && echo FATAL: Failed to copy ./$STEEMD_CONFIG over $1/$STEEMD_CONFIG file. && exit -1
}

copy_config $TEST_WORK_PATH
copy_config $REF_WORK_PATH

echo Running "test instance" replay of $BLOCK_LIMIT blocks
( $TEST_STEEMD_PATH --replay --stop-replay-at-block $BLOCK_LIMIT -d $TEST_WORK_PATH ) & REPLAY_PID1=$!

echo Running "reference instance" replay of $BLOCK_LIMIT blocks
( $REF_STEEMD_PATH --replay --stop-replay-at-block $BLOCK_LIMIT -d $REF_WORK_PATH ) & REPLAY_PID2=$!

wait $REPLAY_PID1
RET_VAL1=$?
wait $REPLAY_PID2
RET_VAL2=$?

[ $RET_VAL1 -ne 0 ] && echo "FATAL: tested steemd failed to replay $BLOCK_LIMIT blocks." && exit -1
[ $RET_VAL2 -ne 0 ] && echo "FATAL: reference steemd failed to replay $BLOCK_LIMIT blocks." && exit -1

exit 0