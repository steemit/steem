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
AH_TEST_SCRIPT=compare_account_history.sh
EXIT_CODE=0
JOBS=$1
TEST_ADDRESS=$2
TEST_PORT=$3
REF_ADDRESS=$4
REF_PORT=$5
BLOCK_LIMIT=$6
WDIR=$PWD/logs

function run_test {
   echo Running $SCRIPT_DIR/$AH_TEST_SCRIPT $JOBS $TEST_ADDRESS $TEST_PORT $REF_ADDRESS $REF_PORT $1
   pushd $SCRIPT_DIR
   ./$AH_TEST_SCRIPT $JOBS $TEST_ADDRESS $TEST_PORT $REF_ADDRESS $REF_PORT $1
   [ $? -ne 0 ] && echo test FAILED && EXIT_CODE=-1
   popd
}

run_test $WDIR

exit $EXIT_CODE