
Python Debug Node Readme
------------------------

The Python Debug Node is a wrapper class that automates the creation and maintenance
of a running Steem Debug Node. The Debug Node is a plugin for Steem that allows realtime
local modification of the chain state in a way that mimicks real world behaviors
without corrupting a localally saved blockchain or propogating changes to the live chain.

More information of the debug node can be found [here](debug_node_plugin.md)

Why Should I Use This?
----------------------

While the Debug Node Plugin is very powerful, it does require intimate knowledge of how
to run a node in various configurations as well as interfacing with the node over the
RPC or WebSocket interfaces. Both of these are considered developer level skills. Python
is a higher level language that many amateur and skilled programmers use. There has already
been community development of Python libraries to make interfacing with a live node easier.
This plugin closes the gap by allowing a node to be launched programmatically in Python
in addition to interfacing with the node. This module utilizes community member Xeroc's
[Python Steem library](https://github.com/xeroc/python-steemlib).

How Do I Use This?
------------------

First of all, you need to install the module. Navigate to `tests/external_testing_scripts`
and run `python3 setup.py install`
To use the script include `from steemdebugnode import DebugNode`

There are a couple of examples already made that you can try modifying yourself.

[debug_hardforks.py](https://github.com/steemit/steem/python_scripts/tests/debug_hardforks.py)
This script starts a debug node, replays blocks, schedules a hardfork, and finally generates
new blocks after the hardfork. The script also communicates via the general purpose rpc
interface in Xeroc's Library to do a simple analysis of the results. In this case it
generates a historgram of block producers to verify the witness scheduling algorithm works
properly. The purpose of the script is it verify any given hardfork does not have a bug that
could crash the chain entirely.

[debugnode.py](https://github.com/steemit/steem/python_scripts/steemdebugnode/debugnode.py#L212)
This script is much simpler. It has the same parsing logic, but has much less test logic.
All it does is replay the blockchain, periodically printing a status update so the user
knows it is still working. The script then hangs so the user can interact with the chain
through RPC calls or the CLI Wallet.

What is the important part of these scripts?

``` Python
debug_node = DebugNode( str( steemd ), str( data_dir ) )
with debug_node:
   # Do stuff with blockchain
```

The module is set up to use the `with` construct. The object construction simply checks
for the correct directories and sets internal state. `with debug_node:` connects the node
and establishes the internal rpc connection. The script can then do whatever it wants to.
When the `with` block ends, the node automatically shutsdown and cleans up. The node uses
a system standard temp directory through the standard Python TemporaryDirectory as the
working data directory for the running node. The only work your script needs to do is
specify the steemd binary location and a populated data directory. For most configurations
this will be `programs/steemd/steemd` and `witness_node_data_dir` respectively, from the
git root directory for Steem.

TODO/ Long Term Goals
---------------------

While this is a great start to a Python testing framework, there are a lot of areas in
which this script is lacking. There is a lot of flexibility in the node, from piping
stdout and stderr for debugging to dynamically specifying plugins and APIs to run with,
there is a lack of ways to interface. Ideally, there would a Python equivalent wrapper
for every RPC API call. This is tedious and with plugins, those API calls could change
revision to revision. For the most part, the exposed Debug Node calls are wrappers for
the RPC call. Most, if not all, RPC API calls could be programatically generated from
the C++ source. It would also be a good step forward to introduce a simple testing framework
that could be used to start a debug node and then run a series of test cases on a common
starting chain state. This would address much of the integration testing that is sorely
needed for Steem.