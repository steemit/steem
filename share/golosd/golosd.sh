#!/bin/bash

export HOME="/var/lib/golosd"
REPLAY_FLAG="$HOME/replay"
FORCE_REPLAY_FLAG="$HOME/force-reply"

GOLOSD="/usr/local/bin/golosd"


chown -R golosd:golosd $HOME

# seed nodes come from documentation/seednodes which is
# installed by docker into /etc/golosd/seednodes
SEED_NODES="$(cat /etc/golosd/seednodes | awk -F' ' '{print $1}')"

ARGS=""

# if user did not pass in any desired
# seed nodes, use the ones above:
if [ -z "$GOLOSD_SEED_NODES" ]; then
    for NODE in $SEED_NODES ; do
        ARGS+=" --p2p-seed-node=$NODE"
    done
fi

# if user did pass in desired seed nodes, use
# the ones the user specified:
if [ ! -z "$GOLOSD_SEED_NODES" ]; then
    for NODE in $GOLOSD_SEED_NODES ; do
        ARGS+=" --p2p-seed-node=$NODE"
    done
fi

if [ ! -z "$GOLOSD_WITNESS_NAME" ]; then
    ARGS+=" --witness=\"$GOLOSD_WITNESS_NAME\""
fi

if [ ! -z "$GOLOSD_MINER_NAME" ]; then
    ARGS+=" --miner=[\"$GOLOSD_MINER_NAME\",\"$GOLOSD_PRIVATE_KEY\"]"
    if [ ! -z "$GOLOSD_MINING_THREADS" ]; then
        ARGS+=" --mining-threads=$GOLOSD_MINING_THREADS"
    else
        ARGS+=" --mining-threads=$(nproc)"
    fi
fi

if [ ! -z "$GOLOSD_PRIVATE_KEY" ]; then
    ARGS+=" --private-key=$GOLOSD_PRIVATE_KEY"
fi

# check existing of flag files for replay

if [ -f "$FORCE_REPLAY_FLAG" ]; then
    rm -f "$FORCE_REPLAY_FLAG"
    rm -f "$REPLAY_FLAG"
    if [ ! -f "$FORCE_REPLAY_FLAG" ] && [ ! -f "$REPLAY_FLAG" ]; then
        ARGS+=" --force-replay-blockchain"
    fi
fi

if [ -f "$REPLAY_FLAG" ]; then
    rm -f "$REPLAY_FLAG"
    if [ ! -f "$REPLAY_FLAG" ]; then
        ARGS+=" --replay-blockchain"
    fi
fi

# overwrite local config with image one
cp /etc/golosd/config.ini $HOME/config.ini

chown golosd:golosd $HOME/config.ini

if [ ! -d $HOME/blockchain ]; then
    if [ -e /var/cache/golosd/blocks.tbz2 ]; then
        # init with blockchain cached in image
        ARGS+=" --replay-blockchain"
        mkdir -p $HOME/blockchain/database
        cd $HOME/blockchain/database
        tar xvjpf /var/cache/golosd/blocks.tbz2
        chown -R golosd:golosd $HOME/blockchain
    fi
fi

# without --data-dir it uses cwd as datadir(!)
# who knows what else it dumps into current dir
cd $HOME

# slow down restart loop if flapping
sleep 1

if [ ! -z "$GOLOSD_HTTP_ENDPOINT" ]; then
    ARGS+=" --webserver-http-endpoint=$GOLOSD_HTTP_ENDPOINT"
fi

if [ ! -z "$GOLOSD_WS_ENDPOINT" ]; then
    ARGS+=" --webserver-ws-endpoint=$GOLOSD_WS_ENDPOINT"
fi

if [ ! -z "$GOLOSD_P2P_ENDPOINT" ]; then
    ARGS+=" --p2p-endpoint=$GOLOSD_P2P_ENDPOINT"
fi

exec chpst -ugolosd \
    $GOLOSD \
        --data-dir=$HOME \
        $ARGS \
        $GOLOSD_EXTRA_OPTS \
        2>&1