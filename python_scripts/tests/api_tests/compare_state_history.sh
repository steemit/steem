#!/bin/bash

EXIT_CODE=0
TEST_DIR=./compare_state
OUT_FILE=$TEST_DIR/get_ops_in_block.json.out
PAT_FILE=$TEST_DIR/get_ops_in_block.json.pat

function print_help_and_quit {
   echo "Usage: last_block_number [--pre-appbase] 1st_node_address [--pre-appbase] 2nd_node_address"
   echo "Example: 10000 --pre-appbase https://steemd.steemit.com http://127.0.0.1:8090"
   exit $EXIT_CODE
}

if [ $# -lt 3 ] || [ $# -gt 5 ]
then
   print_help_and_quit
fi

BLOCK_LIMIT=$1
NODE1=$2
NODE2=$3
YAML_FILE1=$TEST_DIR/compare_block_appbase.yaml
YAML_FILE2=$TEST_DIR/compare_block_appbase.yaml
COMPARATOR1=comparator_equal
COMPARATOR2=comparator_equal

case $# in
4) if [ $2 == "--pre-appbase" ]
   then
      NODE1=$3
      NODE2=$4
      YAML_FILE1=$TEST_DIR/compare_block_old.yaml
      COMPARATOR2=comparator_appbase_ops
   else 
      if [ $3 == "--pre-appbase" ]
      then
         # Here we switch nodes to avoid writing reversed comparator_appbase_ops
         NODE1=$4
         NODE2=$2
         YAML_FILE1=$TEST_DIR/compare_block_old.yaml
         COMPARATOR2=comparator_appbase_ops
      else
         print_help_and_quit
      fi
   fi
   ;;
5) if [ $2 != "--pre-appbase" ] || [ $4 != "--pre-appbase" ]
   then
      print_help_and_quit
   fi
      NODE1=$3
      NODE2=$5
      YAML_FILE1=$TEST_DIR/compare_block_old.yaml
      YAML_FILE2=$TEST_DIR/compare_block_old.yaml
   ;;
esac

echo Checking $NODE1...
pyresttest $NODE1 ./basic_smoketest.yaml
[ $? -ne 0 ] && echo FATAL: steemd not running at $NODE1? && exit -1

echo Checking $NODE2...
pyresttest $NODE2 ./basic_smoketest.yaml
[ $? -ne 0 ] && echo FATAL: steemd not running at $NODE2? && exit -1

# Delete .out & .pat files here.
rm -f $OUT_FILE $PAT_FILE

BLOCK=1
while [ $BLOCK -le $BLOCK_LIMIT ]
do
   echo Comparing block number $BLOCK

   # Ask 1st node to get result into .out file.
   pyresttest --vars="{'BLOCK_NUMBER':$BLOCK}" $NODE1 $YAML_FILE1 --import_extensions="validator_ex;$COMPARATOR1" &> /dev/null

   # Rename .out to .pat
   mv $OUT_FILE $PAT_FILE
   if [ $? -ne 0 ]
   then
      echo Failed to get block $BLOCK from node $NODE1
      exit -1
   fi

   # Ask 2nd node to compare with .pat from 1st node.
   pyresttest --vars="{'BLOCK_NUMBER':$BLOCK}" $NODE2 $YAML_FILE2 --import_extensions="validator_ex;$COMPARATOR2"
   if [ $? -ne 0 ]
   then
      echo Comparison failed at block $BLOCK
      exit -2
   fi

   # Delete .pat file here.
   rm -f $PAT_FILE

   BLOCK=$[$BLOCK +1]
done

exit 0
