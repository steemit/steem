#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys
import os
import steem_utils.steem_runner

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_test.log"

MODULE_NAME = "SPS Tester via steempy"
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

def get_date_as_isostr(date):
    return date.replace(microsecond=0).isoformat()


def get_isostr_start_end_date(now, start_date_delta, end_date_delta):
    from datetime import timedelta

    start_date = now + timedelta(days = start_date_delta)
    end_date = start_date + timedelta(days = end_date_delta)

    start_date = start_date.replace(microsecond=0).isoformat()
    end_date = end_date.replace(microsecond=0).isoformat()

    return start_date, end_date


def test_create_proposal(node, creator_account, receiver_account, wif, subject):
    logger.info("Testing: create_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    
    import datetime
    now = datetime.datetime.now()

    start_date, end_date = get_isostr_start_end_date(now, 10, 2)

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

    ret = s.commit.post("Steempy proposal title", "Steempy proposal body", creator["name"], permlink = "steempy-proposal-title", tags = "proposals")

    ret = s.commit.create_proposal(
      creator["name"], 
      receiver["name"], 
      start_date, 
      end_date,
      "16.000 TBD",
      subject,
      "steempy-proposal-title"
    )

    assert ret["operations"][0][1]["creator"] == creator["name"]
    assert ret["operations"][0][1]["receiver"] == receiver["name"]
    assert ret["operations"][0][1]["start_date"] == start_date
    assert ret["operations"][0][1]["end_date"] == end_date
    assert ret["operations"][0][1]["daily_pay"] == "16.000 TBD"
    assert ret["operations"][0][1]["subject"] == subject
    assert ret["operations"][0][1]["permlink"] == "steempy-proposal-title"

def test_list_proposals(node, account, wif, subject):
    logger.info("Testing: list_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # list inactive proposals, our proposal shoud be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")
    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    
    # list active proposals, our proposal shouldnt be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "active")
    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is None

    # list all proposals, our proposal should be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "all")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None

def test_find_proposals(node, account, wif, subject):
    logger.info("Testing: find_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    ret = s.find_proposals([proposal_id])
    assert ret[0]["subject"] == found["subject"]

def test_vote_proposal(node, account, wif, subject):
    logger.info("Testing: vote_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    # now lets vote
    ret = s.commit.update_proposal_votes(account, [proposal_id], True)
    assert ret["operations"][0][1]["voter"] == account
    assert ret["operations"][0][1]["proposal_ids"][0] == proposal_id
    assert ret["operations"][0][1]["approve"] == True
    sleep(6)

def test_list_voter_proposals(node, account, wif, subject):
    logger.info("Testing: list_voter_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    voter_proposals = s.list_voter_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for voter, proposals in voter_proposals.items():
        for proposal in proposals:
            if proposal["subject"] == subject:
                found = proposal
    
    assert found is not None

def test_remove_proposal(node, account, wif, subject):
    logger.info("Testing: remove_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    # remove proposal
    s.commit.remove_proposal(account, [proposal_id])

    # try to find our special proposal
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is None


def test_iterate_results_test(node, creator_account, receiver_account, wif, subject, remove):
    logger.info("Testing: test_iterate_results_test")
    # test for iterate prosals
    # 1 we will create n proposals of which k proposal will have the same value in one of the fields
    # 2 then we will list proposals starting from kth proposal with limit set to m < k
    # 3 we list proposals again with the same conditiona as in 2, we should get the same set of results
    #   in real life scenatio pagination scheme with limit set to value lower than "k" will be showing
    #   the same results and will hang
    # 4 then we will use newly introduced last_id field, we should see diferent set of proposals
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])

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

    import datetime
    now = datetime.datetime.now()

    # 1 we will create n proposals of which k proposal will have the same value in one of the fields
    # here we have 5 proposals with the same start date
    start_end_pairs = [
        [1,1],
        [2,2],
        [4,3],
        [5,4],
        [5,5],
        [5,6],
        [5,7],
        [5,8],
        [6,9]
    ]

    for start_end_pair in start_end_pairs:
        start_date, end_date = get_isostr_start_end_date(now, start_end_pair[0], start_end_pair[1])

        s.commit.create_proposal(
            creator["name"], 
            receiver["name"], 
            start_date, 
            end_date,
            "16.000 TBD",
            subject,
            "steempy-proposal-title"
            )
        sleep(3)
    sleep(6)

    start_date = get_date_as_isostr(now + datetime.timedelta(days = 5))

    # 2 then we will list proposals starting from kth proposal with limit set to m < k
    proposals = s.list_proposals(start_date, "by_start_date", "direction_descending", 3, "all")
    assert len(proposals) == 3, "Expected {} elements got {}".format(3, len(proposals))
    ids = []
    for proposal in proposals:
        assert proposal["start_date"] == start_date, "Expected start_date do not match {} != {}".format(start_date, proposals[-1]["start_date"])
        ids.append(proposal["id"])
    assert len(ids) == 3, "Expected {} elements got {}".format(3, len(ids))

    # 3 we list proposals again with the same conditiona as in 2, we should get the same set of results
    proposals = s.list_proposals(start_date, "by_start_date", "direction_descending", 3, "all")
    assert len(proposals) == 3, "Expected {} elements got {}".format(3, len(proposals))
    oids = []
    for proposal in proposals:
        assert proposal["start_date"] == start_date, "Expected start_date do not match {} != {}".format(start_date, proposals[-1]["start_date"])
        oids.append(proposal["id"])
    assert len(oids) == 3, "Expected {} elements got {}".format(3, len(oids))

    # the same set of results check
    for id in ids:
        assert id in oids, "Id not found in expected results array {}".format(id)

    # 4 then we will use newly introduced last_id field, we should see diferent set of proposals
    proposals = s.list_proposals(start_date, "by_start_date", "direction_descending", 3, "all", oids[-1])

    start_date, end_date = get_isostr_start_end_date(now, 5, 4)

    assert proposals[-1]["start_date"] == start_date, "Expected start_date do not match {} != {}".format(start_date, proposals[-1]["start_date"])
    assert proposals[-1]["end_date"] == end_date, "Expected end_date do not match {} != {}".format(end_date, proposals[-1]["end_date"])

    # remove all created proposals
    if remove:
        start_date = get_date_as_isostr(now + datetime.timedelta(days = 6))
        for a in range(0, 2):
            proposals = s.list_proposals(start_date, "by_start_date", "direction_descending", 5, "all")
            ids = []
            for proposal in proposals:
                ids.append(int(proposal['id']))
            s.commit.remove_proposal(creator["name"], ids)
            sleep(12)


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create proposals with")
    parser.add_argument("receiver", help = "Account to receive funds")
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

    logger.info("Using node at: {}".format(node_url))
    logger.info("Using private-key: {}".format(wif))

    subject = str(uuid4())
    logger.info("Subject of testing proposal is set to: {}".format(subject))
    if node is not None:
        node.run_steem_node(["--enable-stale-production"])
    try:
        if node is None or node.is_running():
            test_create_proposal(node_url, args.creator, args.receiver, wif, subject)
            sleep(6)
            test_list_proposals(node_url, args.creator, wif, subject)
            test_find_proposals(node_url, args.creator, wif, subject)
            test_vote_proposal(node_url, args.creator, wif, subject)
            test_list_voter_proposals(node_url, args.creator, wif, subject)
            sleep(6)
            if args.no_erase_proposal:
                test_remove_proposal(node_url, args.creator, wif, subject)
            test_iterate_results_test(args.node_url, args.creator, args.receiver, args.wif, str(uuid4()), args.no_erase_proposal)
            sleep(6)
            if node is not None:
                node.stop_steem_node()
            sys.exit(0)
        sys.exit(1)
    except Exception as ex:
        logger.error("Exception: {}".format(ex))
        if node is not None: 
            node.stop_steem_node()
        sys.exit(1)

