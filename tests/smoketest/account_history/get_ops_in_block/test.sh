#!/bin/bash

EXIT_CODE=0

if [[ $# -lt 5 || $# -gt 7 ]]
then
   echo Usage: jobs 1st_address 1st_port 2nd_address 2nd_port [last_block [first_block]]
   echo        if jobs == 0 script detect processor count and use it
   echo        if last_block not passed or 0 will be read from steemd
   echo        if first_block not passed will be 0
   echo Example: 127.0.0.1 8090 ec2-34-235-166-184.compute-1.amazonaws.com 8090
   exit -1
fi

JOBS=$1
NODE1=http://$2:$3
NODE2=http://$4:$5
FIRST_BLOCK=0
LAST_BLOCK=0

if [ $# -eq 6 ]
then
   LAST_BLOCK=$6
fi

if [ $# -eq 7 ]
then
   FIRST_BLOCK=$7
fi

if [ $JOBS -eq 0 ]
then
   $JOBS=$(nproc -all)
fi

if [ $LAST_BLOCK -eq 0 ]
then
   for tries in {1..10}
   do
   LAST_BLOCK=$(curl -s --data '{ "jsonrpc": "2.0", "id": 0, "method": "database_api.get_dynamic_global_properties", "params": {} }' $NODE1\
                | python -c \
                  'import sys, json;\
                   print(json.load(sys.stdin)["result"]["head_block_number"])')
   if [[ $? -eq 0 && $LAST_BLOCK != "" ]]; then
      break
   fi
   done
   [[ $? -ne 0 || $LAST_BLOCK == "" ]] && echo FATAL: database_api.get_dynamic_global_properties on $NODE1 failed && exit -1

   for tries in {1..10}
   do
   LAST_BLOCK2=$(curl -s --data '{ "jsonrpc": "2.0", "id": 0, "method": "database_api.get_dynamic_global_properties", "params": {} }' $NODE2\
                 | python -c \
                   'import sys, json;\
                    print(json.load(sys.stdin)["result"]["head_block_number"])')
   if [[ $? -eq 0 && $LAST_BLOCK != "" ]]; then
      break
   fi
   done
   [[ $? -ne 0 || $LAST_BLOCK2 == "" ]] && echo FATAL: database_api.get_dynamic_global_properties on $NODE2 failed && exit -1

   if [ $LAST_BLOCK -ne $LAST_BLOCK2 ]
   then
      echo FATAL: $NODE1 head_block_number $LAST_BLOCK is different than $NODE2 head_block_number $LAST_BLOCK2
      exit -1
   fi
fi

echo $0 parameters JOBS=$JOBS NODE1=$NODE1 NODE2=$NODE2 FIRST_BLOCK=$FIRST_BLOCK LAST_BLOCK=$LAST_BLOCK

# node block
get_ops_in_block ()
{
local NODE=$1
local BLOCK=$2
local JSON=""

for tries in {1..10}; do
   JSON=$(curl -s --data "{ \"jsonrpc\": \"2.0\", \"id\": \"$BLOCK\", \"method\": \"account_history_api.get_ops_in_block\", \"params\": { \"block_num\": \"$BLOCK\", \"only_virtual\": false } }" $NODE)

   if [[ $? -eq 0 && $JSON != "" ]]; then
      JSON=$(echo $JSON \
             | python -c \
               'import sys, json; \
                response=json.load(sys.stdin); \
                result=response["result"]; \
                print(json.dumps(result, sort_keys=True, indent=2))')
      if [[ $? -eq 0 && $JSON != "" ]]; then
         break
      fi
   fi
done

echo $JSON
} # get_ops_in_block ()

# args: first_block last_block output
test_blocks ()
{
local BLOCK=$1
local OUTPUT=$3
local JSON1=""
local JSON2=""

echo Blocks range: [ $1 : $2 ] >$OUTPUT
echo >>$OUTPUT

while [ $BLOCK -le $2 ]
do
   echo Comparing block number $BLOCK

   JSON1=$(get_ops_in_block $NODE1 $BLOCK &)

   JSON2=$(get_ops_in_block $NODE2 $BLOCK &)

   wait

   [[ $JSON1 == "" ]] && echo ERROR: Failed to get block $BLOCK from node $NODE1 >>$OUTPUT && exit -1
   [[ $JSON2 == "" ]] && echo ERROR: Failed to get block $BLOCK from node $NODE2 >>$OUTPUT && exit -1

   if [[ "$JSON1" != "$JSON2" ]]
   then
      echo ERROR: Comparison failed at block $BLOCK >>$OUTPUT
      echo $NODE1 response: >>$OUTPUT
      echo "$JSON1" >>$OUTPUT
      echo $NODE2 response: >>$OUTPUT
      echo "$JSON2" >>$OUTPUT
      EXIT_CODE=-1
      return
   fi

   ((BLOCK++))
done

echo SUCCESS! >>$OUTPUT
} # test_blocks ()

if [ $JOBS -gt 1 ]
then
   BLOCKS=$(($LAST_BLOCK-$FIRST_BLOCK+1))
   BLOCKS_PER_JOB=$(($BLOCKS/$JOBS))

   for ((JOB=1;JOB<JOBS;++JOB))
   do
      test_blocks $FIRST_BLOCK $(($FIRST_BLOCK+$BLOCKS_PER_JOB-1)) "get_ops_in_block$JOB.log" &
      FIRST_BLOCK=$(($FIRST_BLOCK+$BLOCKS_PER_JOB))
   done
   OUTPUT="get_ops_in_block$JOBS.log"
else
   OUTPUT=get_ops_in_block.log
fi

test_blocks $FIRST_BLOCK $LAST_BLOCK $OUTPUT

wait

exit $EXIT_CODE