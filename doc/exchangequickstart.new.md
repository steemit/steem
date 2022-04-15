Exchange Quickstart (Temp Version)
-------------------

1. Pull docker image and rename it
```
docker pull steemit/steem-fullnode-shared-memory:0.23.0
docker tag steemit/steem-fullnode-shared-memory:0.23.0 steem
```
> If you want to build your own Docker image, the Dockerfile is in this repo's root folder and names [Dockerfile.fullnode](https://github.com/steemit/steem/blob/exchange_node_build_doc/Dockerfile.fullnode).

2. Create the block data folder
```
mkdir /data
```

3. Create data folder structure and default config.ini
```
docker run -it --rm \
    -v /data:/var/steem steem \
    /usr/local/steemd/bin/steemd --data-dir /var/steem
```
After the steemd running, press `CTRL + C` to end it.

4. Empty `/data/blockchain` folder and download the latest `block_log` file into it.
```
rm -rf /data/blockchain/*
wget https://s3.amazonaws.com/steemit-dev-blockchainstate/block_log-latest -O /data/blockchain/block_log
```

5. Edit `/data/config.ini`.
```
plugin = witness account_by_key account_by_key_api condenser_api network_broadcast_api
plugin = webserver p2p json_rpc witness account_history
plugin = database_api condenser_api account_history_api

shared-file-size = 300G  # This is tested on 2022-03-30

account-history-track-account-range = ["your-exchange-account-name","your-exchange-account-name"]

p2p-seed-node = seed-east.steemit.com:2001 # steemit
p2p-seed-node = seed-central.steemit.com:2001 # steemit
p2p-seed-node = seed-west.steemit.com:2001 # steemit
p2p-seed-node = sn1.steemit.com:2001 # Steemit, Inc.
p2p-seed-node = sn2.steemit.com:2001 # Steemit, Inc.
p2p-seed-node = sn3.steemit.com:2001 # Steemit, Inc.
p2p-seed-node = sn4.steemit.com:2001 # Steemit, Inc.
p2p-seed-node = sn5.steemit.com:2001 # Steemit, Inc.
p2p-seed-node = sn6.steemit.com:2001 # Steemit, Inc.
p2p-seed-node = seed.justyy.com:2001 # @justyy
p2p-seed-node = steem-seed.urbanpedia.com:2001 # @fuli
p2p-seed-node = seed.steem.fans:2001 # @ety001
p2p-seed-node = seed-1.blockbrothers.io:2001 # @blockbrothers
p2p-seed-node = 5.189.136.20:2001 # @maiyude
p2p-seed-node = seed.steemhunt.com:2001 # @steemhunt Location: South Korea
p2p-seed-node = seed.steemzzang.com:2001 # @zzan.witnesses
p2p-seed-node = seednode.dlike.io:2001 # @dlike Location Germany
p2p-seed-node = seed.steemer.app:2001 # @bukio
p2p-seed-node = seed.supporter.dev:2001 # @dev.supporters
p2p-seed-node = seed.goodhello.net:2001 # @helloworld.wit

rpc-endpoint = 127.0.0.1:8090
```

> Make sure that you have removed the exist `p2p-seed-node` settings.

6. Run the fullnode and replay the blockchain data
```
docker run -itd \
    --name steem-fullnode \
    -p 8090:8090 \
    -v /data:/var/steem steem \
    /usr/local/steemd/bin/steemd --data-dir /var/steem --replay-blockchain
```

7. Check the container log
```
docker logs -f --tail 100 steem-fullnode
```
