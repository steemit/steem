Quickstart
----------

### Get current steemd
#### Docker
Preferred way is to use docker.
#### Building
If you can't or don't want to use Docker and you want to build `steemd` and `cli_wallet` yourself follow [build instructions](building.md). 
Those was made for `Ubuntu 16.04 LTS` and `macOS`.
If you have troubles building, please use Docker.

#### Low memory node?
Unless you run full api node, use
`LOW_MEMORY_NODE=ON`
Suitable for:
- seed nodes
- witness nodes
- exchanges, etc.
For full api node use `LOW_MEMORY_NODE=OFF`

### Configure for your use case
#### Full API node
Do not use low memory node for this case:
`LOW_MEMORY_NODE=OFF`
Use `contrib/fullnode.config.ini` as a base for your `config.ini` file.

#### Exchanges
Use low memory node:
`LOW_MEMORY_NODE=ON`
Also make sure that your `config.ini` contains:
```
enable-plugin = account_history
public-api = database_api login_api
track-account-range = ["yourexchangeid", "yourexchangeid"]
```
Do not add other APIs or plugins unless you know what you are doing.

### Resources usage

Please make sure that you have enough resources available.
Provided values are expected to grow significantly over time.

Blockchain data takes over **12GB** of storage space.

#### Full node
Shared memory file for full node uses over **45GB**

#### Seed node
Shared memory file for seed node uses over **3GB**

#### Other use cases
Shared memory file size varies, depends on your specific configuration but it is expected to be somewhere between "seed node" and "full node" usage.
