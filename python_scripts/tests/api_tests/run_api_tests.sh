#!/bin/bash

RPC_PORT=9876
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

pyresttest http://127.0.0.1:$RPC_PORT ./basic_smoketest.yaml
[ $? -ne 0 ] && echo FATAL: steemd not running? && exit -1

pyresttest http://127.0.0.1:$RPC_PORT ./database_api/database_api_test.yaml --import_extensions='validator_ex;'$COMPARATOR
[ $? -ne 0 ] && EXIT_CODE=-1

exit $EXIT_CODE
