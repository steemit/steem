#!/bin/bash

EXIT_CODE=0
JOBS=1
API_TEST_PATH=../../python_scripts/tests/api_tests
BLOCK_SUBPATH=blockchain/block_log
STEEMD_CONFIG=config.ini
TEST_ADDRESS=127.0.0.1
TEST_PORT=8090
TEST_NODE=$TEST_ADDRESS:$TEST_PORT
TEST_NODE_OPT=--webserver-http-endpoint=$TEST_NODE

function echo(){ builtin echo $(basename $0 .sh): "$@"; }
pushd () { command pushd "$@" > /dev/null; }
popd () { command popd "$@" > /dev/null; }

function print_help_and_quit {
   echo "Usage: path_to_tested_steemd path_to_blockchain_directory number_of_blocks_to_replay reference_node_address"
   echo "Example: ~/steemit/steem/build/Release/programs/steemd/steemd ~/steemit/steem/work 5000000 https://steemd.steemit.com"
   exit $EXIT_CODE
}

function run_test_group {
   echo Running test group $1
   pushd $1

   echo Copying ./$STEEMD_CONFIG over $WORK_PATH/$STEEMD_CONFIG
   cp ./$STEEMD_CONFIG $WORK_PATH/$STEEMD_CONFIG
   [ $? -ne 0 ] && echo FATAL: Failed to copy ./$STEEMD_CONFIG over $WORK_PATH/$STEEMD_CONFIG file. && exit -1

   echo Running replay of $BLOCK_LIMIT blocks
   $STEEMD_PATH --replay --stop-replay-at-block $BLOCK_LIMIT -d $WORK_PATH
   [ $? -ne 0 ] && echo FATAL: steemd failed to replay $BLOCK_LIMIT blocks. && exit -1

   echo Running steemd to listen
   $STEEMD_PATH $TEST_NODE_OPT -d $WORK_PATH & STEEMD_PID=$!
   #echo $TEST_ADDRESS $TEST_PORT 
   REF_ADDRESS=$(echo $REF_NODE | cut -d ":" -f3 | cut -c 3-)
   REF_PORT=$(echo $REF_NODE | grep -o ':[0-9]\{1,\}' | cut -c 2-)
   #echo REF_ADDRESS=$REF_ADDRESS REF_PORT=$REF_PORT

   echo Running ./test_group.sh $JOBS $TEST_ADDRESS $TEST_PORT $REF_ADDRESS $REF_PORT $BLOCK_LIMIT
   ./test_group.sh $JOBS $TEST_ADDRESS $TEST_PORT $REF_ADDRESS $REF_PORT $BLOCK_LIMIT
   [ $? -ne 0 ] && echo test group $1 FAILED && EXIT_CODE=-1

   kill -s SIGINT $STEEMD_PID
   wait
   popd
}

if [ $# -ne 4 ]
then
   print_help_and_quit
fi

STEEMD_PATH=$1
WORK_PATH=$2
BLOCK_LIMIT=$3
REF_NODE=$4

echo Checking $STEEMD_PATH...
if [ -x "$STEEMD_PATH" ] && file "$STEEMD_PATH" | grep -q "executable"
then
    echo $STEEMD_PATH is executable file.
else
    echo FATAL: $STEEMD_PATH is not executable file or found! && exit -1
fi

echo Checking $WORK_PATH...
if [ -e "$WORK_PATH" ] && [ -e "$WORK_PATH/$BLOCK_SUBPATH" ]
then
    echo $WORK_PATH/$BLOCK_SUBPATH found.
else
    echo FATAL: $WORK_PATH not found or $BLOCK_SUBPATH not found in $WORK_PATH! && exit -1
fi

: 'echo Checking $REF_NODE...
pyresttest $REF_NODE $API_TEST_PATH/basic_smoketest.yaml
if [ $? -eq 0 ]
then
   echo steemd is running at $REF_NODE.
else
    echo FATAL: steemd not running at $REF_NODE? && exit -1
fi'

for dir in ./*/
do
    dir=${dir%*/}
    run_test_group ${dir##*/}
done

exit $EXIT_CODE
