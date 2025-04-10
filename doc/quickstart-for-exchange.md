Quickstart for Exchange
-----------------------

# Node Setup

## I. Requirement

* 8 vCPU
* 16GB RAM
* 1.5 TB SSD DISK
* Docker needed

## II. Prepare

### 1. Create Node Folder

```
mkdir -p /steem/data/blockchain
```

### 2. Prepare blockchain data

```
cd /steem/data

wget -c https://steemit-dev-blockchainstate.s3.amazonaws.com/ahnode_latest.tar.lz4

tar --use-compress-program=lz4 -C /steem/data -xf ahnode_latest.tar.lz4
```

> This snapshot file is about 670GB at 2025-04-10.

After this step, you could find the `/steem/data/blockchain` folder is full of data.

### 3. Prepare config file

```
wget -c https://raw.githubusercontent.com/steemit/steem/master/contrib/config-for-ahnode.ini -O /steem/data/config.ini
```
> If you want to use this node as broadcast node, it needs add `plugin = network_broadcast_api` in `config.ini`.

### 4. Start Up Script

Create a script named `/steem/run.sh`.
```run.sh
#!/bin/bash
docker run -itd \
    --name steemd \
    -v /steem/data:/var/steem \
    -p 8090:8090 \
    steemit/steem:0.23.x-mira-ah \
    /usr/local/steemd/bin/steemd --data-dir /var/steem
```
> Port 8090 is webserver http endpoint.

### 5. Replay Script

Create a script named `/steem/replay.sh`.
```
#!/bin/bash
docker run -itd --rm \
    --name steemd \
    -v /steem/data:/var/steem \
    steemit/steem:0.23.x-mira-ah \
    /usr/local/steemd/bin/steemd --data-dir /var/steem --replay-blockchain
```

### 6. Stop Script

Create a script named `/steem/stop.sh`
```
#!/bin/bash
docker network disconnect bridge steemd
sleep 60
docker stop -t 600 steemd
docker rm steemd
```

### 7. Log Script

Create a script named `/steem/logs.sh`
```
#!/bin/bash
docker logs -f --tail 100 steemd
```

## III. Node Management
### 1. Start Node
```
./run.sh
```
### 2. Stop Node
```
./stop.sh
```
### 3. Check Node Log
```
./logs.sh
```

## IV. How to determine if the blockchain data has been updated to the latest?

Run `./logs.sh` to display the node logs. Until you find logs like this:

```
1457727ms p2p_plugin.cpp:212            handle_block         ] Got 12 transactions on block 94588899 by bountyking5 -- Block Time Offset: -272 ms
1460845ms p2p_plugin.cpp:212            handle_block         ] Got 6 transactions on block 94588900 by dhaka.witness -- Block Time Offset: -154 ms
1464190ms p2p_plugin.cpp:212            handle_block         ] Got 5 transactions on block 94588901 by upvu.witness -- Block Time Offset: 190 ms
1466684ms p2p_plugin.cpp:212            handle_block         ] Got 11 transactions on block 94588902 by steem.history -- Block Time Offset: -315 ms
```

If the node is not working properly, you should stop the node and replay the blockchain data.
```
./stop.sh
./replay.sh
```

Or remove all files in `/steem/data/blockchain` and re-download the latest snapshot file.

# Broadcast to Blockchain

## I. Cli Wallet

### 1. Get in container

```
docker exec -it steemd /bin/bash
```

Now you get in container.

### 2. Create wallet.json And Test Transfer

```
root@3583ab606cf0:/var/steem # /usr/local/steemd/bin/cli_wallet -w /var/steem/wallet.json -s ws://127.0.0.1:8091

Logging RPC to file: logs/rpc/rpc.log
Starting a new wallet
1435194ms main.cpp:169                  main                 ] wdata.ws_server: ws://127.0.0.1:8091
Please use the set_password method to initialize a new wallet before continuing

new >>> set_password 123456                                                                  # Set wallet password to 123456
set_password 123456
null

locked >>> unlock 123456                                                                     # Unlock wallet
unlock 123456
null

unlocked >>> import_key 5Kxxxxx                                                              # Import Active Private Key
import_key 5Kxxxxx
1622468ms wallet.cpp:427                save_wallet_file     ] saving wallet to file wallet.json
true

unlocked >>> transfer "from" "to" "0.001 STEEM" "memo info" true                             # Transfer STEEM
{
  "ref_block_num": 0,
  "ref_block_prefix": 0,
  "expiration": "2025-04-10T18:06:03",
  "operations": [[
      "transfer",{
        "from": "xxx",
        "to": "yyy",
        "amount": "0.001 STEEM",
        "memo": "memo info"
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1111"
  ],
  "transaction_id": "1111",
  "block_num": 1,
  "transaction_num": 1
}
```

> Press CTRL + D to exit wallet cli.

After these commands you would get a wallet file named `wallet.json` and its password is `123456`.

## Http Endpoint Via Steem JS SDK

Create a demo JS file named `transfer.js`. Its content is:

```
const steem = require('steem');
steem.api.setOptions({ url: 'IP_OF_STEEMD_CONTAINER:8090'});

steem.broadcast.transfer(
  'from',
  'to',
  '0.001 STEEM',
  'memo info',
  '123456',
  (err, result) => {
    console.log(err, result);
  }
);
```

Run this script:

```
$ npm install steem
$ node transfer.js
```