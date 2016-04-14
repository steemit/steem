Introducing Steem (beta)
-----------------

Steem is an experimental Proof of Work blockchain with an unproven consensus
algorithm. 

  - **No Pre-mine, No ICO, No IPO**
  - Currency Symbol STEEM 
  - 1.000 STEEM block reward at launch
  - Approximately 113% APR long term inflation rate

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

    steemit          52.38.66.234:2001       (Oregon, USA)
    xeldal           45.55.217.111:12150     (USA)
    ihashfury        104.168.154.160:40696  
    lafona           52.4.250.181:39705     
    steempty         109.74.206.93:2001      (Israel)
    steem-id         45.114.118.146:2001     (Indonesia)
    cyrano.witness   81.89.101.133:2001      (Jena, Germany)


How to Mine
-----------

The mining algorithm used by Steem requires the owner to have access to the private key
used by the account. This means it does not favor mining pools.

    ./steemd --miner=["accountname","${WIFPRIVATEKEY}"] --witness="accountname" --seed-node="52.38.66.234:2001"

Make sure that your accountname is unique and not already used by someone else or your proof of work
might not be accepted by the blockchain.
