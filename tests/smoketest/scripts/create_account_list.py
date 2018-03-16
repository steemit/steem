#!/usr/bin/env python3
"""
Create list of all steem accounts in file.
Usage: create_account_list.py <server_address> <output_filename>
"""
import sys
import json
import requests

def main():
   if len( sys.argv ) != 3:
      exit( "Usage: create_account_list.py <server_address> <output_filename>" )

   url = sys.argv[1] + "/rpc"
   print( url )
   filename = sys.argv[2]

   try:     file = open( filename, "w" )
   except:  exit( "Cannot open file " + filename )

   headers = { 'content-type': 'application/json' }
   last_account = ""
   end = False
   accounts_count = 0

   while end == False:
      request = {
         "jsonrpc": "2.0",
         "id": 0,
         "method": "database_api.list_accounts",
         "params": { "start": last_account, "limit": 1000, "order": "by_name" }
         }

      try:
         response = requests.post( url, data=json.dumps(request), headers=headers).json()

         accounts = response["result"]["accounts"]
      except:
         print( "rpc failed for last_account: " + last_account )
         print( response )
         end = True
         continue

      if last_account != "":
         assert accounts[0]["name"] == last_account
         del accounts[0]

      if len( accounts ) == 0:
         end = True
         continue

      last_account = accounts[-1]["name"]
      accounts_count += len( accounts )
      for account in accounts:
         file.write( account["name"] + "\n" )

   # while end == False
   file.close()
   print( str(accounts_count) + " accounts")


if __name__ == "__main__":
   main()