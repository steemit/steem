#!/bin/bash

NODE='http://127.0.0.1'
RPC_PORT=8090
EXIT_CODE=0
COMPARATOR=''

if [ $1 == 'equal' ]
then
   COMPARATOR='comparator_equal'
elif [ $1 == 'contain' ]
then
   COMPARATOR='comparator_contain'
else
   echo FATAL: $1 is not a valid comparator! && exit -1
fi

echo COMPARATOR: $COMPARATOR

pyresttest $NODE:$RPC_PORT ./basic_smoketest.yaml
[ $? -ne 0 ] && echo FATAL: steemd not running? && exit -1

pyresttest $NODE:$RPC_PORT ./database_api/database_api_test.yaml --import_extensions='validator_ex;'$COMPARATOR
[ $? -ne 0 ] && EXIT_CODE=-1

pyresttest $NODE:$RPC_PORT ./account_history_api/account_history_api_test.yaml --import_extensions='validator_ex;'$COMPARATOR
[ $? -ne 0 ] && EXIT_CODE=-1

exit $EXIT_CODE
