Quickstart
----------

### Get current steemd
Use docker:
```
docker run \
    -d -p 2001:2001 -p 8090:8090 --name steemd-default \
    steemit/steem
```
#### Low memory node?
Above runs low memory node, which is suitable for:
- seed nodes
- witness nodes
- exchanges, etc.
For full api node use:

```
docker run \
    --env USE_WAY_TOO_MUCH_RAM=1 --env USE_FULL_WEB_NODE=1 \
    -d -p 2001:2001 -p 8090:8090 --name steemd-full \
    steemit/steem
```
### Configure for your use case
#### Full API node
You need to use `USE_WAY_TOO_MUCH_RAM=1` and `USE_FULL_WEB_NODE=1` as stated above.
You can Use `contrib/fullnode.config.ini` as a base for your `config.ini` file.

#### Exchanges
Use low memory node.

Also make sure that your `config.ini` contains:
```
enable-plugin = account_history
public-api = database_api login_api
track-account-range = ["yourexchangeid", "yourexchangeid"]
```
Do not add other APIs or plugins unless you know what you are doing.

### Resources usage

Please make sure that you have enough resources available.
Check `shared-file-size =` in your `config.ini` to reflect your needs.
Set it to at least 20% more than current size.

Provided values are expected to grow significantly over time.

Blockchain data takes over **12GB** of storage space.

#### Full node
Shared memory file for full node uses over **45GB**

#### Seed node
Shared memory file for seed node uses over **3GB**

#### Other use cases
Shared memory file size varies, depends on your specific configuration but it is expected to be somewhere between "seed node" and "full node" usage.
