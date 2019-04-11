# this test checks implementation of last_id field in list_voter_proposal arguments
#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys
import steem_utils.steem_runner
import steem_utils.steem_tools


LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_proposal_payment.log"

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

# create_account "initminer" "pychol" "" true
def create_accounts(node, creator, account):
    logger.info("Creating account: {}".format(account['name']))
    node.commit.create_account(account['name'], 
        owner_key=account['public_key'], 
        active_key=account['public_key'], 
        posting_key=account['public_key'],
        memo_key=account['public_key'],
        store_keys = False,
        creator=creator,
        asset='TESTS'
    )
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


# transfer_to_vesting initminer pychol "310.000 TESTS" true
def transfer_to_vesting(node, from_account, account, amount, asset):
    logger.info("Transfer to vesting from {} to {} amount {} {}".format(
        from_account, account['name'], amount, asset)
    )
    
    node.commit.transfer_to_vesting(amount, to = account['name'], 
        account = from_account, asset = asset
    )
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


# transfer initminer pychol "399.000 TESTS" "initial transfer" true
# transfer initminer pychol "398.000 TBD" "initial transfer" true
def transfer_assets_to_accounts(node, from_account, account, amount, asset):
    logger.info("Transfer from {} to {} amount {} {}".format(from_account, 
        account['name'], amount, asset)
    )
    node.commit.transfer(account['name'], amount, asset, 
        memo = "initial transfer", account = from_account
    )
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def get_permlink(account):
    return "steempy-proposal-title-{}".format(account)


def create_posts(node, account):
    logger.info("Creating posts...")
    logger.info("New post ==> ({},{},{},{},{})".format(
        "Steempy proposal title [{}]".format(account['name']), 
        "Steempy proposal body [{}]".format(account['name']), 
        account['name'], 
        get_permlink(account['name']), 
        "proposals"
    ))
    node.commit.post("Steempy proposal title [{}]".format(account['name']), 
        "Steempy proposal body [{}]".format(account['name']), 
        account['name'], 
        permlink = get_permlink(account['name']), 
        tags = "proposals")
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def create_proposals(node, account, start_date, end_date, proposal_count):
    logger.info("Creating proposals...")
    for idx in range(0, proposal_count):
        logger.info("New proposal ==> ({},{},{},{},{},{},{})".format(
            account['name'], 
            account['name'], 
            start_date, 
            end_date,
            "24.000 TBD",
            "Proposal from account {} {}/{}".format(account['name'], idx, proposal_count),
            get_permlink(account['name'])
        ))
        node.commit.create_proposal(
            account['name'], 
            account['name'], 
            start_date, 
            end_date,
            "24.000 TBD",
            "Proposal from account {} {}/{}".format(account['name'], idx, proposal_count),
            get_permlink(account['name'])
        )
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def list_proposals(node, account):
    proposals = node.list_proposals(account['name'], "by_creator", "direction_ascending", 1000, 'all')
    ids = []
    for proposal in proposals:
        ids.append(int(proposal.get('id', -1)))
    logger.info("Listing proposals: {}".format(ids))
    return ids


def vote_proposals(node, account, ids):
    logger.info("Voting proposals...")
    node.commit.update_proposal_votes(account['name'], ids, True)
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def list_voter_proposals(node, account, limit = 1000, last_id = None):
    logger.info("List voted proposals...")
    voter_proposals = node.list_voter_proposals(account['name'], 'by_creator', 'direction_ascending', limit, 'all', last_id)
    proposals = voter_proposals.get(account['name'], None)
    ids = []
    for proposal in proposals:
        ids.append(int(proposal['id']))
    return ids


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create test accounts with")
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

    account = {"name" : "tester001", "private_key" : "", "public_key" : ""}

    assert len(account["private_key"]) != 0, "Private key is empty"

    keys = [wif]
    keys.append(account["private_key"])

    logger.info(keys)
    
    if node is not None:
        node.run_steem_node(["--enable-stale-production"])
    try:
        if node is None or node.is_running():
            node_client = Steem(nodes = [node_url], no_broadcast = False, 
                keys = keys
            )

            # create accounts
            create_accounts(node_client, args.creator, account)
            # tranfer to vesting
            transfer_to_vesting(node_client, args.creator, account, "300.000", 
                "TESTS"
            )
            # transfer assets to accounts
            transfer_assets_to_accounts(node_client, args.creator, account, 
                "400.000", "TESTS"
            )

            transfer_assets_to_accounts(node_client, args.creator, account, 
                "400.000", "TBD"
            )

            # create post for valid permlinks
            create_posts(node_client, account)

            import datetime
            import dateutil.parser
            now = node_client.get_dynamic_global_properties().get('time', None)
            if now is None:
                raise ValueError("Head time is None")
            now = dateutil.parser.parse(now)

            start_date = now + datetime.timedelta(days = 1)
            end_date = start_date + datetime.timedelta(days = 2)

            end_date_blocks = start_date + datetime.timedelta(days = 2, hours = 1)

            start_date_str = start_date.replace(microsecond=0).isoformat()
            end_date_str = end_date.replace(microsecond=0).isoformat()

            end_date_blocks_str = end_date_blocks.replace(microsecond=0).isoformat()

            # create proposals - each account creates one proposal
            create_proposals(node_client, account, start_date_str, end_date_str, 5)

            proposals_ids = list_proposals(node_client, account)
            assert len(proposals_ids) == 5

            # each account is voting on proposal
            vote_proposals(node_client, account, proposals_ids)

            logger.info("All")
            proposals_ids = list_voter_proposals(node_client, account)
            logger.info(proposals_ids)

            logger.info("First three")
            proposals_ids = list_voter_proposals(node_client, account, 3)
            logger.info(proposals_ids)

            logger.info("Rest")
            proposals_ids = list_voter_proposals(node_client, account, 100, proposals_ids[-1])
            logger.info(proposals_ids)

            assert len(proposals_ids) == 3, "Expecting 3 proposals"
            assert proposals_ids[0] == 2, "Expecting id = 2"
            assert proposals_ids[-1] == 4, "Expecting id = 4"
    
            if node is not None:
                node.stop_steem_node()
            sys.exit(0)
        sys.exit(1)
    except Exception as ex:
        logger.error("Exception: {}".format(ex))
        if node is not None: 
            node.stop_steem_node()
        sys.exit(1)