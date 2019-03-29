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

MODULE_NAME = "SPS Tester via steempy - proposal payment test"
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


# 1. create few proposals.
# 2. vote on them to show differences in asset distribution (depending on collected votes)
# 3. wait for proposal payment phase
# 4. verify (using account history and by checking regular account balance) that given accounts have been correctly paid.


# create_account "initminer" "pychol" "" true
def create_accounts(node, creator, accounts):
    for account in accounts:
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
def transfer_to_vesting(node, from_account, accounts, amount, asset):
    for acnt in accounts:
        logger.info("Transfer to vesting from {} to {} amount {} {}".format(
            from_account, acnt['name'], amount, asset)
        )
        
        node.commit.transfer_to_vesting(amount, to = acnt['name'], 
            account = from_account, asset = asset
        )
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


# transfer initminer pychol "399.000 TESTS" "initial transfer" true
# transfer initminer pychol "398.000 TBD" "initial transfer" true
def transfer_assets_to_accounts(node, from_account, accounts, amount, asset):
    for acnt in accounts:
        logger.info("Transfer from {} to {} amount {} {}".format(from_account, 
            acnt['name'], amount, asset)
        )
        node.commit.transfer(acnt['name'], amount, asset, 
            memo = "initial transfer", account = from_account
        )
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def transfer_assets_to_treasury(node, from_account, treasury_account, amount, asset):
    logger.info("Transfer from {} to {} amount {} {}".format(from_account, 
        treasury_account, amount, asset)
    )
    node.commit.transfer(treasury_account, amount, asset, 
        memo = "initial transfer", account = from_account
    )
    steem_utils.steem_tools.wait_for_blocks_produced(2, node.url)


def get_permlink(account):
    return "steempy-proposal-title-{}".format(account)

def create_posts(node, accounts):
    logger.info("Creating posts...")
    for acnt in accounts:
        logger.info("New post ==> ({},{},{},{},{})".format(
            "Steempy proposal title [{}]".format(acnt['name']), 
            "Steempy proposal body [{}]".format(acnt['name']), 
            acnt['name'], 
            get_permlink(acnt['name']), 
            "proposals"
        ))
        node.commit.post("Steempy proposal title [{}]".format(acnt['name']), 
            "Steempy proposal body [{}]".format(acnt['name']), 
            acnt['name'], 
            permlink = get_permlink(acnt['name']), 
            tags = "proposals")
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def create_proposals(node, accounts, start_date, end_date):
    logger.info("Creating proposals...")
    for acnt in accounts:
        logger.info("New proposal ==> ({},{},{},{},{},{},{})".format(
            acnt['name'], 
            acnt['name'], 
            start_date, 
            end_date,
            "24.000 TBD",
            "Proposal from account {}".format(acnt['name']),
            get_permlink(acnt['name'])
        ))
        node.commit.create_proposal(
            acnt['name'], 
            acnt['name'], 
            start_date, 
            end_date,
            "24.000 TBD",
            "Proposal from account {}".format(acnt['name']),
            get_permlink(acnt['name'])
        )
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def vote_proposals(node, accounts):
    logger.info("Voting proposals...")
    for acnt in accounts:
        proposal_set = [x for x in range(0, len(accounts))]
        logger.info("Account {} voted for proposals: {}".format(acnt["name"], ",".join(str(x) for x in proposal_set)))
        node.commit.update_proposal_votes(acnt["name"], proposal_set, True)
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def list_proposals(node, start_date, status):
    proposals = node.list_proposals(start_date, "by_start_date", "direction_ascending", 1000, status)
    ret = []
    votes = []
    for proposal in proposals:
        ret.append("{}:{}".format(proposal.get('id', 'Error'), proposal.get('total_votes', 'Error')))
        votes.append(int(proposal.get('total_votes', -1)))
    logger.info("Listing proposals with status {} (id:total_votes): {}".format(status, ",".join(ret)))
    return votes

def print_balance(node, accounts):
    balances = []
    for acnt in accounts:
        ret = node.get_account(acnt['name'])
        logger.info("{} :: balance ==> {}, sbd_balance ==> {}".format(acnt['name'], ret.get('balance', 'Error'), ret.get('sbd_balance', 'Error')))
        balances.append(ret.get('sbd_balance', 'Error'))
    return balances


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create test accounts with")
    parser.add_argument("treasury", help = "Account to create test accounts with")

    args = parser.parse_args()

    node = steem_utils.steem_runner.SteemNode(
        "/home/dariusz-work/Builds/steem2/programs/steemd/steemd", 
        "/home/dariusz-work/steem-data", 
        "./steem_utils/resources/config.ini.in"
    )
    
    node_url = node.get_node_url()
    wif = node.get_from_config('private-key')[0]
    if len(wif) == 0:
        logger.error("Private-key is not set in config.ini")
        sys.exit(1)

    logger.info("Using node at: {}".format(node_url))
    logger.info("Using private-key: {}".format(wif))

    accounts = [
        ]

    if not accounts:
        logger.error("Accounts array is empty, please add accounts in a form {\"name\" : name, \"private_key\" : private_key, \"public_key\" : public_key}")
        sys.exit(1)

    keys = [wif]
    for account in accounts:
        keys.append(account["private_key"])
    
    node.run_steem_node(["--enable-stale-production"])
    try:
        if node.is_running():
            node_client = Steem(nodes = [node_url], no_broadcast = False, 
                keys = keys
            )

            # create accounts
            create_accounts(node_client, args.creator, accounts)
            # tranfer to vesting
            transfer_to_vesting(node_client, args.creator, accounts, "300.000", 
                "TESTS"
            )
            # transfer assets to accounts
            transfer_assets_to_accounts(node_client, args.creator, accounts, 
                "400.000", "TESTS"
            )

            transfer_assets_to_accounts(node_client, args.creator, accounts, 
                "400.000", "TBD"
            )

            logger.info("Balances for accounts after initial transfer")
            print_balance(node_client, accounts)
            # transfer assets to treasury
            transfer_assets_to_treasury(node_client, args.creator, args.treasury, 
                "1000000.000", "TESTS"
            )

            transfer_assets_to_treasury(node_client, args.creator, args.treasury, 
                "1000000.000", "TBD"
            )

            logger.info("Balances for treasury account after initial transfer")
            print_balance(node_client, [{'name' : args.treasury}])

            # create post for valid permlinks
            create_posts(node_client, accounts)

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
            create_proposals(node_client, accounts, start_date_str, end_date_str)

            # list proposals with inactive status, it shoud be list of pairs id:total_votes
            list_proposals(node_client, start_date_str, "inactive")

            # each account is voting on proposal
            vote_proposals(node_client, accounts)

            # list proposals with inactive status, it shoud be list of pairs id:total_votes
            votes = list_proposals(node_client, start_date_str, "inactive")
            for vote in votes:
                #should be 0 for all
                assert vote == 0

            logger.info("Balances for accounts after creating proposals")
            balances = print_balance(node_client, accounts)
            for balance in balances:
                #should be 390.000 TBD for all
                assert balance == '390.000 TBD'

            logger.info("Balances for treasury after creating proposals")
            print_balance(node_client, [{'name' : args.treasury}])

            # move forward in time to see if proposals are paid
            # moving is made in 1h increments at a time, after each 
            # increment balance is printed
            logger.info("Moving to date: {}".format(start_date_str))
            node_client.debug_generate_blocks_until(wif, start_date_str, False)
            current_date = start_date
            while current_date < end_date:
                current_date = current_date + datetime.timedelta(hours = 1)
                current_date_str = current_date.replace(microsecond=0).isoformat()
                logger.info("Moving to date: {}".format(current_date_str))
                node_client.debug_generate_blocks_until(wif, current_date_str, False)

                logger.info("Balances for accounts at time: {}".format(current_date_str))
                print_balance(node_client, accounts)
                logger.info("Balances for treasury at time: {}".format(current_date_str))
                print_balance(node_client, [{'name' : args.treasury}])
                votes = list_proposals(node_client, start_date_str, "active")
                for vote in votes:
                    # should be > 0 for all
                    assert vote > 0

            # move additional hour to ensure that all proposals ended
            logger.info("Moving to date: {}".format(end_date_blocks_str))
            node_client.debug_generate_blocks_until(wif, end_date_blocks_str, False)
            logger.info("Balances for accounts at time: {}".format(end_date_blocks_str))
            balances = print_balance(node_client, accounts)
            for balance in balances:
                # shoud be 438.000 TBD for all
                assert balance == '438.000 TBD'
            logger.info("Balances for treasury at time: {}".format(end_date_blocks_str))
            print_balance(node_client, [{'name' : args.treasury}])
            votes = list_proposals(node_client, start_date_str, "expired")
            for vote in votes:
                    # should be > 0 for all
                    assert vote > 0

            node.stop_steem_node()
            sys.exit(0)
        sys.exit(1)
    except Exception as ex:
        logger.error("Exception: {}".format(ex))
        node.stop_steem_node()
        sys.exit(1)
