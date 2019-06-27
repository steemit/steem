#!/usr/bin/env python3
"""
  Usage: __name__ jobs url1 url2 [working_dir [accounts_file]]
    Example: script_name 4 http://127.0.0.1:8090 http://127.0.0.1:8091 [get_account_history [accounts]]
    set jobs to 0 if you want use all processors
    url1 is reference url for list_accounts
"""
import sys
import json
import os
import shutil
import re
import locale
from concurrent.futures import ThreadPoolExecutor
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures import Future
from concurrent.futures import wait
from jsonsocket import JSONSocket
from jsonsocket import steemd_call
from list_account import list_accounts
from pathlib import Path


wdir = Path()
errors = 0


def main():
  if len( sys.argv ) < 4 or len( sys.argv ) > 6:
    print( "Usage: __name__ jobs url1 url2 [working_dir [accounts_file]]" )
    print( "  Example: __name__ 4 http://127.0.0.1:8090 http://127.0.0.1:8091 [get_account_history [accounts]]" )
    print( "  set jobs to 0 if you want use all processors" )
    print( "  url1 is reference url for list_accounts" )
    exit ()

  global wdir
  global errors
    
  jobs = int(sys.argv[1])
  if jobs <= 0:
    import multiprocessing
    jobs = multiprocessing.cpu_count()
    
  url1 = sys.argv[2]
  url2 = sys.argv[3]
  
  if len( sys.argv ) > 4:
    wdir = Path(sys.argv[4])
    
  accounts_file = sys.argv[5] if len( sys.argv ) > 5 else ""

  if accounts_file != "":
    name = re.compile("[^\r^\n]*")
    try:
      with open(accounts_file, "rt") as file:
        accounts = [name.match(account).group(0) for account in file]
    except:
      exit("Cannot open file: " + accounts_file)
  else:
    accounts = list_accounts(url1)

  length = len(accounts)
  
  if length == 0:
    exit("There are no any account!")
    
  create_wdir()

  print( str(length) + " accounts" )

  if jobs > length:
    jobs = length

  print( "setup:" )
  print( "  jobs: {}".format(jobs) )
  print( "  url1: {}".format(url1) )
  print( "  url2: {}".format(url2) )
  print( "  wdir: {}".format(wdir) )
  print( "  accounts_file: {}".format(accounts_file) )
  
  if jobs > 1:
    first = 0
    last = length - 1
    accounts_per_job = length // jobs

    with ProcessPoolExecutor(max_workers=jobs) as executor:
      for i in range(jobs-1):
        executor.submit(compare_results, url1, url2, accounts[first : first+accounts_per_job-1])
        first = first + accounts_per_job
      executor.submit(compare_results, url1, url2, accounts[first : last])
  else:
    compare_results(url1, url2, accounts)
    
  exit( errors )

  
def create_wdir():
  global wdir
  
  if wdir.exists():
    if wdir.is_file():
      os.remove(wdir)
      
  if wdir.exists() == False:
    wdir.mkdir(parents=True)


def compare_results(url1, url2, accounts, max_tries=10, timeout=0.1):
  success = True
  print("Compare accounts: [{}..{}]".format(accounts[0], accounts[-1]))

  for account in accounts:
    if get_account_history(url1, url2, account, max_tries, timeout) == False:
      success = False; break

  print("Compare accounts: [{}..{}] {}".format(accounts[0], accounts[-1], "finished" if success else "break with error" ))


def get_account_history(url1, url2, account, max_tries=10, timeout=0.1):
  global wdir
  global errors
  START = -1
  HARD_LIMIT = 10000
  LIMIT = HARD_LIMIT

  while True:
    request = bytes( json.dumps( {
      "jsonrpc": "2.0",
      "id": 0,
      "method": "account_history_api.get_account_history",
      "params": { "account": account, "start": START, "limit": LIMIT }
      } ), "utf-8" ) + b"\r\n"

    with ThreadPoolExecutor(max_workers=2) as executor:
      future1 = executor.submit(steemd_call, url1, data=request, max_tries=max_tries, timeout=timeout)
      future2 = executor.submit(steemd_call, url2, data=request, max_tries=max_tries, timeout=timeout)

    status1, json1 = future1.result()
    status2, json2 = future2.result()
    #status1, json1 = steemd_call(url1, data=request, max_tries=max_tries, timeout=timeout)
    #status2, json2 = steemd_call(url2, data=request, max_tries=max_tries, timeout=timeout)
    
    if status1 == False or status2 == False or json1 != json2:
      print("Comparison failed for account: {}; start: {}; limit: {}".format(account, START, LIMIT))
      errors += 1

      filename = wdir / account
      try:    file = filename.open("w")
      except: print("Cannot open file:", filename); return False
      
      file.write("Comparison failed:\n")
      file.write("{} response:\n".format(url1))
      json.dump(json1, file, indent=2, sort_keys=True)
      file.write("\n")
      file.write("{} response:\n".format(url2))
      json.dump(json2, file, indent=2, sort_keys=True)
      file.write("\n")
      file.close()
      return False

    history = json1["result"]["history"]
    last = history[0][0] if len(history) else 0
    
    if last == 0: break

    last -= 1
    START = last
    LIMIT = last if last < HARD_LIMIT else HARD_LIMIT
  # while True
  
  return True


if __name__ == "__main__":
  main()