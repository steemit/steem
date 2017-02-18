#!/bin/bash

export HOME="/var/lib/golosd"

STEEMD="/usr/local/bin/golosd"

#if [[ "$USE_WAY_TOO_MUCH_RAM" ]]; then
#    STEEMD="/usr/local/golosd-full/bin/golosd"
#fi

chown -R golosd:golosd $HOME

# seed nodes come from doc/seednodes which is
# installed by docker into /etc/golosd/seednodes
SEED_NODES="$(cat /etc/golosd/seednodes | awk -F' ' '{print $1}')"

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

if [[ ! -z "$STEEMD_WITNESS_NAME" ]]; then
    ARGS+=" --witness=\"$STEEMD_WITNESS_NAME\""
fi

if [[ ! -z "$STEEMD_MINER_NAME" ]]; then
    ARGS+=" --miner=[\"$STEEMD_MINER_NAME\",\"$STEEMD_PRIVATE_KEY\"]"
    ARGS+=" --mining-threads=$(nproc)"
fi

if [[ ! -z "$STEEMD_PRIVATE_KEY" ]]; then
    ARGS+=" --private-key=$STEEMD_PRIVATE_KEY"
fi

# overwrite local config with image one
cp /etc/golosd/config.ini $HOME/config.ini

chown golosd:golosd $HOME/config.ini

if [[ ! -d $HOME/blockchain ]]; then
    if [[ -e /var/cache/golosd/blocks.tbz2 ]]; then
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

if [[ ! -z "$STEEMD_RPC_ENDPOINT" ]]; then
    RPC_ENDPOINT=$STEEMD_RPC_ENDPOINT
else
    RPC_ENDPOINT="0.0.0.0:8090"
fi

if [[ ! -z "$STEEMD_P2P_ENDPOINT" ]]; then
    P2P_ENDPOINT=$STEEMD_P2P_ENDPOINT
else
    P2P_ENDPOINT="0.0.0.0:2001"
fi

exec chpst -ugolosd \
    $STEEMD \
        --rpc-endpoint=${RPC_ENDPOINT} \
        --p2p-endpoint=${P2P_ENDPOINT} \
        --data-dir=$HOME \
        $ARGS \
        $STEEMD_EXTRA_OPTS \
        2>&1
