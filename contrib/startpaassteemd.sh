#!/bin/bash

VERSION=`cat /etc/steemdversion`

if [[ "$IS_BROADCAST_NODE" ]]; then
  STEEMD="/usr/local/steemd-default/bin/steemd"
elif [[ "$IS_AH_NODE" ]]; then
  STEEMD="/usr/local/steemd-default/bin/steemd"
else
  STEEMD="/usr/local/steemd-full/bin/steemd"
fi

chown -R steemd:steemd $HOME

# clean out data dir since it may be semi-persistent block storage on the ec2 with stale data
rm -rf $HOME/*

# seed nodes come from doc/seednodes.txt which is
# installed by docker into /etc/steemd/seednodes.txt
SEED_NODES="$(cat /etc/steemd/seednodes.txt | awk -F' ' '{print $1}')"

ARGS=""

# if user did not pass in any desired
# seed nodes, use the ones above:
if [[ -z "$STEEMD_SEED_NODES" ]]; then
    for NODE in $SEED_NODES ; do
        ARGS+=" --p2p-seed-node=$NODE"
    done
fi

# if user did pass in desired seed nodes, use
# the ones the user specified:
if [[ ! -z "$STEEMD_SEED_NODES" ]]; then
    for NODE in $STEEMD_SEED_NODES ; do
        ARGS+=" --p2p-seed-node=$NODE"
    done
fi

NOW=`date +%s`
STEEMD_FEED_START_TIME=`expr $NOW - 1209600`

ARGS+=" --follow-start-feeds=$STEEMD_FEED_START_TIME"

STEEMD_PROMOTED_START_TIME=`expr $NOW - 604800`
ARGS+=" --tags-start-promoted=$STEEMD_PROMOTED_START_TIME"

if [[ ! "$DISABLE_BLOCK_API" ]]; then
   ARGS+=" --plugin=block_api"
fi

if [[ ! "$IS_BROADCAST_NODE" ]]; then
  ARGS+=" --follow-start-feeds=$STEEMD_FEED_START_TIME"
  ARGS+=" --disable-get-block"
fi

# overwrite local config with image one
if [[ "$IS_BROADCAST_NODE" ]]; then
  cp /etc/steemd/config-for-broadcaster.ini $HOME/config.ini
elif [[ "$IS_AH_NODE" ]]; then
  cp /etc/steemd/config-for-ahnode.ini $HOME/config.ini
else
  cp /etc/steemd/fullnode.config.ini $HOME/config.ini
fi

chown steemd:steemd $HOME/config.ini

cd $HOME

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/steemd.nginx.conf /etc/nginx/nginx.conf

# get blockchain state from an S3 bucket
echo steemd: beginning download and decompress of s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.bz2
if [[ "$USE_RAMDISK" ]]; then
  mkdir -p /mnt/ramdisk
  mount -t ramfs -o size=${RAMDISK_SIZE_IN_MB:-51200}m ramfs /mnt/ramdisk
  ARGS+=" --shared-file-dir=/mnt/ramdisk/blockchain"
  if [[ "$IS_BROADCAST_NODE" ]]; then
    s3cmd get s3://$S3_BUCKET/broadcast-$VERSION-latest.tar.bz2 - | pbzip2 -m2000dc | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
  elif [[ "$IS_AH_NODE" ]]; then
    s3cmd get s3://$S3_BUCKET/ahnode-$VERSION-latest.tar.bz2 - | pbzip2 -m2000dc | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
  else
    s3cmd get s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.bz2 - | lbzip2 -dc | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
  fi
  chown -R steemd:steemd /mnt/ramdisk/blockchain
else
  if [[ "$IS_BROADCAST_NODE" ]]; then
    s3cmd get s3://$S3_BUCKET/broadcast-$VERSION-latest.tar.bz2 - | pbzip2 -m2000dc | tar x
  elif [[ "$IS_AH_NODE" ]]; then
    s3cmd get s3://$S3_BUCKET/ahnode-$VERSION-latest.tar.bz2 - | pbzip2 -m2000dc | tar x
  else
    s3cmd get s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.bz2 - | lbzip2 -dc | tar x
  fi
fi
if [[ $? -ne 0 ]]; then
  if [[ ! "$SYNC_TO_S3" ]]; then
    echo notifyalert steemd: unable to pull blockchain state from S3 - exiting
    exit 1
  else
    echo notifysteemdsync steemdsync: shared memory file for $VERSION not found, creating a new one by replaying the blockchain
    mkdir blockchain
    aws s3 cp s3://$S3_BUCKET/block_log-latest blockchain/block_log
    if [[ $? -ne 0 ]]; then
      echo notifysteemdsync steemdsync: unable to pull latest block_log from S3, will sync from scratch.
    else
      ARGS+=" --replay-blockchain --force-validate"
    fi
    touch /tmp/isnewsync
  fi
fi

cd $HOME

if [[ "$SYNC_TO_S3" ]]; then
  touch /tmp/issyncnode
  chown www-data:www-data /tmp/issyncnode
fi

chown -R steemd:steemd $HOME/*

# let's get going
cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
echo server 127.0.0.1:8091\; >> /etc/nginx/healthcheck.conf
echo } >> /etc/nginx/healthcheck.conf
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
/etc/init.d/fcgiwrap restart
service nginx restart
exec chpst -usteemd \
    $STEEMD \
        --webserver-ws-endpoint=127.0.0.1:8091 \
        --webserver-http-endpoint=127.0.0.1:8091 \
        --p2p-endpoint=0.0.0.0:2001 \
        --data-dir=$HOME \
        $ARGS \
        $STEEMD_EXTRA_OPTS \
        2>&1&
SAVED_PID=`pgrep -f p2p-endpoint`
echo $SAVED_PID >> /tmp/steemdpid
mkdir -p /etc/service/steemd
if [[ ! "$SYNC_TO_S3" ]]; then
  cp /usr/local/bin/paas-sv-run.sh /etc/service/steemd/run
else
  cp /usr/local/bin/sync-sv-run.sh /etc/service/steemd/run
fi
chmod +x /etc/service/steemd/run
runsv /etc/service/steemd
