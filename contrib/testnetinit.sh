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

echo chain-id = $CHAIN_ID >> config.ini
echo chain-id = $CHAIN_ID >> testnet_datadir/config.ini

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/steemd.nginx.conf /etc/nginx/nginx.conf

# for appbase tags plugin loading
ARGS+=" --tags-skip-startup-update"

cd $HOME

# setup tinman
git clone https://github.com/steemit/tinman
cd tinman/tinman
git clone https://github.com/steemit/simple_steem_client
cd $HOME
virtualenv -p $(which python3) ~/ve/tinman
source ~/ve/tinman/bin/activate
pip install ./tinman

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

# pipe the transactions through keysub and into the fastgen node
echo steemd-testnet: pipelining transactions into fastgen node, this may take some time
( \
  echo [\"set_secret\", {\"secret\":\"$SHARED_SECRET\"}] ; \
  cat txgen.list \
) | \
tinman keysub --get-dev-key $UTILS/get_dev_key | \
tinman submit -t http://127.0.0.1:9990 --signer $UTILS/sign_transaction -f fail.json

# add witness names to config file
i=0 ; while [ $i -lt 21 ] ; do echo witness = '"'init-$i'"' >> config.ini ; let i=i+1 ; done

# add keys derived from shared secret to config file
$UTILS/get_dev_key $SHARED_SECRET block-init-0:21 | cut -d '"' -f 4 | sed 's/^/private-key = /' >> config.ini

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
