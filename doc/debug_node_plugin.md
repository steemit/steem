
Plugin readme
-------------

The latest release (TODO: Version number) exposes the plugin architecture.
Each plugin has full access to the database and can change node behavior.
Each plugin may also provide API's.

The debug_node plugin
---------------------

The goal of the `debug_node` plugin is to start with the live chain, then easily
simulate future hypothetical actions.  The plugin simulates changes to chain state.
For example, you can edit an account's balances and signing keys to enable
performing (simulated) actions with that account.


Why this isn't unsafe
---------------------

Anyone (even you!) can edit their account to contain 1 million STEEM.  It's really
impossible for the developers to physically stop you from editing the memory/disk of your own
computer to make your local node think any account balance is any amount, so you can "give"
yourself 1 million STEEM.  Just like it's really impossible for your bank to physically stop
you from writing any numbers you'd like when balancing your checkbook, so you can "give"
yourself 1 million dollars.

But you have no way to control what other nodes do (or what your bank's clerks and computer systems do).
They do their own bookkeeping and keep track of what your real balance is (without all
the fake STEEM or fake dollars you "gave" yourself).  So you can believe whatever you want
about your balance and rewrite the rules of your own bookkeeping system to show you whatever balance you
want to be shown, but as soon as you try to actually spend STEEM (or dollars) that you don't actually have,
you'll be stopped because every other node on the network is a system you don't control that's keeping the
books properly (without all your edits to give yourself extra funds), and they do their own verification
of every transaction and will suppress any that doesn't have sufficient balance and a proper cryptographic
signature.

Why this is useful for debugging
--------------------------------

Say you've written some code for a hardfork -- some new feature or a bugfix that activates on every node
at a certain time or a certain block number -- e.g. January 1 at midnight (hypothetical date).
One basic question to ask is, if we apply that code to the current chain, does block production continue
as expected on January 1?  If the process segfaults, or block production hangs, or nobody can do transactions
after January 1, those are certainly showstopping problems that need to be fixed before your code is ready to
be included in a release.  So to test that it would be nice to speed up time and simulate creating blocks to
January 1, but you don't have any witness private keys.  With `debug_node_plugin` you can simulate changing
of witness keys to a key you control and produce as many blocks as you'd like.

Example usage
-------------

Let's say we want to get the history of the chain.  The first order of business is to save the blocks from a running node:

    cp -Rp datadir/blockchain/database/block_num_to_block /mydir/myblocks

Then create a new node with an empty datadir and no seed nodes.  (No seed nodes means it won't participate in the p2p network
and won't receive any blocks except the ones we'll explicitly tell it to load later.)  We enable the debug node and allow
access to its API to anyone:

    # no seed-node in config file or command line
    p2p-endpoint = 127.0.0.1:2001       # bind to localhost to prevent remote p2p nodes from connecting to us
    rpc-endpoint = 127.0.0.1:8090       # bind to localhost to secure RPC API access
    enable-plugin = witness account_history debug_node
    public-api = database_api login_api debug_node_api

The `public-api` section lists API's accessible to the RPC endpoint.  Since (as explained above) the `debug_node_api`
allows the database to be edited in ways that cause the node to report simulated chain state to clients and fail
to sync to the real network, RPC should not be accessible to the entire internet when the `debug_node_api` is in
the list of public API's.

The API's configured with `public-api` are assigned numbers starting at zero.  So the `debug_node_api` is callable by
API number 2 (TODO:  Explain about resolving names to API's and get it working).

The API provides the following methods
(see `libraries/plugins/debug_node/include/steemit/plugins/debug_node/debug_node_api.hpp`
for these definitions):

    void debug_push_blocks( std::string src_filename, uint32_t count );
    void debug_generate_blocks( std::string debug_key, uint32_t count );
    void debug_update_object( fc::variant_object update );
    void debug_stream_json_objects( std::string filename );
    void debug_stream_json_objects_flush();

Okay, let's run `steemd`.  It should start immediately with no blocks.  We can ask it to read blocks from the directory we saved earlier:

    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_push_blocks",["/mydir/myblocks", 1000]], "id": 1}' http://127.0.0.1:8090/rpc

As expected we now have 1000 blocks:

    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_dynamic_global_properties",[]], "id": 2}' http://127.0.0.1:8090/rpc

It queries the directory for blocks after the current head block, so we can subsequently load 500 more:

    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_push_blocks",["/mydir/myblocks", 500]], "id": 3}' http://127.0.0.1:8090/rpc

Let's generate some blocks:

    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_generate_blocks",["5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3",5]], "id": 4}' http://127.0.0.1:8090/rpc

As you can see, the `debug_node` performs a local edit of each witness's public key:

    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_witness_by_account",["dantheman4"]], "id": 5}' http://127.0.0.1:8990/rpc
    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_witness_by_account",["thisisnice4"]], "id": 6}' http://127.0.0.1:8990/rpc
    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_dynamic_global_properties",[]], "id": 7}' http://127.0.0.1:8090/rpc

The important information in the above is:

    ... "signing_key":"STM6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" ...
    ... "head_block_number":1505 ...

which demonstrates the witness keys have been reset and the head block number has been advanced with new blocks.  The blocks are signed by the above private key, and the database is edited to set the block signing key of the scheduled witnesses accordingly so the node accepts the simulated signatures as valid.

If we want to take control of an account we can do so by editing its key with `debug_update_object` command like this:

    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0,"lookup_account_names",[["steemit"]]], "id": 8}' http://127.0.0.1:8090/rpc    # find out ID of account we want is 2.2.28
    curl --data '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_update_object",[{"_action":"update","id":"2.2.28","active":{"weight_threshold":1,"key_auths":[["STM6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",1]]}}]], "id": 9}]' http://127.0.0.1:8090/rpc

Now that we've reset its key, we can take control of it in the wallet:
    programs/cli_wallet/cli_wallet -w debug_wallet.json -s ws://127.0.0.1:8090
    set_password abc
    unlock abc
    import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
    list_my_accounts
    transfer steemit dantheman "1.234 STEEM" "make -j100 money" true
    list_my_accounts
    get_account_history steemit -1 1000

(For some unknown reason, the current version of the wallet hangs after the transfer command -- why?)

Again, we're not actually taking control of anything, we're doing a "what-if" experiment -- what if the keys to this account had suddenly changed at block 1505 and then this transfer operation was broadcast?  And we find out that the chain and the wallet handle the situation as exepected (processing the transfer, putting it in the account history, and updating the balance).

This plugin allows all sorts of creative "what-if" experiments with the chain.
