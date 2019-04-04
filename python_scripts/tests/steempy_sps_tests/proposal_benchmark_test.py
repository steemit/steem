#!/usr/bin/python3

#example A:
#"/home/a/steemd" Path to steemd exe
# "/home/a/data"  Path to blockchain directory
# "./steem_utils/resources/config.ini.in" Path to config.ini.in - usually ./steem_utils/resources/config.ini.in
#  1 - number of proposals for every account
#  200 - number of accounts
#  200000.000- number of STEEM's for every account
#  30000.000- number of SBD's for every account
#  100000.000- number of VEST's for every account
# finally is created 200(1*200) proposals and 200 votes for every proposals -> 40200 objects are created
#  ./proposal_benchmark_test.py "/home/a/steemd" "/home/a/data" "./steem_utils/resources/config.ini.in" initminer 1 200 200000.000 30000.000 100000.000

#example B:
#"/home/a/steemd" Path to steemd exe
# "/home/a/data"  Path to blockchain directory
# "./steem_utils/resources/config.ini.in" Path to config.ini.in - usually ./steem_utils/resources/config.ini.in
#  2 - number of proposals for every account
#  300 - number of accounts
#  200000.000- number of STEEM's for every account
#  30000.000- number of SBD's for every account
#  100000.000- number of VEST's for every account
# finally is created 600(2*300) proposals and 300 votes for every proposals -> 180600 objects are created
#  ./proposal_benchmark_test.py "/home/a/steemd" "/home/a/data" "./steem_utils/resources/config.ini.in" initminer 2 300 200000.000 30000.000 100000.000

#Time[ms] is saved in `r_advanced_benchmark.json` ( position{"op_name": "sps_processor"} ) in directory where this script is called

from uuid import uuid4
from time import sleep
import logging
import sys
import steem_utils.steem_runner
import steem_utils.steem_tools
import dateutil.parser

data_size = 100
delayed_blocks = 1
delayed_blocks_ex = 3
SPS_API_SINGLE_QUERY_LIMIT = 1000
STEEM_PROPOSAL_MAX_IDS_NUMBER = 5

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
            asset='TESTS')
    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)


# transfer_to_vesting initminer pychol "310.000 TESTS" true
def transfer_to_vesting(node, from_account, accounts, vests ):
    for acnt in accounts:
        logger.info("Transfer to vesting from {} to {} amount {} {}".format(from_account, acnt['name'], vests, "TESTS"))
        node.commit.transfer_to_vesting(vests, to = acnt['name'], 
            account = from_account, asset='TESTS')
    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)


# transfer initminer pychol "399.000 TESTS" "initial transfer" true
# transfer initminer pychol "398.000 TBD" "initial transfer" true
def transfer_assets_to_accounts(node, from_account, accounts, steems, sbds):
    for acnt in accounts:
        logger.info("Transfer from {} to {} amount {} {}".format(from_account, acnt['name'], steems, "TESTS"))
        node.commit.transfer(acnt['name'], steems, "TESTS", memo = "initial transfer", account = from_account)
    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)
    for acnt in accounts:
        logger.info("Transfer from {} to {} amount {} {}".format(from_account, acnt['name'], sbds, "TBD"))
        node.commit.transfer(acnt['name'], sbds, "TBD", memo = "initial transfer", account = from_account)
    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)

def create_permlink( node, account ):
   return "steempy-proposal-title-{}".format( account )

def create_posts(node, accounts):
    logger.info("Creating posts...")
    i = 0
    for acnt in accounts:
        node.commit.post("Steempy proposal title [{}]".format(acnt['name']), 
            "Steempy proposal body [{}]".format(acnt['name']), 
            acnt['name'], 
            permlink = create_permlink( node, acnt['name'] ), 
            tags = "proposals")
        i += 1
        if ( i % data_size ) == 0:
            steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks_ex, node.url)
            print_progress( i, len( accounts ) )

    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)

def generate_dates( node ):
    logger.info("Generating dates...")
    import datetime

    block_time = get_block_time( node )
    now = dateutil.parser.parse( block_time )

    start_date = now + datetime.timedelta(hours = 2)
    end_date = start_date + datetime.timedelta(hours = 1)
    #because of STEEM_PROPOSAL_MAINTENANCE_CLEANUP
    end_date_delay = end_date + datetime.timedelta(days = 1)

    start_date = start_date.replace(microsecond=0).isoformat()
    end_date = end_date.replace(microsecond=0).isoformat()
    end_date_delay = end_date_delay.replace(microsecond=0).isoformat()

    return start_date, end_date, end_date_delay

def print_progress( i, count ):
    sys.stdout.write('\r')
    sys.stdout.write( " === %d from %d === " % ( i, count ) )
    sys.stdout.flush()
    print("")

def create_proposals(node, accounts, start_date, end_date, nr_proposals):
    logger.info("Creating proposals...")

    i = 0
    for cnt in range( nr_proposals ):
        for acnt in accounts:
            ret = node.commit.create_proposal(
            acnt["name"],
            acnt["name"],
            start_date,
            end_date,
            "16.000 TBD",
            "subject: %d" % (cnt),
            create_permlink( node, acnt["name"] )
            )
            i += 1
            if ( i % data_size ) == 0:
                steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks_ex, node.url)
                print_progress( i, nr_proposals * len( accounts ) )

    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)

def list_proposals(node, ids, limit ):
    logger.info("Listing proposals...")

    cnt = 0;
    start = ""
    last_id = None
    while cnt < limit:
        nr_proposals = ( limit - cnt if limit - cnt <= SPS_API_SINGLE_QUERY_LIMIT else SPS_API_SINGLE_QUERY_LIMIT )
        proposals = node.list_proposals( start, "by_creator", "direction_ascending", nr_proposals, "all", last_id )
        for proposal in proposals:
            ids.append( proposal["id"] )

        if len( proposals ) == 0:
            break;
        cnt += len( proposals )
        last_proposal = proposals[ len( proposals ) - 1 ]
        start = last_proposal["creator"]
        last_id = last_proposal["id"]

def list_voter_proposals(node, limit ):
    logger.info("Listing voter proposals...")

    res = 0
    cnt = 0;
    start = ""
    while cnt < limit:
        nr_proposals = ( limit - cnt if limit - cnt <= SPS_API_SINGLE_QUERY_LIMIT else SPS_API_SINGLE_QUERY_LIMIT )
        voter_proposals = node.list_voter_proposals( start, "by_creator", "direction_ascending", nr_proposals, "all" )
        res += len( voter_proposals )

        if len( voter_proposals ) == 0:
            break;
        cnt += len( voter_proposals )
        start = voter_proposals[ len( voter_proposals ) - 1 ]["creator"]
    return res

def vote_proposals(node, ids, accounts ):
    logger.info("Voting proposals for %i proposals..." % ( len( ids ) ) )

    proposal_limit = STEEM_PROPOSAL_MAX_IDS_NUMBER

    i = 0
    i_real = 0
    cnt = 0
    array_length = len( ids )
    proposal_set = []

    for id in ids:
        proposal_set.append( id )
        cnt += 1
        if len( proposal_set ) == proposal_limit or cnt == array_length:
            for acnt in accounts:
                ret = node.commit.update_proposal_votes( acnt["name"], proposal_set, True)
                i += 1
                i_real += len( proposal_set )
                if ( i % data_size ) == 0:
                    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)
                    print_progress( i_real, len( ids ) * len( accounts ) )
            proposal_set = []

    steem_utils.steem_tools.wait_for_blocks_produced(delayed_blocks, node.url)

def generate_blocks( node, time, wif ):
    logger.info("Generating blocks ...")

    ret = node.debug_generate_blocks_until( wif, time, False )
    print( "Moved %s blocks" % ( ret["blocks"] ) )
    node.debug_generate_blocks( wif, delayed_blocks_ex )

def get_block_time( node ):
    global_properties = steem_utils.steem_tools.get_dynamic_global_properties( node.url )

    last_block_number = global_properties.get('result', None)
    return last_block_number[ "time" ]

def get_info( node, message, time ):
    block_time = get_block_time( node )

    print( "%s:" % ( message ) )
    print( "block_time: %s" % ( block_time ) )
    print( "end time  : %s" % ( time ) )

if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=str, help = "Path to steemd")
    parser.add_argument("dir", type=str, help = "Path to blockchain directory")
    parser.add_argument("ini", type=str, help = "Path to config.ini.in - usually ./steem_utils/resources/config.ini.in")
    parser.add_argument("creator", help = "Account to create test accounts with")
    parser.add_argument("nr_proposals", type=int, help = "Nr proposals for every account")
    parser.add_argument("nr_accounts", type=int, help = "Nr accounts")
    parser.add_argument("steems", type=str, default="200000.000", help = "STEEM's")
    parser.add_argument("sbds", type=str, default="30000.000", help = "SBD's")
    parser.add_argument("vests", type=str, default="100000.000", help = "VEST's")

    args = parser.parse_args()

    node = steem_utils.steem_runner.SteemNode( args.path, args.dir, args.ini )
    node_url = node.get_node_url()
    wif = node.get_from_config('private-key')[0]

    logger.info("Using node at: {}".format(node_url))
    logger.info("Using private-key: {}".format(wif))

    accounts = []
    for i in range( args.nr_accounts ):
        accounts.append( {"name" : "", "private_key" : "5JBuekd1sVXXK3wBu6nvPB1LWypZ83BYdu7tGcUNYVd42xQGGh1", "public_key" : "TST5kSj1yTzBz3PDoJ5QUyVVagdgYfs8Y4vVsZG3dqKJU8hg7WmQN"} )
        accounts[ len( accounts ) - 1 ]["name"] = "tester" + str( i )

    keys = [wif]
    #for account in accounts:
    keys.append(accounts[0]["private_key"])
    
    node.run_steem_node(["--enable-stale-production"])
    try:
        if node.is_running():
            node_client = Steem(nodes = [node_url], no_broadcast = False, keys = keys)

            create_accounts(node_client, args.creator, accounts)
            transfer_to_vesting(node_client, args.creator, accounts, args.vests )
            transfer_assets_to_accounts(node_client, args.creator, accounts, args.steems, args.sbds )
            create_posts(node_client, accounts)

            #total proposals: nr_proposals * accounts
            start_date, end_date, end_date_delay = generate_dates( node_client )
            create_proposals( node_client, accounts, start_date, end_date, args.nr_proposals )

            total_proposals = args.nr_proposals * len(accounts)

            ids = []
            list_proposals( node_client, ids, total_proposals )
            total_proposals = len( ids )

            #total votes: total proposals * accounts
            vote_proposals(node_client, ids, accounts )

            print( "%s items is ready to be removed" % ( total_proposals * len( accounts ) + total_proposals ) )

            get_info( node_client, "before movement", end_date_delay )
            generate_blocks( node_client, end_date_delay, wif )
            get_info( node_client, "after movement", end_date_delay )

            ids = []
            total_votes = total_proposals * len( accounts )
            list_proposals( node_client, ids, total_proposals )
            voter_proposals_number = list_voter_proposals( node_client, total_votes )

            if len( ids ) == 0 and voter_proposals_number == 0:
                print( "***All proposals/votes were removed - test passed***" )
            else:
                print( "***There are %i proposals/votes. All proposals should be removed - test failed***" % ( len( ids ) + total_votes ) )

            node.stop_steem_node()
            sys.exit(0)
        sys.exit(1)
    except Exception as ex:
        logger.error("Exception: {}".format(ex))
        node.stop_steem_node()
        sys.exit(1)
