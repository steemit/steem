#!/bin/bash
export HOME="/var/lib/steemd"

STEEMD="/usr/local/steemd-full/bin/steemd"

export DATADIR="/media/ssd0"

chown -R steemd:steemd $HOME

# seed nodes come from doc/seednodes.txt which is
# installed by docker into /etc/steemd/seednodes.txt
SEED_NODES="$(cat /etc/steemd/seednodes.txt | awk -F' ' '{print $1}')"

ARGS=""

# if user did not pass in any desired
# seed nodes, use the ones above:
if [[ -z "$STEEMD_SEED_NODES" ]]; then
    for NODE in $SEED_NODES ; do
        ARGS+=" --seed-node=$NODE"
    done
fi

# if user did pass in desired seed nodes, use
# the ones the user specified:
if [[ ! -z "$STEEMD_SEED_NODES" ]]; then
    for NODE in $STEEMD_SEED_NODES ; do
        ARGS+=" --seed-node=$NODE"
    done
fi

# overwrite local config with image one
cp /etc/steemd/fullnode.config.ini $HOME/config.ini

chown steemd:steemd $HOME/config.ini

cd $HOME

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/steemd.nginx.conf /etc/nginx/nginx.conf

# get blockchain state from an S3 bucket
S3_DOWNLOAD_BUCKET=steemit-$NODE_ENV-blockchainstate
echo steemd: beginning download and decompress of s3://$S3_DOWNLOAD_BUCKET/blockchain-$VERSION-latest.tar.bz2
# clean out data dir since it's semi-persistent block storage on the ec2
rm -rf $DATADIR/*
cd $DATADIR
s3cmd get s3://$S3_DOWNLOAD_BUCKET/blockchain-$VERSION-latest.tar.bz2 - | pbzip2 -m2000dc | tar x --wildcards 'blockchain/shared*' -C $HOME 'blockchain/block*'
if [[ $? -ne 0 ]]; then
  if [[ ! "$SYNC_TO_S3"]]; then
    echo notifyalert steemd: unable to pull blockchain state from S3 - exiting
    exit 1
  else
    echo notifywarning steemdsync: shared memory file for $VERSION not found, creating a new one from scratch
    mkdir $DATADIR/blockchain 
  fi
fi
cd $HOME
ln -s blockchain/block_log $DATADIR/blockchain/block_log
ln -s blockchain/block_log.index $DATADIR/blockchain/block_log.index
ARGS+=" --shared-file-dir=$DATADIR/blockchain"
chown -R steemd:steemd $DATADIR/blockchain

if [[ "$SYNC_TO_S3" ]]
  touch /tmp/issyncnode
  chown www-data:www-data /tmp/issyncnode
fi

cd $HOME

chown -R steemd:steemd /var/lib/steemd/*

# start multiple read-only instances based on the number of cores
# attach to the local interface since a proxy will be used to loadbalance
if [[ "$USE_MULTICORE_READONLY" ]]; then
    exec chpst -usteemd \
        $STEEMD \
            --rpc-endpoint=127.0.0.1:8091 \
            --p2p-endpoint=0.0.0.0:2001 \
            --data-dir=$HOME \
            $ARGS \
            $STEEMD_EXTRA_OPTS \
            2>&1 &
    # sleep for a moment to allow the writer node to be ready to accept connections from the readers
    sleep 30
    PORT_NUM=8092
    cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
    CORES=$(nproc)
    PROCESSES=$((CORES * 4))
    for (( i=2; i<=$PROCESSES; i++ ))
      do
        echo server 127.0.0.1:$PORT_NUM\; >> /etc/nginx/healthcheck.conf
        ((PORT_NUM++))
    done
    echo } >> /etc/nginx/healthcheck.conf
    PORT_NUM=8092
    for (( i=2; i<=$PROCESSES; i++ ))
      do
        exec chpst -usteemd \
        $STEEMD \
          --rpc-endpoint=127.0.0.1:$PORT_NUM \
          --data-dir=$HOME \
          --shared-file-dir=/mnt/ramdisk/blockchain \
          --read-forward-rpc=127.0.0.1:8091 \
          --read-only \
          2>&1 &
          ((PORT_NUM++))
          sleep 1
    done
    # start nginx now that the config file is complete with all endpoints
    # all of the read-only processes will connect to the write node onport 8091
    # nginx will balance all incoming traffic on port 8090
    rm /etc/nginx/sites-enabled/default
    cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
    /etc/init.d/fcgiwrap restart
    service nginx restart
    # start runsv script that kills containers if they die
    mkdir -p /etc/service/steemd
    cp /usr/local/bin/paas-sv-run.sh /etc/service/steemd/run
    chmod +x /etc/service/steemd/run
    runsv /etc/service/steemd
else
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
    mkdir -p /etc/service/steemd
    if [[ ! "$SYNC_TO_S3" ]]; then
      cp /usr/local/bin/paas-sv-run.sh /etc/service/steemd/run
    else
      cp /usr/local/bin/sync-sv-run.sh /etc/service/steemd/run
    fi
    chmod +x /etc/service/steemd/run
    runsv /etc/service/steemd
fi
