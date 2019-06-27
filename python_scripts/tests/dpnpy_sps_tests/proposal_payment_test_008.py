#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys
import dpn_utils.dpn_runner
import dpn_utils.dpn_tools
import test_utils


LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_proposal_payment_008.log"

MODULE_NAME = "SPS-Tester-via-dpnpy"
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
    from dpn import Dpn
except Exception as ex:
    logger.error("DpnPy library is not installed.")
    sys.exit(1)

# Circular payment
# 1. create few proposals - in this scenario proposals have the same starting and ending dates
#    one of the proposals will by paying to the treasury
# 2. vote on them to show differences in asset distribution (depending on collected votes)
# 3. wait for proposal payment phase
# 4. verify (using account history and by checking regular account balance) that given accounts have been correctly paid.

# Expected result: all got paid.

def vote_proposals(node, accounts):
    logger.info("Voting proposals...")
    for acnt in accounts:
        proposal_set = [0,1,2,3,4]
        logger.info("Account {} voted for proposals: {}".format(acnt["name"], ",".join(str(x) for x in proposal_set)))
        node.commit.update_proposal_votes(acnt["name"], proposal_set, True)
    dpn_utils.dpn_tools.wait_for_blocks_produced(5, node.url)

if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create test accounts with")
    parser.add_argument("treasury", help = "Treasury account")
    parser.add_argument("wif", help="Private key for creator account")
    parser.add_argument("--node-url", dest="node_url", default="http://127.0.0.1:8090", help="Url of working dpn node")
    parser.add_argument("--run-dpnd", dest="dpnd_path", help = "Path to dpnd executable. Warning: using this option will erase contents of selected dpnd working directory.")
    parser.add_argument("--working_dir", dest="dpnd_working_dir", default="/tmp/dpnd-data/", help = "Path to dpnd working directory")
    parser.add_argument("--config_path", dest="dpnd_config_path", default="./dpn_utils/resources/config.ini.in",help = "Path to source config.ini file")
    parser.add_argument("--no-erase-proposal", action='store_false', dest = "no_erase_proposal", help = "Do not erase proposal created with this test")


    args = parser.parse_args()

    node = None

    if args.dpnd_path:
        logger.info("Running dpnd via {} in {} with config {}".format(args.dpnd_path,
            args.dpnd_working_dir,
            args.dpnd_config_path)
        )
        
        node = dpn_utils.dpn_runner.DpnNode(
            args.dpnd_path,
            args.dpnd_working_dir,
            args.dpnd_config_path
        )
    
    node_url = args.node_url
    wif = args.wif

    if len(wif) == 0:
        logger.error("Private-key is not set in config.ini")
        sys.exit(1)

    logger.info("Using node at: {}".format(node_url))
    logger.info("Using private-key: {}".format(wif))

    accounts = [
        # place accounts here in the format: {'name' : name, 'private_key' : private-key, 'public_key' : public-key}
        ]

    if not accounts:
        logger.error("Accounts array is empty, please add accounts in a form {\"name\" : name, \"private_key\" : private_key, \"public_key\" : public_key}")
        sys.exit(1)

    keys = [wif]
    for account in accounts:
        keys.append(account["private_key"])
    
    if node is not None:
        node.run_dpn_node(["--enable-stale-production"])
    try:
        if node is None or node.is_running():
            node_client = Dpn(nodes = [node_url], no_broadcast = False,
                keys = keys
            )

            # create accounts
            test_utils.create_accounts(node_client, args.creator, accounts)
            # tranfer to vesting
            test_utils.transfer_to_vesting(node_client, args.creator, accounts, "300.000", 
                "TESTS"
            )
            # transfer assets to accounts
            test_utils.transfer_assets_to_accounts(node_client, args.creator, accounts, 
                "400.000", "TESTS"
            )

            test_utils.transfer_assets_to_accounts(node_client, args.creator, accounts, 
                "400.000", "TBD"
            )

            logger.info("Balances for accounts after initial transfer")
            test_utils.print_balance(node_client, accounts)
            # transfer assets to treasury
            test_utils.transfer_assets_to_treasury(node_client, args.creator, args.treasury, 
                "999950.000", "TESTS"
            )

            test_utils.transfer_assets_to_treasury(node_client, args.creator, args.treasury, 
                "999950.000", "TBD"
            )
            test_utils.print_balance(node_client, [{'name' : args.treasury}])

            # create post for valid permlinks
            test_utils.create_posts(node_client, accounts)

            now = node_client.get_dynamic_global_properties().get('time', None)
            if now is None:
                raise ValueError("Head time is None")
            now = test_utils.date_from_iso(now)

            proposal_data = [
                ['tester001', 1 + 0, 4, '24.000 TBD'], # starts 1 day from now and lasts 3 day
                ['tester002', 1 + 0, 4, '24.000 TBD'], # starts 1 days from now and lasts 3 day
                ['tester003', 1 + 0, 4, '24.000 TBD'],  # starts 1 days from now and lasts 3 day
                ['tester004', 1 + 0, 4, '24.000 TBD'], # starts 1 day from now and lasts 3 days
            ]

            proposals = [
                # pace proposals here in the format: {'creator' : creator, 'receiver' : receiver, 'start_date' : start-date, 'end_date' : end_date}
                
            ]

            for pd in proposal_data:
                start_date, end_date = test_utils.get_start_and_end_date(now, pd[1], pd[2])
                proposal = {'creator' : pd[0], 'receiver' : pd[0], 'start_date' : start_date, 'end_date' : end_date, 'daily_pay' : pd[3]}
                proposals.append(proposal)

            start_date, end_date = test_utils.get_start_and_end_date(now, 1, 4)
            proposals.append({'creator' : 'tester001', 'receiver' : 'dpn.dao', 'start_date' : start_date, 'end_date' : end_date, 'daily_pay' : '96.000 TBD'})

            import datetime
            test_start_date = now + datetime.timedelta(days = 1)
            test_start_date_iso = test_utils.date_to_iso(test_start_date)

            test_mid_date = test_start_date + datetime.timedelta(days = 3, hours = 1)

            test_end_date = test_start_date + datetime.timedelta(days = 5, hours = 1)
            test_end_date_iso = test_utils.date_to_iso(test_end_date)

            test_utils.create_proposals(node_client, proposals)

            # list proposals with inactive status, it shoud be list of pairs id:total_votes
            test_utils.list_proposals(node_client, test_start_date_iso, "inactive")

            # each account is voting on proposal
            vote_proposals(node_client, accounts)

            # list proposals with inactive status, it shoud be list of pairs id:total_votes
            votes = test_utils.list_proposals(node_client, test_start_date_iso, "inactive")
            for vote in votes:
                #should be 0 for all
                assert vote == 0, "All votes should be equal to 0"

            logger.info("Balances for accounts after creating proposals")
            test_balances = [
                '380.000 TBD',
                '390.000 TBD',
                '390.000 TBD',
                '390.000 TBD',
            ]
            balances = test_utils.print_balance(node_client, accounts)
            for idx in range(0, len(test_balances)):
                assert balances[idx] == test_balances[idx],  "Balances dont match {} != {}".format(balances[idx], test_balances[idx])
            test_utils.print_balance(node_client, [{'name' : args.treasury}])

            # move forward in time to see if proposals are paid
            # moving is made in 1h increments at a time, after each 
            # increment balance is printed
            logger.info("Moving to date: {}".format(test_start_date_iso))
            node_client.debug_generate_blocks_until(wif, test_start_date_iso, False)
            current_date = test_start_date
            while current_date < test_end_date:
                current_date = current_date + datetime.timedelta(hours = 1)
                current_date_iso = test_utils.date_to_iso(current_date)

                logger.info("Moving to date: {}".format(current_date_iso))
                node_client.debug_generate_blocks_until(wif, current_date_iso, False)

                logger.info("Balances for accounts at time: {}".format(current_date_iso))
                test_utils.print_balance(node_client, accounts)
                test_utils.print_balance(node_client, [{'name' : args.treasury}])

                votes = test_utils.list_proposals(node_client, test_start_date_iso, "active")
                votes = test_utils.list_proposals(node_client, test_start_date_iso, "expired")
                votes = test_utils.list_proposals(node_client, test_start_date_iso, "all")

            # move additional hour to ensure that all proposals ended
            logger.info("Moving to date: {}".format(test_end_date_iso))
            node_client.debug_generate_blocks_until(wif, test_end_date_iso, False)
            logger.info("Balances for accounts at time: {}".format(test_end_date_iso))
            balances = test_utils.print_balance(node_client, accounts)
            # it should be '476.000 TBD', '486.000 TBD', '486.000 TBD', '486.000 TBD',
            # but because of rounding implementation it is 475.808 TBD,485.808 TBD,485.808 TBD,485.808 TBD
            test_balances = [
                '475.808 TBD',
                '485.808 TBD',
                '485.808 TBD',
                '485.808 TBD',
            ]
            for idx in range(0, len(test_balances)):
                assert balances[idx] == test_balances[idx], "Balances dont match {} != {}".format(balances[idx], test_balances[idx])

            test_utils.print_balance(node_client, [{'name' : args.treasury}])

            if node is not None:
                node.stop_dpn_node()
            sys.exit(0)
        sys.exit(1)
    except Exception as ex:
        logger.error("Exception: {}".format(ex))
        if node is not None: 
            node.stop_dpn_node()
        sys.exit(1)
