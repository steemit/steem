#!/usr/bin/env python3

import sys
import requests
import json

tests = [
   {
      "method": "condenser_api.get_state",
      "params": ["trending"]
   },
   {
      "method": "condenser_api.get_state",
      "params": ["hot"]
   },
   {
      "method": "condenser_api.get_state",
      "params": ["/@temp"]
   },
   {
      "method": "condenser_api.get_trending_tags",
      "params": ["", 20]
   },
   {
      "method": "condenser_api.get_active_witnesses",
      "params": []
   },
   {
      "method": "condenser_api.get_block_header",
      "params": [10000]
   },
   {
      "method": "condenser_api.get_block",
      "params": [10000]
   },
   {
      "method": "condenser_api.get_ops_in_block",
      "params": [10000]
   },
   {
      "method": "condenser_api.get_ops_in_block",
      "params": [10000, True]
   },
   {
      "method": "condenser_api.get_config",
      "params": []
   },
   {
      "method": "condenser_api.get_dynamic_global_properties",
      "params": []
   },
   {
      "method": "condenser_api.get_chain_properties",
      "params": []
   },
   {
      "method": "condenser_api.get_feed_history",
      "params": []
   },
   {
      "method": "condenser_api.get_witness_schedule",
      "params": []
   },
   {
      "method": "condenser_api.get_hardfork_version",
      "params": []
   },
   {
      "method": "condenser_api.get_next_scheduled_hardfork",
      "params": []
   },
   {
      "method": "condenser_api.get_reward_fund",
      "params": ["post"]
   },
   {
      "method": "condenser_api.get_key_references",
      "params": [["STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX"]]
   },
   {
      "method": "condenser_api.get_accounts",
      "params": [["initminer","temp"]]
   },
   {
      "method": "condenser_api.lookup_account_names",
      "params": [["initminer","foobar","temp"]]
   },
   {
      "method": "condenser_api.lookup_accounts",
      "params": ["", 10]
   },
   {
      "method": "condenser_api.get_account_count",
      "params": []
   },
   {
      "method": "condenser_api.get_owner_history",
      "params": ["initminer"]
   },
   {
      "method": "condenser_api.get_recovery_request",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_escrow",
      "params": ["temp", 0]
   },
   {
      "method": "condenser_api.get_withdraw_routes",
      "params": ["temp", "incoming"]
   },
   {
      "method": "condenser_api.get_withdraw_routes",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_savings_withdraw_from",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_savings_withdraw_to",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_vesting_delegations",
      "params": ["temp","",10]
   },
   {
      "method": "condenser_api.get_expiring_vesting_delegations",
      "params": ["temp","2016-3-24T00:00:00",10]
   },
   {
      "method": "condenser_api.get_witnesses",
      "params": [[0,1]]
   },
   {
      "method": "condenser_api.get_conversion_requests",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_witness_by_account",
      "params": ["initminer"]
   },
   {
      "method": "condenser_api.get_witnesses_by_vote",
      "params": ["", 5]
   },
   {
      "method": "condenser_api.lookup_witness_accounts",
      "params": ["", 5]
   },
   {
      "method": "condenser_api.get_witness_count",
      "params": []
   },
   {
      "method": "condenser_api.get_open_orders",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_transaction_hex",
      "params": [{"operations":[["transfer", {"from":"temp", "to":"null", "ammount":"1.000 STEEM"}]]}]
   },
# Not currently tracking info required for this
#   {
#      "method": "condenser_api.get_transaction",
#      "params": ["46ddcba847f2297d13e32be07d72d15c530a7271"]
#   },
   {
      "method": "condenser_api.get_required_signatures",
      "params": [{"operations":[["transfer", {"from":"temp", "to":"null", "ammount":"1.000 STEEM"}]]}, []]
   },
   {
      "method": "condenser_api.verify_authority",
      "params": [{"operations":[["transfer", {"from":"temp", "to":"null", "ammount":"1.000 STEEM"}]]}]
   },
   {
      "method": "condenser_api.verify_account_authority",
      "params": ["temp",["STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX"]]
   },
   {
      "method": "condenser_api.get_active_votes",
      "params": ["temp", "test1"]
   },
   {
      "method": "condenser_api.get_account_votes",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_content",
      "params": ["temp", "test1"]
   },
   {
      "method": "condenser_api.get_content_replies",
      "params": ["temp", "foobar"]
   },
   {
      "method": "condenser_api.get_tags_used_by_author",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_comment_discussions_by_payout",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_trending",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_created",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_active",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_cashout",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_votes",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_children",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_hot",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_feed",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_blog",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_discussions_by_comments",
      "params": [{"tag":"test", "start_author":"temp"}]
   },
   {
      "method": "condenser_api.get_discussions_by_promoted",
      "params": [{"tag":"test"}]
   },
   {
      "method": "condenser_api.get_replies_by_last_update",
      "params": ["temp", "test1", 10]
   },
# Might be broken
#   {
#      "method": "condenser_api.get_discussions_by_author_before_date",
#      "params": ["temp", "test1", "1969-12-31T23:59:59", 10]
#   },
   {
      "method": "condenser_api.get_account_history",
      "params": ["temp", -1, 10]
   },
   {
      "method": "condenser_api.get_followers",
      "params": ["temp", "", "blog", 10]
   },
   {
      "method": "condenser_api.get_following",
      "params": ["temp", "", "blog", 10]
   },
   {
      "method": "condenser_api.get_follow_count",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_feed_entries",
      "params": ["temp", -1, 10]
   },
   {
      "method": "condenser_api.get_feed",
      "params": ["temp", -1, 10]
   },
   {
      "method": "condenser_api.get_blog_entries",
      "params": ["temp", -1, 10]
   },
   {
      "method": "condenser_api.get_blog",
      "params": ["temp", -1, 10]
   },
   {
      "method": "condenser_api.get_account_reputations",
      "params": ["", 10]
   },
# Might be broken
#   {
#      "method": "condenser_api.get_reblogged_by",
#      "params": ["temp", "test1"]
#   },
   {
      "method": "condenser_api.get_blog_authors",
      "params": ["temp"]
   },
   {
      "method": "condenser_api.get_ticker",
      "params": []
   },
   {
      "method": "condenser_api.get_volume",
      "params": []
   },
   {
      "method": "condenser_api.get_order_book",
      "params": [10]
   },
   {
      "method": "condenser_api.get_trade_history",
      "params": ["2016-3-24T00:00:00", "2017-3-24T00:00:00", 10]
   },
   {
      "method": "condenser_api.get_recent_trades",
      "params": [10]
   },
   {
      "method": "condenser_api.get_market_history",
      "params": [60, "2016-3-24T00:00:00", "2017-3-24T00:00:00", ]
   },
   {
      "method": "condenser_api.get_market_history_buckets",
      "params": []
   },
   {
      "method": "account_by_key_api.get_key_references",
      "params": {"keys":["STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX"]}
   },
   {
      "method": "account_history_api.get_ops_in_block",
      "params": {"block_num":10000}
   },
   {
      "method": "account_history_api.get_ops_in_block",
      "params": {"block_num":10000, "only_virtual":True}
   },
   {
      "method": "account_history_api.get_account_history",
      "params": {"account":"temp", "start":-1, "limit":10}
   },
   {
      "method": "account_history_api.enum_virtual_ops",
      "params": {"block_range_begin":1, "block_range_end":10}
   },
   {
      "method": "block_api.get_block_header",
      "params": {"block_num":10000}
   },
   {
      "method": "block_api.get_block",
      "params": {"block_num":10000}
   },
   {
      "method": "follow_api.get_followers",
      "params": {"account":"temp", "start":"", "type":"blog", "limit":10}
   },
   {
      "method": "follow_api.get_following",
      "params": {"account":"temp", "start":"", "type":"blog", "limit":10}
   },
   {
      "method": "follow_api.get_follow_count",
      "params": {"account":"temp"}
   },
   {
      "method": "follow_api.get_feed_entries",
      "params": {"account":"temp", "start":-1, "limit":10}
   },
   {
      "method": "follow_api.get_feed",
      "params": {"account":"temp", "start":-1, "limit":10}
   },
   {
      "method": "follow_api.get_blog_entries",
      "params": {"account":"temp", "start":-1, "limit":10}
   },
   {
      "method": "follow_api.get_blog",
      "params": {"account":"temp", "start":-1, "limit":10}
   },
   {
      "method": "follow_api.get_account_reputations",
      "params": {"lower_bound":"", "limit":10}
   },
# Might be broken
#   {
#      "method": "condenser_api.get_reblogged_by",
#      "params": ["temp", "test1"]
#   },
   {
      "method": "follow_api.get_blog_authors",
      "params": {"blog_account":"temp"}
   },
   {
      "method": "market_history_api.get_ticker",
      "params": {}
   },
   {
      "method": "market_history_api.get_volume",
      "params": {}
   },
   {
      "method": "market_history_api.get_order_book",
      "params": {"limit":10}
   },
   {
      "method": "market_history_api.get_trade_history",
      "params": {"start":"2016-3-24T00:00:00", "end":"2017-3-24T00:00:00", "limit":10}
   },
   {
      "method": "market_history_api.get_recent_trades",
      "params": {"limit":10}
   },
   {
      "method": "market_history_api.get_market_history",
      "params": {"bucket_seconds":60, "start":"2017-3-24T00:00:00", "end":"2017-3-24T01:00:00"}
   },
   {
      "method": "market_history_api.get_market_history_buckets",
      "params": {}
   },
   {
      "method": "rc_api.get_resource_params",
      "params": {}
   },
   {
      "method": "rc_api.get_resource_pool",
      "params": {}
   },
   {
      "method": "rc_api.find_rc_accounts",
      "params": {"accounts":["test"]}
   },
# Not currently enabled
#   {
#      "method": "reputation_api.get_account_reputations",
#      "params": {"lower_bound":"", "limit":10}
#   },
   {
      "method": "tags_api.get_trending_tags",
      "params": {"start_tag":"", "limit":20}
   },
   {
      "method": "tags_api.get_tags_used_by_author",
      "params": {"author":"temp"}
   },
   {
      "method": "tags_api.get_discussion",
      "params": {"author":"temp", "permlink":"test1"}
   },
   {
      "method": "tags_api.get_content_replies",
      "params": {"author":"temp", "permlink":"foobar"}
   },
   {
      "method": "tags_api.get_post_discussions_by_payout",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_comment_discussions_by_payout",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_trending",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_created",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_active",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_cashout",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_votes",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_children",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_hot",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_feed",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_blog",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_discussions_by_comments",
      "params": {"tag":"test", "start_author":"temp"}
   },
   {
      "method": "tags_api.get_discussions_by_promoted",
      "params": {"tag":"test"}
   },
   {
      "method": "tags_api.get_replies_by_last_update",
      "params": {"start_parent_author":"temp", "start_permlink":"test1", "limit":10}
   },
# Might be broken
#   {
#      "method": "tags_api.get_discussions_by_author_before_date",
#      "params": {"author":"temp", "start_permlink":"test1", "before_date":"1969-12-31T23:59:59", "limit":10}
#   },
   {
      "method": "tags_api.get_active_votes",
      "params": {"author":"temp", "permlink":"test1"}
   },
   {
      "method": "database_api.get_config",
      "params": {}
   },
   {
      "method": "database_api.get_version",
      "params": {}
   },
   {
      "method": "database_api.get_dynamic_global_properties",
      "params": {}
   },
   {
      "method": "database_api.get_witness_schedule",
      "params": {}
   },
   {
      "method": "database_api.get_hardfork_properties",
      "params": {}
   },
   {
      "method": "database_api.get_reward_funds",
      "params": {}
   },
   {
      "method": "database_api.get_current_price_feed",
      "params": {}
   },
   {
      "method": "database_api.get_feed_history",
      "params": {}
   },
   {
      "method": "database_api.list_witnesses",
      "params": {"start":"", "limit":10, "order":"by_name"}
   },
   {
      "method": "database_api.find_witnesses",
      "params": {"owners":["initminer"]}
   },
   {
      "method": "database_api.list_witness_votes",
      "params": {"start":["",""], "limit":10, "order":"by_account_witness"}
   },
   {
      "method": "database_api.get_active_witnesses",
      "params": {}
   },
   {
      "method": "database_api.list_accounts",
      "params": {"start":"", "limit":10, "order":"by_name"}
   },
   {
      "method": "database_api.find_accounts",
      "params": {"accounts":["temp","null"]}
   },
   {
      "method": "database_api.list_owner_histories",
      "params": {"start":["","1970-01-01T00:00:00"], "limit":10}
   },
   {
      "method": "database_api.find_owner_histories",
      "params": {"owner":"temp"}
   },
   {
      "method": "database_api.list_account_recovery_requests",
      "params": {"start":"", "limit":10, "order":"by_account"}
   },
   {
      "method": "database_api.find_account_recovery_requests",
      "params": {"accounts":["temp","null"]}
   },
   {
      "method": "database_api.list_change_recovery_account_requests",
      "params": {"start":["1960-01-01T00:00:00",""], "limit":10, "order":"by_effective_date"}
   },
   {
      "method": "database_api.find_change_recovery_account_requests",
      "params": {"accounts":["temp","null"]}
   },
   {
      "method": "database_api.list_escrows",
      "params": {"start":["",0], "limit":10, "order":"by_from_id"}
   },
   {
      "method": "database_api.find_escrows",
      "params": {"from": "temp"}
   },
   {
      "method": "database_api.list_withdraw_vesting_routes",
      "params": {"start":["temp",""], "limit":10, "order":"by_withdraw_route"}
   },
   {
      "method": "database_api.find_withdraw_vesting_routes",
      "params": {"account":"temp", "order":"by_destination"}
   },
   {
      "method": "database_api.list_savings_withdrawals",
      "params": {"start":["",0], "limit":10, "order":"by_from_id"}
   },
   {
      "method": "database_api.find_savings_withdrawals",
      "params": {"start":"temp"}
   },
   {
      "method": "database_api.list_vesting_delegations",
      "params": {"start":["",""], "limit":10, "order":"by_delegation"}
   },
   {
      "method": "database_api.find_vesting_delegations",
      "params": {"account":"temp"}
   },
   {
      "method": "database_api.list_vesting_delegation_expirations",
      "params": {"start":["1970-01-01T00:00:00",0], "limit":10, "order":"by_expiration"}
   },
   {
      "method": "database_api.find_vesting_delegation_expirations",
      "params": {"account":"temp"}
   },
   {
      "method": "database_api.list_sbd_conversion_requests",
      "params": {"start":["",0], "limit":10, "order":"by_account"}
   },
   {
      "method": "database_api.find_sbd_conversion_requests",
      "params": {"account":"temp"}
   },
   {
      "method": "database_api.list_decline_voting_rights_requests",
      "params": {"start":"", "limit":10, "order":"by_account"}
   },
   {
      "method": "database_api.find_decline_voting_rights_requests",
      "params": {"accounts":["temp","null"]}
   },
   {
      "method": "database_api.list_comments",
      "params": {"start":("1970-01-01T00:00:00","",""), "limit":10, "order":"by_cashout_time"}
   },
   {
      "method": "database_api.find_comments",
      "params": {"start":[["temp","test1"],["temp","foobar"]], "limit":10, "order":"by_account"}
   },
   {
      "method": "database_api.list_votes",
      "params": {"start":["","",""], "limit":10, "order":"by_comment_voter"}
   },
   {
      "method": "database_api.find_votes",
      "params": {"author":"temp", "permlink":"test1"}
   },
   {
      "method": "database_api.list_limit_orders",
      "params": {"start":[{"base":{"amount":1000,"precision":3,"nai":"@@000000021"},"quote":{"amount":1,"precision":3,"nai":"@@000000013"}},0], "limit":10, "order":"by_price"}
   },
   {
      "method": "database_api.find_limit_orders",
      "params": {"account":"temp"}
   },
   {
      "method": "database_api.get_order_book",
      "params": {"limit":10}
   },
   {
      "method": "database_api.get_transaction_hex",
      "params": {"trx":{"operations":[{"type":"transfer_operation", "value":{"from":"temp", "to":"null", "ammount":{"base":{"amount":1000,"precision":3,"nai":"@@000000021"}}}}]}}
   },
   {
      "method": "database_api.get_required_signatures",
      "params": {"trx":{"operations":[{"type":"transfer_operation", "value":{"from":"temp", "to":"null", "ammount":{"base":{"amount":1000,"precision":3,"nai":"@@000000021"}}}}]}, "available_keys":[]}
   },
   {
      "method": "database_api.verify_authority",
      "params": {"trx":{"operations":[{"type":"transfer_operation", "value":{"from":"temp", "to":"null", "ammount":{"base":{"amount":1000,"precision":3,"nai":"@@000000021"}}}}]}}
   },
   {
      "method": "database_api.verify_signatures",
      "params": {"required_active":["temp"]}
   },
   {
      "method": "database_api.list_smt_token_emissions",
      "params": {"limit": 100, "order": "by_symbol_time", "start": []}
   },
   {  "method": "database_api.find_smt_token_emissions",
      "params": {"asset_symbol": {"nai":"@@100000006", "decimals": 3}}
   }
]

def test_api( url, headers, payload ):
   response = requests.post( url, data=json.dumps(payload), headers=headers).json()

   try:
      if( response["id"] != payload["id"]
         or response["jsonrpc"] != "2.0" ):
         return False
      response["result"]
   except:
      return False

   try:
      response["error"]
      return False
   except KeyError:
      return True
   except:
      return False

   return True

def main():
   if len( sys.argv ) == 1:
      url = "https://api.steemit.com/"
   elif len( sys.argv ) == 2:
      url = sys.argv[1]
   else:
      exit( "Usage: api_error_smoketest.py <steem_api_endpoint>" )

   print( "Testing against endpoint: " + url )

   headers = {'content-type': 'application/json'}

   payload = {
      "jsonrpc": "2.0"
   }

   id = 0
   errors = 0

   for testcase in tests:
      payload["method"] = testcase["method"]
      payload["params"] = testcase["params"]
      payload["id"] = id

      if( not test_api( url, headers, payload ) ):
         errors += 1
         print( "Error in testcase: " + json.dumps( payload ) )

      id += 1

   print( str( errors ) + " error(s) found." )

   if( errors > 0 ):
      return 1

   return 0

if __name__ == "__main__":
   main()
