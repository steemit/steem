#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys
import steem_utils.steem_runner
import steem_utils.steem_tools
import threading

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_id_collision_test.log"

MODULE_NAME = "SPS-Tester-via-steempy"
logger = logging.getLogger(MODULE_NAME)
logger.setLevel(LOG_LEVEL)

ch = logging.StreamHandler(sys.stdout)
ch.setLevel(LOG_LEVEL)
ch.setFormatter(logging.Formatter(LOG_FORMAT))

fh = logging.FileHandler(MAIN_LOG_PATH)
fh.setLevel(LOG_LEVEL)
fh.setFormatter(logging.Formatter(LOG_FORMAT))

if not logger.hasHandlers():
  logger.addHandler(ch)
  logger.addHandler(fh)

try:
    from steem import Steem
except Exception as ex:
    logger.error("SteemPy library is not installed.")
    sys.exit(1)

# we would like to test ID conflict problem and I'd like to have python scripts 
# which will be producing 2 transactions containing few proposals (having 
# specified some uuid as subject) and next submitted in opposite way to 2 nodes 
# (we can also specify some 0,5 delay between). Next the script should iterate 
# proposals from all nodes and compare uuids against ID assigned by each node. 

class ProposalsCreatorThread(threading.Thread):
    def __init__(self, node_url, proposals, private_key, delay):
        threading.Thread.__init__(self)
        self.node_url = node_url
        self.proposals = proposals
        self.private_key = private_key
        self.delay = delay
        self.log = logging.getLogger(MODULE_NAME + ".ProposalsCreatorThread." + self.node_url)
        self.node_client = Steem(nodes = [self.node_url], keys = [self.private_key])


    def run(self):
        self.log.info("Sending proposals to node at: {} with delay {}".format(self.node_url, self.delay))
        sleep(self.delay)
        for proposal in self.proposals:
            self.log.info("New proposal ==> ({},{},{},{},{},{},{})".format(
                proposal['creator'],
                proposal['receiver'],
                proposal['start_date'],
                proposal['end_date'],
                proposal['daily_pay'],
                proposal['subject'],
                proposal['permlink']
            ))

            self.node_client.commit.create_proposal(
                proposal['creator'],
                proposal['receiver'],
                proposal['start_date'],
                proposal['end_date'],
                proposal['daily_pay'],
                proposal['subject'],
                proposal['permlink']
            )


def get_permlink(account):
    return "steempy-proposal-title-{}".format(account)


def list_proposals_by_node(creator, private_key, nodes, subjects):
    for idx in range(0, len(nodes)):
        node = nodes[idx]
        logger.info("Listing proposals using node at {}".format(node))
        s = Steem(nodes = [node], keys = [private_key])
        proposals = s.list_proposals(creator, "by_creator", "direction_ascending", 1000, "all")
        for subject in subjects:
            msg = "Looking for id of proposal with subject {}".format(subject)
            for proposal in proposals:
                if proposal['subject'] == subject:
                    msg = msg + " - FOUND ID = {}".format(proposal['id'])
                    #assert proposal['id'] == results[subject], "ID do not match expected {} got {}".format(results[subject], proposal['id'])
                    break
            logger.info(msg)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create test accounts with")
    parser.add_argument("receiver", help = "Account to receive payment for proposal")
    parser.add_argument("wif", help="Private key for creator account")
    parser.add_argument("nodes_url", type=str, nargs="+", help="Url of working steem node")
    parser.add_argument("--delays", dest="delays", type=float, nargs="+", help="Delays for each worker/node (default 0)")
    parser.add_argument("--proposal-count", dest="proposal_count", type=int, default=1, help="Number of proposals each worker will create.")

    args = parser.parse_args()

    logger.info("Performing ID collision test with nodes {}".format(args.nodes_url))

    import steem_utils.steem_tools

    node_client = Steem(nodes = args.nodes_url, keys = [args.wif])
    logger.info("New post ==> ({},{},{},{},{})".format(
        "Steempy proposal title [{}]".format(args.creator), 
        "Steempy proposal body [{}]".format(args.creator), 
        args.creator, 
        get_permlink(args.creator), 
        "proposals"
    ))

    node_client.commit.post("Steempy proposal title [{}]".format(args.creator), 
        "Steempy proposal body [{}]".format(args.creator), 
        args.creator, 
        permlink = get_permlink(args.creator), 
        tags = "proposals"
    )

    steem_utils.steem_tools.wait_for_blocks_produced(5, args.nodes_url[0])

    workers = []

    import datetime
    import dateutil.parser
    now = node_client.get_dynamic_global_properties().get('time', None)
    if now is None:
        raise ValueError("Head time is None")
    now = dateutil.parser.parse(now)

    start_date = now + datetime.timedelta(days = 1)
    end_date = start_date + datetime.timedelta(days = 2)

    start_date_str = start_date.replace(microsecond=0).isoformat()
    end_date_str = end_date.replace(microsecond=0).isoformat()

    node_subjects = []
    only_subjects = []

    logger.info("Creating proposals and workers...")
    for idx in range(0, len(args.nodes_url)):
        proposals = []
        subjects = []
        for i in range(0, args.proposal_count):
            subject = str(uuid4())
            proposal = {
                'creator' : args.creator,
                'receiver' : args.receiver,
                'start_date' : start_date_str,
                'end_date' : end_date_str,
                'daily_pay' : "24.000 TBD",
                'subject' : subject,
                'permlink' : get_permlink(args.creator)
            }
            proposals.append(proposal)
            subjects.append(subject)
            only_subjects.append(subject)
        node_subjects.append(subjects)
        delay = 0.
        if args.delays:
            delay = args.delays[idx]
        worker = ProposalsCreatorThread(args.nodes_url[idx], proposals, args.wif, delay)
        workers.append(worker)

    logger.info("Starting workers...")
    for worker in workers:
        worker.start()
    
    logger.info("Waiting for workers to join...")
    for worker in workers:
        worker.join()


    logger.info("===== QUERY PROPOSALS =====")
    logger.info("Gathering proposals ID from the nodes where we send the transactions")
    results = {}
    for idx in range(0, len(node_subjects)):
        node = args.nodes_url[idx]
        s = Steem(nodes = [node], keys = [args.wif])
        proposals = s.list_proposals(args.creator, "by_creator", "direction_ascending", 1000, "all")
        for subject in node_subjects[idx]:
            for proposal in proposals:
                msg = "Looking for id of proposal sent to {} with subject {}".format(node, subject)
                if proposal['subject'] == subject:
                    msg = msg + " - FOUND ID = {}".format(proposal['id'])
                    results[subject] = proposal['id']
                    break
            logger.info(msg)

    logger.info("Checking for all transaction IDs by querying all nodes, IDs should match those gathered from nodes where we send the transactions")
    list_proposals_by_node(args.creator, args.wif, args.nodes_url, only_subjects)

    steem_utils.steem_tools.wait_for_blocks_produced(5, args.nodes_url[0])
    logger.info("Checking for all transaction IDs by querying all nodes (after some blocks produced)")
    list_proposals_by_node(args.creator, args.wif, args.nodes_url, only_subjects)




