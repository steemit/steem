#!/usr/bin/env python3
"""
  Usage: script_name jobs url1 url2 [wdir [last_week [first_week]]]
    Example: script_name 4 http://127.0.0.1:8090 http://127.0.0.1:8091 ./ 104
    set jobs to 0 if you want use all processors
    if last_week is 0 or is not passed, 52 will be used
"""

import datetime
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


def future_end_cb(future):
  global errors
  if future.result() == False:
    errors += 1


def main():
  if len(sys.argv) < 4 or len(sys.argv) > 7:
    print("Usage: script_name jobs url1 url2 [wdir [last_week [first_week]]]")
    print("  Example: script_name 4 http://127.0.0.1:8090 http://127.0.0.1:8091 ./ 104 52")
    print( "  set jobs to 0 if you want use all processors" )
    print("  if weeks_number is 0 or is not passed, 52 will be used")
    exit()
    
  global wdir
  global errors
  first_week = 0
  last_week = 52

  jobs = int(sys.argv[1])
  if jobs <= 0:
    import multiprocessing
    jobs = multiprocessing.cpu_count()
    
  url1 = sys.argv[2]
  url2 = sys.argv[3]
  
  if len(sys.argv) > 4:
    wdir = Path(sys.argv[4])
    
    if len(sys.argv) > 5:
      last_week = int(sys.argv[5])
    else:
      last_week = 52
    if len(sys.argv) == 7:
      first_week = int(sys.argv[6])
    else:
      first_week = 0
    
  if last_week < first_week:
    exit("last week " +str(last_week)+ " cannot be less than first week " +len(first_week)+ "!" )
    
  create_wdir()

  weeks = last_week - first_week + 1

  if jobs > weeks:
    jobs = weeks

  print("setup:")
  print("  jobs: {}".format(jobs))
  print("  url1: {}".format(url1))
  print("  url2: {}".format(url2))
  print("  wdir: {}".format(wdir))
  print("  week range: {}:{}".format(first_week, last_week))

  if jobs > 1:
    weeks_per_job = weeks // jobs

    with ProcessPoolExecutor(max_workers=jobs) as executor:
      for i in range(jobs-1):
        future = executor.submit(compare_results, first_week, (first_week + weeks_per_job - 1), url1, url2)
        future.add_done_callback(future_end_cb)
        first_week = first_week + weeks_per_job
      future = executor.submit(compare_results, first_week, last_week, url1, url2)
      future.add_done_callback(future_end_cb)
  else:
    errors = (compare_results(first_week, last_week, url1, url2) == False)
    
  exit( errors )


def create_wdir():
  global wdir
  
  if wdir.exists():
    if wdir.is_file():
      os.remove(wdir)
      
  if wdir.exists() == False:
    wdir.mkdir(parents=True)


def compare_results(f_week, l_week, url1, url2, max_tries=10, timeout=0.1):
  global wdir
  
  print( "Compare weeks [{} : {}]".format(f_week, l_week) )

  genesis_date = datetime.datetime.strptime("2016-03-24","%Y-%m-%d")
  start_delta = datetime.timedelta(weeks=f_week)
  week_delta = datetime.timedelta(weeks=1)

  date = genesis_date + start_delta

  for i in range(f_week, l_week+1):
    request = bytes( json.dumps( {
      "jsonrpc": "2.0",
      "id": i,
      "method": "call",
      "params": [ "database_api", "list_comments", {"start": ['{0:%Y-%m-%dT00-00-00}'.format(date),"",""], "limit": 1000, "order": "by_cashout_time"} ]
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
      print("Difference @week: {}\n".format(i))

      filename1 = wdir / Path(str(f_week) + "_" + str(l_week) + "_ref.log")
      filename2 = wdir / Path(str(f_week) + "_" + str(l_week) + "_tested.log")
      try:    file1 = filename1.open( "w" )
      except: print( "Cannot open file:", filename1 ); return
      try:    file2 = filename2.open( "w" )
      except: print( "Cannot open file:", filename2 ); return
  
      file1.write("{} response:\n".format(url1))
      json.dump(json1, file1, indent=2, sort_keys=True)
      file1.close
      file2.write("{} response:\n".format(url2))
      json.dump(json2, file2, indent=2, sort_keys=True)
      file2.close()
      print( "Compare weeks [{} : {}] break with error".format(f_week, l_week) )
      return False

    date = date + week_delta

  print( "Compare weeks [{} : {}] finished".format(f_week, l_week) )
  return True

  
if __name__ == "__main__":
  main()