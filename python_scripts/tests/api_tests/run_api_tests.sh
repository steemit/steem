#!/bin/bash

RPC_PORT=9876
EXIT_CODE=0

pyresttest http://127.0.0.1:$RPC_PORT ./basic_smoketest.yaml
[ $? -ne 0 ] && echo FATAL: steemd not running? && exit -1

pyresttest http://127.0.0.1:$RPC_PORT ./database_api/database_api_test.yaml --import_extensions 'validator_ex'
[ $? -ne 0 ] && EXIT_CODE=-1

exit $EXIT_CODE
