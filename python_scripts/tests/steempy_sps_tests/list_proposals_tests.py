#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys
import steem_utils.steem_runner
import steem_utils.steem_tools
import steem_utils.utils


LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_list_proposal.log"

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


START_END_SUBJECTS = [
    [1,3,"Subject001"],
    [2,3,"Subject002"],
    [3,3,"Subject003"],
    [4,3,"Subject004"],
    [5,3,"Subject005"],
    [6,3,"Subject006"],
    [7,3,"Subject007"],
    [8,3,"Subject008"],
    [9,3,"Subject009"]
]


def create_proposals(node_client, creator_account, receiver_account):
    import datetime
    now = datetime.datetime.now()

    start_date, end_date = steem_utils.utils.get_isostr_start_end_date(now, 10, 2)

    from steem.account import Account
    try:
        creator = Account(creator_account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(creator_account, ex))
        sys.exit(1)
    
    try:
        receiver = Account(receiver_account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(receiver_account, ex))
        sys.exit(1)

    logger.info("Creating initial post...")
    node_client.commit.post("Steempy proposal title", "Steempy proposal body", creator["name"], permlink = "steempy-proposal-title", tags = "proposals")

    logger.info("Creating proposals...")
    for start_end_subject in START_END_SUBJECTS:
        start_date, end_date = steem_utils.utils.get_isostr_start_end_date(now, start_end_subject[0], start_end_subject[1])

        node_client.commit.create_proposal(
            creator["name"], 
            receiver["name"], 
            start_date, 
            end_date,
            "16.000 TBD",
            start_end_subject[2],
            "steempy-proposal-title"
            )
        sleep(3)
    sleep(6)


def list_proposals_test(node_client, creator):
    logger.info("Testing direction ascending with start field given")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_ascending", 1000, "all")
    # we should get len(START_END_SUBJECTS) proposals with wirs proposal with subject Subject001
    # and last with subject Subject009
    assert len(proposals) == len(START_END_SUBJECTS), "Proposals count do not match assumed proposal count {}!={}".format(len(proposals), len(START_END_SUBJECTS))
    assert proposals[0]['subject'] == START_END_SUBJECTS[0][2], "Subject of the first proposal does not match with assumed proposal subject {}!={}".format(proposals[0]['subject'], START_END_SUBJECTS[0][2])
    assert proposals[-1]['subject'] == START_END_SUBJECTS[-1][2], "Subject of the last proposal does not match with assumed proposal subject {}!={}".format(proposals[-1]['subject'], START_END_SUBJECTS[-1][2])

    id_first = proposals[0]['id']
    id_last  = proposals[-1]['id']

    logger.info("Testing direction descending with start field given")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_descending", 1000, "all")
    # we should get len(START_END_SUBJECTS) proposals with first proposal with subject Subject009
    # and last with subject Subject001
    assert len(proposals) == len(START_END_SUBJECTS), "Proposals count do not match assumed proposal count {}!={}".format(len(proposals), len(START_END_SUBJECTS))
    assert proposals[0]['subject'] == START_END_SUBJECTS[-1][2], "Subject of the first proposal does not match with assumed proposal subject {}!={}".format(proposals[0]['subject'], START_END_SUBJECTS[-1][2])
    assert proposals[-1]['subject'] == START_END_SUBJECTS[0][2], "Subject of the last proposal does not match with assumed proposal subject {}!={}".format(proposals[-1]['subject'], START_END_SUBJECTS[0][2])

    # if all pass we can proceed with other tests
    # first we will test empty start string in defferent directions
    logger.info("Testing empty start string and ascending direction")
    proposals = node_client.list_proposals("", "by_start_date", "direction_ascending", 1, "all")
    # we shoud get proposal with Subject001
    assert proposals[0]['subject'] == START_END_SUBJECTS[0][2], "Subject of the proposal does not match with assumed proposal subject {}!={}".format(proposals[0]['subject'], START_END_SUBJECTS[0][2])
    
    # now we will test empty start string in descending
    logger.info("Testing empty start string and descending direction")
    proposals = node_client.list_proposals("", "by_start_date", "direction_descending", 1, "all")
    assert proposals[0]['subject'] == START_END_SUBJECTS[-1][2], "Subject of the proposal does not match with assumed proposal subject {}!={}".format(proposals[-1]['subject'], START_END_SUBJECTS[-1][2])
    
    # now we will test empty start string with ascending order and last_id set
    logger.info("Testing empty start string and ascenging direction and last_id set")
    proposals = node_client.list_proposals("", "by_start_date", "direction_ascending", 100, "all", 5)
    assert proposals[0]['id'] == 5, "First proposal should have id == 5, has {}".format(proposals[0]['id'])
    assert proposals[-1]['subject'] == START_END_SUBJECTS[-1][2], "Subject of the proposal does not match with assumed proposal subject {}!={}".format(proposals[-1]['subject'], START_END_SUBJECTS[-1][2])
    
    # now we will test empty start string with descending order and last_id set
    logger.info("Testing empty start string and descending direction and last_id set")
    proposals = node_client.list_proposals("", "by_start_date", "direction_descending", 100, "all", 5)
    assert proposals[0]['id'] == 5, "First proposal should have id == 5, has {}".format(proposals[0]['id'])
    assert proposals[-1]['subject'] == START_END_SUBJECTS[0][2], "Subject of the proposal does not match with assumed proposal subject {}!={}".format(proposals[-1]['subject'], START_END_SUBJECTS[0][2])

    # now we will test empty start string with ascending order and last_id set
    logger.info("Testing not empty start string and ascenging direction and last_id set")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_ascending", 100, "all", 5)
    assert proposals[0]['id'] == 5, "First proposal should have id == 5, has {}".format(proposals[0]['id'])
    assert proposals[-1]['subject'] == START_END_SUBJECTS[-1][2], "Subject of the proposal does not match with assumed proposal subject {}!={}".format(proposals[-1]['subject'], START_END_SUBJECTS[-1][2])

    # now we will test empty start string with descending order and last_id set
    logger.info("Testing not empty start string and descending direction and last_id set")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_descending", 100, "all", 5)
    assert proposals[0]['id'] == 5, "First proposal should have id == 5, has {}".format(proposals[0]['id'])
    assert proposals[-1]['subject'] == START_END_SUBJECTS[0][2], "Subject of the proposal does not match with assumed proposal subject {}!={}".format(proposals[-1]['subject'], START_END_SUBJECTS[0][2])

    logger.info("Testing not empty start string and ascending direction and last_id set to the last element")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_ascending", 100, "all", id_last)
    assert len(proposals) == 1
    assert proposals[0]['id'] == id_last

    logger.info("Testing not empty start string and descending direction and last_id set to the first element")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_descending", 100, "all", id_first)
    assert len(proposals) == 1
    assert proposals[0]['id'] == id_first

    logger.info("Testing not empty start string and ascending direction and last_id set to the first element")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_ascending", 100, "all", id_first)
    assert len(proposals) == len(START_END_SUBJECTS)

    logger.info("Testing not empty start string and descending direction and last_id set to the last element")
    proposals = node_client.list_proposals(creator, "by_creator", "direction_descending", 100, "all", id_last)
    assert len(proposals) == len(START_END_SUBJECTS)


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create test accounts with")
    parser.add_argument("receiver", help = "Account to receive payment")
    parser.add_argument("wif", help="Private key for creator account")
    parser.add_argument("--node-url", dest="node_url", default="http://127.0.0.1:8090", help="Url of working steem node")
    parser.add_argument("--run-steemd", dest="steemd_path", help = "Path to steemd executable. Warning: using this option will erase contents of selected steemd working directory.")
    parser.add_argument("--working_dir", dest="steemd_working_dir", default="/tmp/steemd-data/", help = "Path to steemd working directory")
    parser.add_argument("--config_path", dest="steemd_config_path", default="./steem_utils/resources/config.ini.in",help = "Path to source config.ini file")
    parser.add_argument("--no-erase-proposal", action='store_false', dest = "no_erase_proposal", help = "Do not erase proposal created with this test")


    args = parser.parse_args()

    node = None

    if args.steemd_path:
        logger.info("Running steemd via {} in {} with config {}".format(args.steemd_path, 
            args.steemd_working_dir, 
            args.steemd_config_path)
        )
        
        node = steem_utils.steem_runner.SteemNode(
            args.steemd_path, 
            args.steemd_working_dir, 
            args.steemd_config_path
        )
    
    node_url = args.node_url
    wif = args.wif

    if len(wif) == 0:
        logger.error("Private-key is not set in config.ini")
        sys.exit(1)

    logger.info("Using node at: {}".format(node_url))
    logger.info("Using private-key: {}".format(wif))

    keys = [wif]
    
    if node is not None:
        node.run_steem_node(["--enable-stale-production"])
    try:
        if node is None or node.is_running():
            node_client = Steem(nodes = [node_url], no_broadcast = False, 
                keys = keys
            )

            create_proposals(node_client, args.creator, args.receiver)
            list_proposals_test(node_client, args.creator)

            if node is not None:
                node.stop_steem_node()
            sys.exit(0)
        sys.exit(1)
    except Exception as ex:
        logger.error("Exception: {}".format(ex))
        if node is not None: 
            node.stop_steem_node()
sys.exit(1)

