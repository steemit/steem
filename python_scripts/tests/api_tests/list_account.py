#!/usr/bin/env python3
"""
Create list of all steem accounts in file.
Usage: create_account_list.py <server_address> [<output_filename>]
"""
import sys
import json
from jsonsocket import JSONSocket
from jsonsocket import steemd_call

def list_accounts(url):
  """
  url in form <ip_address>:<port>
  """
  last_account = ""
  end = False
  accounts_count = 0
  accounts = []
  
  while end == False:
    request = bytes( json.dumps( {
      "jsonrpc": "2.0",
      "id": 0,
      "method": "database_api.list_accounts",
      "params": { "start": last_account, "limit": 1000, "order": "by_name" }
      } ), "utf-8" ) + b"\r\n"

    status, response = steemd_call(url, data=request)
    
    if status == False:
      print( "rpc failed for last_account: " + last_account )
      return []

    account_list = response["result"]["accounts"]
        
    if last_account != "":
      assert account_list[0]["name"] == last_account
      del account_list[0]

    if len( account_list ) == 0:
      end = True
      continue

    last_account = account_list[-1]["name"]
    accounts_count += len( accounts )
    for account in account_list:
      accounts.append( account["name"] )

  # while end == False
  return accounts
  

def main():
  if len( sys.argv ) < 2 or len( sys.argv ) > 3:
    exit( "Usage: create_account_list.py <server_address> [<output_filename>]" )

  url = sys.argv[1]
  print( url )

  accounts = list_accounts( url )
  
  if len(accounts) == 0:
    exit(-1)
   
  if len( sys.argv ) == 3:
    filename = sys.argv[2]

    try:     file = open( filename, "w" )
    except:  exit( "Cannot open file " + filename )
    
    for account in accounts:
      file.write(account + "\n")
   
    file.close()
    

if __name__ == "__main__":
  main()