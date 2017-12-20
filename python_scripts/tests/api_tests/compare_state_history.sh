#!/bin/bash

EXIT_CODE=0
TEST_DIR=./compare_state
YAML_FILE=$TEST_DIR/compare_state.yaml
OUT_FILE=$TEST_DIR/get_ops_in_block.json.out
PAT_FILE=$TEST_DIR/get_ops_in_block.json.pat

if [ $# -ne 5 ]
then
   echo Usage: last_block_number 1st_address 1st_port 2nd_address 2nd_port
   echo Example: 10000 127.0.0.1 8090 ec2-34-235-166-184.compute-1.amazonaws.com 8090
   exit $EXIT_CODE
fi

BLOCK_LIMIT=$1
NODE1=http://$2:$3
NODE2=http://$4:$5

echo Checking NODE1...
pyresttest $NODE1 ./basic_smoketest.yaml
[ $? -ne 0 ] && echo FATAL: steemd not running at $NODE1? && exit -1

echo Checking NODE2...
pyresttest $NODE2 ./basic_smoketest.yaml
[ $? -ne 0 ] && echo FATAL: steemd not running at $NODE2? && exit -1

# Delete .out & .pat files here.
rm -f $OUT_FILE $PAT_FILE

BLOCK=1
while [ $BLOCK -le $BLOCK_LIMIT ]
do
   echo Comparing block number $BLOCK

   # Ask 1st node to get result into .out file.
   pyresttest --vars="{'BLOCK_NUMBER':$BLOCK}" $NODE1 $YAML_FILE --import_extensions='validator_ex;comparator_equal' &> /dev/null

   # Rename .out to .pat
   mv $OUT_FILE $PAT_FILE
   if [ $? -ne 0 ]
   then
      echo Failed to get block $BLOCK from node $NODE1
      exit -1
   fi

   # Ask 2nd node to compare with .pat from 1st node.
   pyresttest --vars="{'BLOCK_NUMBER':$BLOCK}" $NODE2 $YAML_FILE --import_extensions='validator_ex;comparator_equal'
   if [ $? -ne 0 ]
   then
      echo Comparison failed at block $BLOCK
      exit -2
   fi

   # Delete .pat file here.
   rm -f $PAT_FILE

   BLOCK=$[$BLOCK +1]
done

exit $EXIT_CODE
