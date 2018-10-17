
What is an API
--------------

An API is a set of related methods, accessible over Websocket / HTTP and provided by a single C++ class.  API's exist on a per-connection basis.

Compiling with hello_api plugin
-------------------------------

We will use the `hello_api` example plugin.  To follow along with the examples, you may enable the `hello_api` example plugin by running the following command and recompiling:

    ln -s ../example_plugins/hello_api external_plugins/hello_api

Publicly available API's
------------------------

Some API's are public API's which are available to clients without login.  These API's may be specified as follows in the configuration file:

    public-api = database_api login_api hello_api_api

Note, `hello_api` is the name of the plugin and `hello_api_api` is the name of the API it provides.  This slightly confusing naming may be revised in a later version of the plugin.

You may also specify `--public-api` on the command line.

Server configuration note:  If you customize `public-api` configuration, you should set the first two API's to `database_api` and `login_api`.

For experts:  The reason for recommending `database_api` and `login_api` to be the first two API's is that many clients expect these to be first two API's.
Additionally, the string API identifier feature's FC implementation requires a working `get_api_by_name` method to be available on API ID 1, so effectively
any client that uses string API identifiers (as recommended) requires `login_api` to be on API ID 1 even if no client uses the login feature.

Numeric API identifiers
-----------------------

API's are assigned numeric ID's in the order they are specified as public API's.  So for example you can access `hello_api_api` over HTTP like this:

    curl --data '{"jsonrpc": "2.0", "params": [2, "get_message", []], "id":1, "method":"call"}' http://127.0.0.1:8090/rpc

The number `2` is the numeric API identifier for `hello_api_api`.  These identifiers are assigned sequentially starting with 0 when the API is registered to the connection.
So effectively this means numeric API identifiers are determined by the order of `hello_api_api` in the `public-api` declaration above.

The `get_api_by_name` method on API 1 (the `login_api`) can be used to query the numeric ID by name:

    curl --data '{"jsonrpc": "2.0", "params": [1, "get_api_by_name", ["hello_api_api"]], "id":1, "method":"call"}' http://127.0.0.1:8790/rpc

String API identifiers
----------------------

Client code that references API's by their numeric identifiers have to coordinated with server-side configuration.
This is inconvenient, so the API server supports identifying the API by name as well, like this:

    curl --data '{"jsonrpc": "2.0", "params": ["hello_api_api", "get_message", []], "id":1, "method":"call"}' http://127.0.0.1:8090/rpc

It is considered best practice for API clients to use this syntax to reference API's by their string identifiers.  The reason is that the client becomes robust against
server-side configuration changes which result in renumbering of API's.

API access control methods
--------------------------

There are three methods to secure the API:

- Limit access to the API socket to a trusted machine by binding the RPC server to localhost
- Limit access to the API socket to a trusted LAN by firewall configuration
- Limit access to particular API's with username/password authentication

The Steem developers recommend using the first of these methods to secure the API by binding to localhost, as follows:

    rpc-endpoint = 127.0.0.1:8090

Securing specific API's
-----------------------

The problem with securing API's at the network level is that there are deployment scenarios where a node may want to have some API's public, but other API's private.
The `steemd` process includes username/password based authentication to individual API's.

Since the username/password is sent directly over the wire, you should use a TLS connection when authenticating with username and password.  TLS connection can be achieved by one of two methods:

- Configure an `rpc-tls-endpoint`.
- Configure an `rpc-endpoint` bound to localhost (or a secure LAN), and use an external reverse proxy (for example, `nginx`).  This advanced configuration will not be covered here.

You will need to create a key and certificate for the server in `.pem` format.  Many tutorials for creating SSL certificates exist, and many providers allow creating SSL
certificates signed by widely recognized CA's.  If you have a working `openssl` installation you can use these commands to generate a self-signed certificate:

    openssl req -x509 -newkey rsa:4096 -keyout data/ss-key.pem -out data/ss-cert.pem -days 365
    cat data/ss-{cert,key}.pem > data/ss-cert-key.pem

Then in your `config.ini` you must give the server access to the key and certificate:

    rpc-tls-endpoint = 0.0.0.0:8091
    server-pem = data/ss-cert-key.pem
    server-pem-password = password

When connecting to the API, you can use the `cli_wallet` to honor a specific certificate, individual CA, or CA bundle file with the `-a` option:

    programs/cli_wallet/cli_wallet -s wss://mydomain.tld:8091 -a data/ss-cert.pem

Note that the hostname `mydomain.tld` needs to match the CN field in the certificate.

Now let's protect the `hello_api_api` API to only be usable by user `bytemaster` with password `supersecret`.  Modify the `config.ini` file to make
sure we have the correct plugin which provides the sensitive API, make sure the sensitive API is not accessible in `public-api`, and add an
`api-user` definition to define the access policy:

    enable-plugin = witness account_history hello_api
    public-api = database_api login_api
    api-user = {"username" : "bytemaster", "password_hash_b64" : "T2k/wMBB9BKyv7X+ngghL+MaoubEuFb6GWvF3qQ9NU0=", "password_salt_b64" : "HqK9mAQCkWU=", "allowed_apis" : ["hello_api_api"]}

The values of `password_hash_b64` and `password_salt_b64` are generated by running `programs/util/saltpass.py` and entering your desired password.

Username/password access is only available through websockets, since login is inherently *stateful*.  We can use the `wscat` program to demonstrate the login functionality:

    wscat -c wss://mydomain.tld:8091
    {"jsonrpc": "2.0", "params": ["database_api", "get_dynamic_global_properties", []], "id":1, "method":"call"}
    < (ok)
    {"jsonrpc": "2.0", "params": ["hello_api_api", "get_message", []], "id":2, "method":"call"}
    < (error)
    {"jsonrpc": "2.0", "params": ["login_api", "login", ["bytemaster", "supersecret"]], "id":3, "method":"call"}
    < (ok)
    {"jsonrpc": "2.0", "params": ["hello_api_api", "get_message", []], "id":4, "method":"call"}
    < (ok)

The `cli_wallet` also has the capability to login to provide access to restricted API's.
