#!/bin/bash

VERSION=`cat /etc/steemdversion`

STEEMD="/usr/local/steemd-testnet/bin/steemd"

UTILS="/usr/local/steemd-testnet/bin"

chown -R steemd:steemd $HOME

# clean out data dir since it may be semi-persistent block storage on the ec2 with stale data
rm -rf $HOME/*

mkdir -p $HOME/testnet_datadir

# for the startup node to connect to the fastgen
ARGS+=" --p2p-seed-node=127.0.0.1:12001"

# copy over config for testnet init and fastgen nodes
cp /etc/steemd/testnet.config.ini $HOME/config.ini
cp /etc/steemd/fastgen.config.ini $HOME/testnet_datadir/config.ini

chown steemd:steemd $HOME/config.ini
chown steemd:steemd $HOME/testnet_datadir/config.ini

cd $HOME

echo -en '\n' >> config.ini
echo -en '\n' >> testnet_datadir/config.ini

echo chain-id = $CHAIN_ID >> config.ini
echo chain-id = $CHAIN_ID >> testnet_datadir/config.ini

echo -en '\n' >> config.ini
echo -en '\n' >> testnet_datadir/config.ini

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/steemd.nginx.conf /etc/nginx/nginx.conf

# for appbase tags plugin loading
ARGS+=" --tags-skip-startup-update"

cd $HOME

# setup tinman
git clone https://github.com/steemit/tinman
virtualenv -p $(which python3) ~/ve/tinman
source ~/ve/tinman/bin/activate
cd tinman
pip install pipenv && pipenv install
pip install .

cd $HOME

# get latest tx-gen from s3
aws s3 cp s3://$S3_BUCKET/txgen-latest.list ./txgen.list
cp tinman/txgen.conf.example ./txgen.conf

chown -R steemd:steemd $HOME/*

echo steemd-testnet: starting fastgen node
# start the fastgen node
exec chpst -usteemd \
    $STEEMD \
        --webserver-ws-endpoint=0.0.0.0:9990 \
        --webserver-http-endpoint=0.0.0.0:9990 \
        --p2p-endpoint=0.0.0.0:12001 \
        --data-dir=$HOME/testnet_datadir \
        2>&1&

# give the fastgen node some time to startup
sleep 120

# set start_date in the tinman configuration to a date in the near-past so the testnet won't run out of blocks before it can be used.
# disable by setting environment variable $USE_SNAPSHOT_TIME to truthy value
if [ ! $USE_SNAPSHOT_TIME ]; then
  setDate=`date +%Y-%m-%dT%H:%M:%S -d "4 days ago"`
  tmp=$(mktemp)
  jq  --arg setDate $setDate '.start_time = $setDate' txgen.conf > "$tmp" && mv "$tmp" txgen.conf
fi

# pipe the transactions through keysub and into the fastgen node
echo steemd-testnet: pipelining transactions into fastgen node, this may take some time
( \
  echo [\"set_secret\", {\"secret\":\"$SHARED_SECRET\"}] ; \
  cat txgen.list \
) | \
tinman keysub --get-dev-key $UTILS/get_dev_key | \
tinman submit --realtime -t http://127.0.0.1:9990 --signer $UTILS/sign_transaction -f fail.json --timeout 1000 &

# add witness names to config file
i=0 ; while [ $i -lt 21 ] ; do echo witness = '"'init-$i'"' >> config.ini ; let i=i+1 ; done

# add keys derived from shared secret to config file
$UTILS/get_dev_key $SHARED_SECRET block-init-0:21 | cut -d '"' -f 4 | sed 's/^/private-key = /' >> config.ini

# loop until bootstrap catches up to the head block in real time
finished=0
while [[ $finished == 0 ]]
do
  head_block_num=$(curl -s --data '{"jsonrpc":"2.0","id":39,"method":"database_api.get_dynamic_global_properties"}' http://localhost:9990 | jq '.result.last_irreversible_block_num')
  num=$(curl -s --data "{\"jsonrpc\": \"2.0\", \"method\": \"call\", \"params\": [\"block_api\", \"get_block\", {\"block_num\":$head_block_num}], \"id\": 1}" http://localhost:9990 | jq '.result.block.transactions | length')
  if [[ $num == 0 ]]; then finished=1; fi
  sleep 60
done

# let's get going
echo steemd-testnet: bringing up witness / full node
cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
echo server 127.0.0.1:8091\; >> /etc/nginx/healthcheck.conf
echo } >> /etc/nginx/healthcheck.conf
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
/etc/init.d/fcgiwrap restart
service nginx restart
exec chpst -usteemd \
    $STEEMD \
        --webserver-ws-endpoint=0.0.0.0:8091 \
        --webserver-http-endpoint=0.0.0.0:8091 \
        --p2p-endpoint=0.0.0.0:2001 \
        --data-dir=$HOME \
        $ARGS \
        2>&1
