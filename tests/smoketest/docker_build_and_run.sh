#!/bin/bash
#params:
# - ref steemd location
# - tested steemd location
# - ref blockchain folder location
# - tested blockchain folder location
# - path to directory, where non-empty logs should be generated
# - stop replay at block
# - number of jobs (optional)
# - --dont-copy-config (optional), if passed config.init files are not copied from test directories
#
# WARNING: use absolute paths instead of relative!
#
# sudo ./docker_build_and_run.sh ~/steemit/steem/build/Release/programs/steemd ~/steemit/steem/build/Release/programs/steemd ~/steemit/steemd_data/steemnet ~/steemit/steemd_data/steemnet ~/steemit/logs 5000000 12

if [ $# -lt 6 -o $# -gt 8 ]
then
   echo "Usage: reference_steemd_location tested_steemd_location ref_blockchain_folder_location tested_blockchain_folder_location"
   echo "       logs_dir stop_replay_at_block [jobs [--dont-copy-config]"
   echo "Example: ~/steemit/ref_steemd ~/steemit/steem/build/Release/programs/steemd ~/steemit/steemnet ~/steemit/testnet"
   echo "         ~/steemit/logs 5000000 12"
   echo "         if <jobs> not passed, <nproc> will be used."
   exit -1
fi

echo $*

JOBS=0

if [ $# -eq 7 ]
then
   JOBS=$7
fi

docker build -t smoketest ../ -f Dockerfile
[ $? -ne 0 ] && echo docker build FAILED && exit -1

docker system prune -f

if [ -e $5 ]; then
   rm -rf $5/*
else
   mkdir -p $5
fi

docker run -v $1:/reference -v $2:/tested -v $3:/ref_blockchain -v $4:/tested_blockchain -v $5:/logs_dir -v /run:/run \
   -e STOP_REPLAY_AT_BLOCK=$6 -e JOBS=$JOBS -e COPY_CONFIG=$8 -p 8090:8090 -p 8091:8091 smoketest:latest
