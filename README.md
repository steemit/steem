# Introducing Golos (beta)

[![Build Status](https://travis-ci.org/GolosChain/golos.svg?branch=master)](https://travis-ci.org/GolosChain/golos)

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

Just want to get up and running quickly?  Try deploying a prebuilt dockerized container. 

```
sudo docker run -d \
    -p 4243:4243 \
    -p 8090:8090 \
    -p 8091:8091 \
    --name golos-default  goloschain/golos:latest
```    

To attach to the golosd you should use the cli_wallet:
```
sudo docker exec -ti golos-default \
    /usr/local/bin/cli_wallet \
    --server-rpc-endpoint="ws://127.0.0.1:8091"
```

# Building

See the [build instruction](https://github.com/GolosChain/golos/wiki/Build-instruction), which contains 
more information about configuring, building and running of docker containers.

# Testing

```
git clone https://github.com/GolosChain/golos.git
cd golos
sudo docker rm local/golos-test
sudo docker build -t local/golos-test -f share/golosd/docker/Dockerfile-test .
```

# Seed Nodes

A list of some seed nodes to get you started can be found in
[share/golosd/seednodes](share/golosd/seednodes).

This same file is baked into the docker images and can be overridden by
setting `STEEMD_SEED_NODES` in the container environment at `docker run`
time to a whitespace delimited list of seed nodes (with port).
