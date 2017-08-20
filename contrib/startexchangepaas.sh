#!/bin/bash

VERSION=`cat /etc/steemdversion`

STEEMD="/usr/local/steemd-default/bin/steemd"

chown -R steemd:steemd $HOME

# clean out data dir since it may be semi-persistent block storage on the ec2 with stale data
rm -rf $HOME/*

# seed nodes come from doc/seednodes.txt which is
# installed by docker into /etc/steemd/seednodes.txt
SEED_NODES="$(cat /etc/steemd/seednodes.txt | awk -F' ' '{print $1}')"

ARGS=""

# if user did pass in desired seed nodes, use
# the ones the user specified:
if [[ ! -z "$STEEMD_SEED_NODES" ]]; then
    for NODE in $STEEMD_SEED_NODES ; do
        ARGS+=" --seed-node=$NODE"
    done
fi

if [[ ! -z "$TRACK_ACCOUNT" ]]; then
    ARGS+=" --track-account-range=[\"$TRACK_ACCOUNT\",\"$TRACK_ACCOUNT\"]"
fi

# overwrite local config with image one
cp /etc/steemd/config.ini $HOME/config.ini

chown steemd:steemd $HOME/config.ini

cd $HOME

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/steemd.nginx.conf /etc/nginx/nginx.conf

chown -R steemd:steemd $HOME/*

cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
echo server 127.0.0.1:8091\; >> /etc/nginx/healthcheck.conf
echo } >> /etc/nginx/healthcheck.conf
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
/etc/init.d/fcgiwrap restart
service nginx restart
exec chpst -usteemd \
    $STEEMD \
        --rpc-endpoint=0.0.0.0:8091 \
        --p2p-endpoint=0.0.0.0:2001 \
        --data-dir=$HOME \
        $ARGS \
        $STEEMD_EXTRA_OPTS \
        2>&1&
SAVED_PID=`pgrep -f p2p-endpoint`
echo $SAVED_PID >> /tmp/steemdpid
mkdir -p /etc/service/steemd
cp /usr/local/bin/paas-sv-run.sh /etc/service/steemd/run
chmod +x /etc/service/steemd/run
runsv /etc/service/steemd
