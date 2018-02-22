#!/bin/bash

if [ $# -ne 6 ]
then
   echo Usage: jobs 1st_address 1st_port 2nd_address 2nd_port working_dir
   echo Example: 100 127.0.0.1 8090 ec2-34-235-166-184.compute-1.amazonaws.com 8090 logs
   exit -1
fi

JOBS=$1
NODE1=http://$2:$3
NODE2=http://$4:$5
WDIR=$6

rm -fr $WDIR
mkdir $WDIR

./create_account_list.py $NODE1 $WDIR/accounts

EXTRACT_RESULT="python -c \
                'import sys, json; \
                  response=json.load(sys.stdin); \
                  result=response[\"result\"]; \
                  print(json.dumps(result, sort_keys=True, indent=2))'"

ERRORS=0

# account
get_account_history ()
{
   echo Comparing account history for $account

   local account=$1
   local START=-1
   local LIMIT=10000

   while true; do

   local GET_AH="curl -s --data \
      \"{ \\\"jsonrpc\\\": \\\"2.0\\\", \\\"id\\\": 0, \
          \\\"method\\\": \\\"account_history_api.get_account_history\\\", \
          \\\"params\\\": { \\\"account\\\": \\\"$account\\\", \\\"start\\\": $START, \\\"limit\\\": $LIMIT } }\""

   for tries in {1..10}
   do
   local JSON1=$(eval $GET_AH $NODE1)
   if [[ $? -eq 0 && $JSON1 != "" ]]; then
      JSON1=$(echo $JSON1 | python -c \
         'import sys, json; \
          response=json.load(sys.stdin); \
          result=response["result"]; \
          print(json.dumps(result, sort_keys=True, indent=2))')
      if [[ $? -eq 0 && $JSON1 != "" ]]; then
         break
      fi
   fi
   done
   [[ $? -ne 0 || $JSON1 == "" ]] && ((ERRORS++)) && echo ERROR: Failed to get history account for account from node $NODE1 >$WDIR/$account && return

   for tries in {1..10}
   do
   local JSON2=$(eval $GET_AH $NODE2)
   if [[ $? -eq 0 && $JSON2 != "" ]]; then
      JSON2=$(echo $JSON2 | python -c \
         'import sys, json; \
          response=json.load(sys.stdin); \
          result=response["result"]; \
          print(json.dumps(result, sort_keys=True, indent=2))')
      if [[ $? -eq 0 && $JSON2 != "" ]]; then
         break
      fi
   fi
   done
   [[ $? -ne 0 || $JSON2 == "" ]] && ((ERRORS++)) && echo ERROR: Failed to get history account for $account from node $NODE2 >$WDIR/$account && return

   if [[ "$JSON1" != "$JSON2" ]]
   then
      echo ERROR: Comparison failed >$WDIR/$account
      echo $NODE1 response: >>$WDIR/$account
      echo "$JSON1" >>$WDIR/$account
      echo $NODE2 response: >>$WDIR/$account
      echo "$JSON2" >>$WDIR/$account
      ((ERRORS++))
      return
   fi

   LAST=$(echo $JSON1 \
          | python -c \
            'import sys, json; \
             result=json.load(sys.stdin); \
             history=result["history"]; \
             print(history[0][0] if len(history) else 0)')

   if [ "$LAST" -eq 0 ]; then
      break
   fi

   START=$LAST
   if [ $LAST -gt 10000 ]; then
      LIMIT=10000
   else
      LIMIT=$LAST
   fi
   done # while true; do
} # get_account_history ()

CURRENT_JOBS=0

for account in $( <$WDIR/accounts ); do

if [ $ERRORS -ne 0 ]; then
   wait
   exit -1
fi

if [ $CURRENT_JOBS -eq $JOBS ]; then
   wait
   CURRENT_JOBS=0
fi

((CURRENT_JOBS++))
get_account_history $account &

done # for account in $( <accounts ); do

wait

exit $ERRORS