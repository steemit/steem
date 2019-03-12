#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys

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

SUBJECT = str(uuid4())

def test_create_proposal(node, account, wif):
    logger.info("Testing: create_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    
    import datetime
    now = datetime.datetime.now()

    start_date = now + datetime.timedelta(days = 1)
    end_date = start_date + datetime.timedelta(days = 2)

    start_date = start_date.replace(microsecond=0).isoformat()
    end_date = end_date.replace(microsecond=0).isoformat()

    from steem.account import Account
    try:
        creator = Account(account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(account, ex))
        sys.exit(1)
    
    try:
        receiver = Account("treasury")
    except Exception as ex:
        logger.error("Account: {} not found. {}".format("treasury", ex))
        sys.exit(1)

    ret = s.commit.create_proposal(
      creator["name"], 
      receiver["name"], 
      start_date, 
      end_date,
      "16.000 TBD",
      SUBJECT,
      "mypermlink"
    )

    assert ret["operations"][0][1]["creator"] == account
    assert ret["operations"][0][1]["receiver"] == "treasury"
    assert ret["operations"][0][1]["start_date"] == start_date
    assert ret["operations"][0][1]["end_date"] == end_date
    assert ret["operations"][0][1]["daily_pay"] == "16.000 TBD"
    assert ret["operations"][0][1]["subject"] == SUBJECT
    assert ret["operations"][0][1]["url"] == "mypermlink"

def test_list_proposals(node, account, wif):
    logger.info("Testing: list_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # list inactive proposals, our proposal shoud be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, 0)
    found = None
    for proposal in proposals:
        if proposal["subject"] == SUBJECT:
            found = proposal
    
    assert found is not None
    
    # list active proposals, our proposal shouldnt be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, 1)
    found = None
    for proposal in proposals:
        if proposal["subject"] == SUBJECT:
            found = proposal
    
    assert found is None

    # list all proposals, our proposal should be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, -1)

    found = None
    for proposal in proposals:
        if proposal["subject"] == SUBJECT:
            found = proposal
    
    assert found is not None

def test_find_proposals(node, account, wif):
    logger.info("Testing: find_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, -1)

    found = None
    for proposal in proposals:
        if proposal["subject"] == SUBJECT:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    ret = s.find_proposals([proposal_id])
    assert ret[0]["subject"] == found["subject"]

def test_vote_proposal(node, account, wif):
    logger.info("Testing: vote_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, -1)

    found = None
    for proposal in proposals:
        if proposal["subject"] == SUBJECT:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    # now lets vote
    ret = s.commit.update_proposal_votes(account, [proposal_id], True)
    assert ret["operations"][0][1]["voter"] == account
    assert ret["operations"][0][1]["proposal_ids"][0] == proposal_id
    assert ret["operations"][0][1]["approve"] == True
    sleep(6)

def test_list_voter_proposals(node, account, wif):
    logger.info("Testing: list_voter_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    voter_proposals = s.list_voter_proposals(account, "by_creator", "direction_ascending", 1000, -1)

    found = None
    for voter, proposals in voter_proposals.items():
        for proposal in proposals:
            if proposal["subject"] == SUBJECT:
                found = proposal
    
    assert found is not None

def test_remove_proposal(node, account, wif):
    logger.info("Testing: remove_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, -1)

    found = None
    for proposal in proposals:
        if proposal["subject"] == SUBJECT:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    # remove proposal
    s.commit.remove_proposal(account, [proposal_id])

    # try to find our special proposal
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, -1)

    found = None
    for proposal in proposals:
        if proposal["subject"] == SUBJECT:
          found = proposal
    
    assert found is None


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--node-ip", help = "IP address of steem node", default = "http://127.0.0.1", dest = "node_ip")
    parser.add_argument("--node-port", help = "Steem node port", default = 8090, dest = "node_port")
    parser.add_argument("--account", default = "initminer", dest = "account")
    parser.add_argument("--wif", default = "5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n", dest = "wif")

    args = parser.parse_args()

    url = "{0}:{1}".format(args.node_ip, args.node_port)
    logger.info("Using node at: {}".format(url))

    test_create_proposal(url, args.account, args.wif)
    sleep(6)
    test_list_proposals(url, args.account, args.wif)
    test_find_proposals(url, args.account, args.wif)
    test_vote_proposal(url, args.account, args.wif)
    test_list_voter_proposals(url, args.account, args.wif)
    sleep(6)
    test_remove_proposal(url, args.account, args.wif)
