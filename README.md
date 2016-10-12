Introducing Steem (beta)
-----------------

Steem is an experimental Proof of Work blockchain with an unproven consensus
algorithm.

  - Currency Symbol STEEM
  - 1.000 STEEM block reward at launch
  - Approximately 100% APR long term inflation rate

Public Announcement & Discussion
--------------------------------

Steem was announced on [Bitcointalk](https://bitcointalk.org/index.php?topic=1410943.new) prior to the start of any mining.

No Support & No Warranty
------------------------
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Code is Documentation
---------------------

Rather than attempt to describe the rules of the blockchain, it is up to
each individual to inspect the code to understand the consensus rules.

Seed Nodes
----------

You'll need to connect to seed nodes in order to download the blockchain and participate in STEEM's decentralized network.  [Here](https://status.steemnodes.com/)'s a listing of seed nodes that are currently online:

How to Mine
-----------

The mining algorithm used by Steem requires the owner to have access to the private key
used by the account. This means it does not favor mining pools.

    ./steemd --miner=["accountname","${WIFPRIVATEKEY}"] --witness="accountname" --seed-node="52.38.66.234:2001"

Make sure that your accountname is unique and not already used by someone else or your proof of work
might not be accepted by the blockchain.

OS-specific build instructions
------------------------------

See [here](doc/build-ubuntu.md) for Linux and [here](doc/build-osx.md) for OSX (Apple).

cmake Build Options
--------------------------

### CMAKE_BUILD_TYPE=[Release/Debug]

Specifies whether to build with or without optimization and without or with the symbol table for debugging. Unless you are specifically
debugging or running tests, it is recommended to build as release.

### LOW_MEMORY_NODE=[OFF/ON]

Builds steemd to be a consensus only low memory node. Data and fields not needed for consensus are not stored in the object database.
This option is recommended for witnesses and seed-nodes.

### ENABLE_CONTENT_PATCHING=[ON/OFF]

Allows content to be updated using a patch rather than a complete replacement.
If you do not need an API server or need to see the result of patching content then you can set this to OFF.

### CLEAR_VOTES=[ON/OFF]

Clears old votes from memory that are not longer required for consensus.

### BUILD_STEEM_TESTNET=[OFF/ON]

Builds steemd for use in a private testnet. Also required for correctly building unit tests

Testing
-------

The unit test target is `make chain_test`
This creates an executable `./tests/chain_test` that will run all unit tests.

Tests are broken in several categories:
```
basic_tests          // Tests of "basic" functionality
block_tests          // Tests of the block chain
live_tests           // Tests on live chain data (currently only past hardfork testing)
operation_tests      // Unit Tests of Steem operations
operation_time_tests // Tests of Steem operations that include a time based component (ex. vesting withdrawals)
serialization_tests  // Tests related of serialization
```

Code Coverage Testing
---------------------

If you have not done so, install lcov `brew install lcov`

```
cmake -D BUILD_STEEM_TESTNET=ON -D ENABLE_COVERAGE_TESTING=true -D CMAKE_BUILD_TYPE=Debug .
make
lcov --capture --initial --directory . --output-file base.info --no-external
tests/chain_test
lcov --capture --directory . --output-file test.info --no-external
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info
lcov -o interesting.info -r total.info tests/\*
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix `pwd`
```

Now open `lcov/index.html` in a browser
