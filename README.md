# Introducing Golos (beta)

Golos is an experimental Proof of Work blockchain with an unproven consensus
algorithm.

# No Support & No Warranty

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

# Code is Documentation

Rather than attempt to describe the rules of the blockchain, it is up to
each individual to inspect the code to understand the consensus rules.

# Quickstart

Just want to get up and running quickly?  Try deploying a prebuilt
dockerized container.  Both common binary types are included.

## Dockerized p2p Node

To run a p2p node (ca. 2GB of memory is required at the moment):

    docker run \
        -d -p 2001:2001 -p 8090:8090 --name golosd-default \
        golosit/golos

    docker logs -f golosd-default  # follow along

## Dockerized Full Node

To run a node with *all* the data (e.g. for supporting a content website)
that uses ca. 14GB of memory and growing:

    docker run \
        --env USE_WAY_TOO_MUCH_RAM=1 \
        -d -p 2001:2001 -p 8090:8090 --name golosd-full \
        golosit/golos

    docker logs -f golosd-full

# Seed Nodes

A list of some seed nodes to get you started can be found in
[doc/seednodes](documentation/seednodes).

This same file is baked into the docker images and can be overridden by
setting `STEEMD_SEED_NODES` in the container environment at `docker run`
time to a whitespace delimited list of seed nodes (with port).

# How to Mine

The mining algorithm used by Golos requires the owner to have access to the
private key used by the account. This means it does not favor mining pools.

    ./golosd --miner=["accountname","${WIFPRIVATEKEY}"] \
        --witness="accountname" --seed-node="95.85.13.229:2225"

Make sure that your accountname is unique and not already used by someone
else or your proof of work might not be accepted by the blockchain.

# Building

See [doc/building.md](documentation/building.md) for detailed build instructions, including
compile-time options, and specific commands for Linux (Ubuntu LTS) or macOS X.

# Testing

See [doc/testing.md](documentation/testing.md) for test build targets and info
on how to use lcov to check code test coverage.
