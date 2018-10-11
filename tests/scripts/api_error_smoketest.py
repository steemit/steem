#!/usr/bin/env python3

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
      "params": ["temp","",10]
   },
   {
      "method": "condenser_api.get_witnesses",
      "params": [0]
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
   url = "https://api.steemitdev.com/"
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
