#!/bin/bash
#params:
# - ref steemd location
# - tested steemd location
# - ref blockchain folder location
# - tested blockchain folder location
# - stop replay at block
#
# sudo ./docker_build_and_run.sh ~/steemit/steem/build/Release/programs/steemd ~/steemit/steem/build/Release/programs/steemd ~/steemit/steemd_data/steemnet ~/steemit/steemd_data/steemnet

if [ $# -ne 5 ]
then
   echo "Usage: reference_steemd_location tested_steemd_location ref_blockchain_folder_location tested_blockchain_folder_location stop_replay_at_block"
   echo "Example: ~/steemit/ref_steemd ~/steemit/steem/build/Release/programs/steemd ~/steemit/steemnet ~/steemit/testnet 5000000"
   exit -1
fi

echo $*

docker build --build-arg STOP_REPLAY_AT_BLOCK=$5 -t smoketest .
[ $? -ne 0 ] && echo docker build FAILED && exit -1

docker system prune -f

#docker run -v $1:/reference -v $2:/tested -v $3:/ref_blockchain -v $4:/tested_blockchain \
#   -it smoketest:latest /bin/bash
docker run -v $1:/reference -v $2:/tested -v $3:/ref_blockchain -v $4:/tested_blockchain \
   smoketest:latest
[ $? -ne 0 ] && echo docker run FAILED && exit -1
