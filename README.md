Introducing Steem (beta)
-----------------

Steem is an experimental Proof of Work blockchain with an unproven consensus
algorithm. 

  - Currency Symbol STEEM 
  - 1.000 STEEM block reward at launch
  - Approximately 100% APR long term inflation rate

Public Announcement & Discussion
--------------------------------

Steem was announced on [Bitcointalk](https://bitcointalk.org/index.php?topic=1410943.new) prior to
any the start of any mining.  

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
    steem-id         45.114.118.146:2001                         (Indonesia)
    cyrano.witness   81.89.101.133:2001                          (Jena, Germany)
    kushed           40.76.37.6:2001          steem.kushed.com   (Azure)
    nextgencrypto    104.207.152.44:2201      steemwitness.com   (Los Angeles, USA)
    pharesim         78.46.32.4               steemd.pharesim.me (Falkenstein, German)

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


How to Mine
-----------

The mining algorithm used by Steem requires the owner to have access to the private key
used by the account. This means it does not favor mining pools.

    ./steemd --miner=["accountname","${WIFPRIVATEKEY}"] --witness="accountname" --seed-node="52.38.66.234:2001"

Make sure that your accountname is unique and not already used by someone else or your proof of work
might not be accepted by the blockchain.

How to Update for Hardfork (Ubuntu)
-----------------------------------

Stop steemd
cd into your steem repository

```
git pull origin master
git submodule update --init --recursive
cmake -DENABLE_CONTENT_PATCHING=OFF .
make
cd programs/steemd
sudo ./steemd --rebuild-blockchain --rpc-endpoint
```
