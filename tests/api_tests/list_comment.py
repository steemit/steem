#!/usr/bin/env python3
"""
Create list of all steem comments in file.
Usage: list_comment.py <server_address> [<output_filename>]
"""
import sys
import json
from jsonsocket import JSONSocket
from jsonsocket import steemd_call

def list_comments(url):
  """
  url in form <ip_address>:<port>
  """
  last_cashout_time = "2016-01-01T00-00-00"
  end = False
  comments = []
  
  while True:
    request = bytes( json.dumps( {
      "jsonrpc": "2.0",
      "id": 0,
      "method": "database_api.list_comments",
      "params": { "start":[ last_cashout_time, "", "" ], "limit": 5, "order": "by_cashout_time" }
      } ), "utf-8" ) + b"\r\n"

    status, response = steemd_call(url, data=request)
    
    if status == False:
      print( "rpc failed for last_cashout_time: " + last_cashout_time )
      return []

    comment_list = response["result"]["comments"]

    if len(comment_list) == 0:
       break

    actual_cashout_time = comment_list[-1]["cashout_time"]

    if actual_cashout_time == last_cashout_time:
      break

    last_cashout_time = actual_cashout_time

    for comment in comment_list:
      comments.append( comment["permlink"]+";"+comment["author"] +";"+comment["last_update"] )

  # while True
  return comments
  

def main():
  if len( sys.argv ) < 2 or len( sys.argv ) > 3:
    exit( "Usage: list_comment.py <server_address> [<output_filename>]" )

  url = sys.argv[1]
  print( url )

  comments = list_comments( url )
  
  if len(comments) == 0:
    exit(-1)
   
  if len( sys.argv ) == 3:
    filename = sys.argv[2]

    try:     file = open( filename, "w" )
    except:  exit( "Cannot open file " + filename )
    
    for comment in comments:
      file.write(comment + "\n")
   
    file.close()
    

if __name__ == "__main__":
  main()