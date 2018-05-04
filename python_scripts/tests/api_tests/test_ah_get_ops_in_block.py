#!/usr/bin/env python3
"""
  Usage: script_name jobs url1 url2 [wdir [last_block [first_block]]]
    Example: script_name 4 http://127.0.0.1:8090 http://127.0.0.1:8091 ./ 5000000 0
    set jobs to 0 if you want use all processors
    if last_block == 0, it is read from url1 (as reference)
"""

import sys
import json
import os
import shutil
from jsonsocket import JSONSocket
from jsonsocket import steemd_call
from concurrent.futures import ThreadPoolExecutor
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures import Future
from concurrent.futures import wait
from pathlib import Path


wdir = Path()
errors = 0


def main():
  if len(sys.argv) < 4 or len(sys.argv) > 7:
    print("Usage: script_name jobs url1 url2 [wdir [last_block [first_block]]]")
    print("  Example: script_name 4 http://127.0.0.1:8090 http://127.0.0.1:8091 ./ 5000000 0")
    print( "  set jobs to 0 if you want use all processors" )
    print("  if last_block == 0, it is read from url1 (as reference)")
    exit()
    
  global wdir
  global errors
  first_block = 0
  last_block = 0

  jobs = int(sys.argv[1])
  if jobs <= 0:
    import multiprocessing
    jobs = multiprocessing.cpu_count()
    
  url1 = sys.argv[2]
  url2 = sys.argv[3]
  
  if len(sys.argv) > 4:
    wdir = Path(sys.argv[4])
    
    if len(sys.argv) > 5:
      last_block = int(sys.argv[5])
    else:
      last_block = 0
    if len(sys.argv) == 7:
      first_block = int(sys.argv[6])
    else:
      first_block = 0
    
  last_block1 = get_last_block(url1)
  last_block2 = get_last_block(url2)
  
  if last_block1 != last_block2:
    exit("last block of {} ({}) is different then last block of {} ({})".format(url1, last_block1, url2, last_block2))

  if last_block == 0:
    last_block = last_block1
  elif last_block != last_block1:
    print("WARNING: last block from cmdline {} is different then from {} ({})".format(last_block, url1, last_block1))
  
  if last_block == 0:
    exit("last block cannot be 0!")
    
  create_wdir()

  blocks = last_block - first_block + 1

  if jobs > blocks:
    jobs = blocks

  print("setup:")
  print("  jobs: {}".format(jobs))
  print("  url1: {}".format(url1))
  print("  url2: {}".format(url2))
  print("  wdir: {}".format(wdir))
  print("  block range: {}:{}".format(first_block, last_block))

  if jobs > 1:
    blocks_per_job = blocks // jobs

    with ProcessPoolExecutor(max_workers=jobs) as executor:
      for i in range(jobs-1):
        executor.submit(compare_results, first_block, (first_block + blocks_per_job - 1), url1, url2)
        first_block = first_block + blocks_per_job
      executor.submit(compare_results, first_block, last_block, url1, url2)
  else:
    compare_results(first_block, last_block, url1, url2)
    
  exit( errors )


def create_wdir():
  global wdir
  
  if wdir.exists():
    if wdir.is_file():
      os.remove(wdir)
      
  if wdir.exists() == False:
    wdir.mkdir(parents=True)


def get_last_block(url, max_tries=10, timeout=0.1):
  request = bytes( json.dumps( {
    "jsonrpc": "2.0",
    "id": 0,
    "method": "database_api.get_dynamic_global_properties",
    "params": {}
    } ), "utf-8" ) + b"\r\n"
    
  status, response = steemd_call(url, data=request, max_tries=max_tries, timeout=timeout)
  
  if status == False:
    return 0
  try:
    return response["result"]["head_block_number"]
  except:
    return 0


def compare_results(f_block, l_block, url1, url2, max_tries=10, timeout=0.1):
  global wdir
  global errors
  
  print( "Compare blocks [{} : {}]".format(f_block, l_block) )

  for i in range(f_block, l_block+1):
    request = bytes( json.dumps( {
      "jsonrpc": "2.0",
      "id": i,
      "method": "account_history_api.get_ops_in_block",
      "params": { "block_num": i, "only_virtual": False }
      } ), "utf-8" ) + b"\r\n"

    with ThreadPoolExecutor(max_workers=2) as executor:
    #with ProcessPoolExecutor(max_workers=2) as executor:
      future1 = executor.submit(steemd_call, url1, data=request, max_tries=max_tries, timeout=timeout)
      future2 = executor.submit(steemd_call, url2, data=request, max_tries=max_tries, timeout=timeout)

    status1, json1 = future1.result()
    status2, json2 = future2.result()
    
    #status1, json1 = steemd_call(url1, data=request, max_tries=max_tries, timeout=timeout)
    #status2, json2 = steemd_call(url2, data=request, max_tries=max_tries, timeout=timeout)
      
    if status1 == False or status2 == False or json1 != json2:
      print("Difference @block: {}\n".format(i))
      errors += 1
      
      filename = wdir / Path(str(f_block) + "_" + str(l_block) + ".log")
      try:    file = filename.open( "w" )
      except: print( "Cannot open file:", filename ); return
  
      file.write("Difference @block: {}\n".format(i))
      file.write("{} response:\n".format(url1))
      json.dump(json1, file, indent=2, sort_keys=True)
      file.write("\n")
      file.write("{} response:\n".format(url2))
      json.dump(json2, file, indent=2, sort_keys=True)
      file.write("\n")
      file.close()
      print( "Compare blocks [{} : {}] break with error".format(f_block, l_block) )
      return

  print( "Compare blocks [{} : {}] finished".format(f_block, l_block) )

  
if __name__ == "__main__":
  main()
