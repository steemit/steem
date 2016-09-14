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

    xeldal           45.55.217.111:12150                         (USA)
    ihashfury        104.168.154.160:40696
    lafona           52.4.250.181:39705
    steempty         109.74.206.93:2001       steem.clawmap.com  (UK)
    steem-id         steem-id.altexplorer.xyz:2001               (France)
    cyrano.witness   81.89.101.133:2001                          (Jena, Germany)
    kushed           40.76.37.6:2001          steem.kushed.com   (Azure)
    nextgencrypto    104.207.152.44:2201      steemwitness.com   (Los Angeles, USA)
    pharesim         78.46.32.4               steemd.pharesim.me (Falkenstein, German)
    liondani         212.117.213.186:2016                        (Switzerland)
    someguy123       199.241.186.130:2001     steemit-seed.someguy123.com:2001
    smooth.witness   52.74.152.79:2001

    52.38.66.234:2001
    52.37.169.52:2001
    52.26.78.244:2001
    192.99.4.226:2001
    46.252.27.1:1337
    81.89.101.133:2001
    52.4.250.181:39705
    steemd.pharesim.me:2001
    seed.steemed.net:2001
    steem.clawmap.com:2001
    seed.steemwitness.com:2001
    steem-seed1.abit-more.com:2001

Visit STEEM [Seed Nodes Status Monitoring](https://status.steemnodes.com/) page for more up-to-date seed-node list.

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
