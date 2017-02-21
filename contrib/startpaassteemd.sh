#!/bin/bash
export HOME="/var/lib/steemd"

STEEMD="/usr/local/steemd-full/bin/steemd"
TEST_SYNC="/usr/local/test_sync.py"

chown -R steemd:steemd $HOME

# seed nodes come from doc/seednodes.txt which is
# installed by docker into /etc/steemd/seednodes.txt
SEED_NODES_FILE="/etc/steemd/seednodes.txt"
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
chown steemd:steemd $TEST_SYNC


cd $HOME

# get blockchain state from an S3 bucket
# if this url is not provieded then we might as well exit
if [[ ! -z "$BLOCKCHAIN_URL" ]]; then
  echo steemd: beginning download and decompress of $BLOCKCHAIN_URL
  s3cmd get $BLOCKCHAIN_URL - | pbzip2 -m2000dc | tar x
  if [[ $? -ne 0 ]]; then
    echo error: unable to pull blockchain state from S3 - exitting
    exit 1
  fi
else
  echo error: no URL specified to get blockchain state from - exiting
  exit 1
fi

# change owner of downloaded blockchainstate to steemd user
chown -R steemd:steemd /var/lib/steemd/*

# start a watchdog and detect if steemd syncs successfully
# with the main chain of steem.

if [[ "$RUN_SYNC_TEST" ]]; then
  exec chpst -usteemd \
      $TEST_SYNC --steemd $STEEMD --config-file $HOME/config.ini --seed-nodes $SEED_NODES_FILE \
      2>$1
fi


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
    sleep 5
    PORT_NUM=8092
    # don't generate endpoints in haproxy config if it already exists
    # this prevents adding to it if the docker container is stopped/started
    if [[ ! -f /etc/haproxy/haproxy.steem.cfg ]]; then
        cp /etc/haproxy/haproxy.cfg /etc/haproxy/haproxy.steem.cfg
        for (( i=2; i<=$(nproc); i++ ))
          do
            echo server server$PORT_NUM 127.0.0.1:$PORT_NUM maxconn 10000 weight 10 cookie server$PORT_NUM check >> /etc/haproxy/haproxy.steem.cfg
            ((PORT_NUM++))
        done
    fi
    PORT_NUM=8092
    for (( i=2; i<=$(nproc); i++ ))
      do
        exec chpst -usteemd \
        $STEEMD \
          --rpc-endpoint=127.0.0.1:$PORT_NUM \
          --data-dir=$HOME \
          --read-forward-rpc=127.0.0.1:8091 \
          --read-only \
          2>&1 &
          ((PORT_NUM++))
          sleep 1
    done
    # start haproxy now that the config file is complete with all endpoints
    # all of the read-only processes will connect to the write node onport 8091
    # haproxy will balance all incoming traffic on port 8090
    /usr/sbin/haproxy -D -f /etc/haproxy/haproxy.steem.cfg 2>&1
    # start runsv script that kills containers if they die
    mkdir -p /etc/service/steemd
    cp /usr/local/bin/paas-sv-run.sh /etc/service/steemd/run
    chmod +x /etc/service/steemd/run
    runsv /etc/service/steemd
else
    exec chpst -usteemd \
        $STEEMD \
            --rpc-endpoint=0.0.0.0:8090 \
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
