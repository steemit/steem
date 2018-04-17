#!/bin/bash

function echo(){ builtin echo $(basename $0 .sh): "$@"; }
pushd () { command pushd "$@" > /dev/null; }
popd () { command popd "$@" > /dev/null; }

if [ $# -ne 6 ]
then
   echo Usage: jobs 1st_address 1st_port 2nd_address 2nd_port working_dir
   echo Example: 100 127.0.0.1 8090 ec2-34-235-166-184.compute-1.amazonaws.com 8090 logs
   exit -1
fi

SCRIPT_DIR=../scripts
PY_SCRIPT_DIR=../../api_tests
REPLAY_SCRIPT=run_replay.sh
NODE_SCRIPT=open_node.sh
NODE_ADDRESS=127.0.0.1
TEST_PORT=8090
REF_PORT=8091
TEST_NODE=$NODE_ADDRESS:$TEST_PORT
REF_NODE=$NODE_ADDRESS:$REF_PORT
TEST_NODE_OPT=--webserver-http-endpoint=$TEST_NODE
REF_NODE_OPT=--webserver-http-endpoint=$REF_NODE
EXIT_CODE=0
JOBS=$1
TEST_STEEMD_PATH=$2
REF_STEEMD_PATH=$3
TEST_WORK_PATH=$4
REF_WORK_PATH=$5
BLOCK_LIMIT=$6
WDIR=$PWD/logs
TEST_STEEMD_PID=-1
REF_STEEMD_PID=-1
export STEEMD_NODE_PID="default"

function run_replay {
   echo Running $SCRIPT_DIR/$REPLAY_SCRIPT $TEST_STEEMD_PATH $REF_STEEMD_PATH $TEST_WORK_PATH $REF_WORK_PATH $BLOCK_LIMIT
   $SCRIPT_DIR/$REPLAY_SCRIPT $TEST_STEEMD_PATH $REF_STEEMD_PATH $TEST_WORK_PATH $REF_WORK_PATH $BLOCK_LIMIT
   [ $? -ne 0 ] && echo test group FAILED && exit -1
}

function open_node {
   echo Running $SCRIPT_DIR/$NODE_SCRIPT $1 $2 $3 $4 $5
   . $SCRIPT_DIR/$NODE_SCRIPT $1 $2 $3 $4 $5
}

function run_test {
   pushd $PY_SCRIPT_DIR
   echo Running python3 $1 $JOBS http://$TEST_NODE http://$REF_NODE $WDIR
   python3 $1 $JOBS http://$TEST_NODE http://$REF_NODE 50 $WDIR
   [ $? -ne 0 ] && echo test FAILED && EXIT_CODE=-1
   popd
}

run_replay

open_node "tested" $TEST_STEEMD_PATH $TEST_NODE_OPT $TEST_WORK_PATH $TEST_PORT
TEST_STEEMD_PID=$STEEMD_NODE_PID

open_node "reference" $REF_STEEMD_PATH $REF_NODE_OPT $REF_WORK_PATH $REF_PORT
REF_STEEMD_PID=$STEEMD_NODE_PID

#echo TEST_STEEMD_PID: $TEST_STEEMD_PID REF_STEEMD_PID: $REF_STEEMD_PID
if [ $TEST_STEEMD_PID -ne -1 ] &&  [ $REF_STEEMD_PID -ne -1 ]; then
   run_test "test_list_votes.py"
   #run_test "test_list_votes2.py"	Obsolete
else
   EXIT_CODE=-1
fi

kill -s SIGINT $TEST_STEEMD_PID
kill -s SIGINT $REF_STEEMD_PID
wait

exit $EXIT_CODE
