#!/usr/bin/env python3
"""
Module parse json_rpc log file and create request/response.result pairs for each call.
Additionally yaml file is generated to execute these calls with pyresttest.

json_rpc_plugin will generate log messages if you add lines like these into config.ini:
log-file-appender = {"appender": "json_rpc", "file": "logs/json_rpc/json_rpc.log"}
log-logger = {"name": "json_rpc", "level": "debug", "appender": "json_rpc"}

In addition --jsonrpc-log-responses must be passed to steemd.

Usage: parse_json_rpc_log <log_file_name> [<location>]
       <location> points where output files generated
"""
import sys
import re
import json
from pathlib import Path

request_pat = re.compile( r"rpc_jsonrpc ] request: (?P<request>[^\t]*)" )
result_pat = re.compile( r"rpc_jsonrpc ] response\.result: (?P<response>[^\t]*)" )

def main():
   if len(sys.argv) < 2 or len(sys.argv) > 4:
      print( "Usage: parse_json_rpc_log <log_file_name> [<location>]" )
      print( "       <location> points where output files generated" )
      exit()
   location = ""
   if len(sys.argv) == 3:
      location = sys.argv[2]
      p = Path( location )
      if p.exists() and p.is_dir() == False:
         exit( location + " is not a directory")
      p.mkdir(parents=True, exist_ok=True)
      if location[-1] != '/':
         location += '/'
   file = 0
   try:     file = open( sys.argv[1] )
   except:  exit( "Cannot open " + sys.argv[1] + " file" )

   try:     y_file = open( location + "tests.yaml", "w" )
   except:  exit( "Cannot open " + location + "tests.yaml file")

   y_file.write( "---\n" )
   y_file.write( "- config:\n" )
   y_file.write( "  - testset: \"API Tests\"\n" )
   y_file.write( "  - generators:\n" )
   y_file.write( "    - test_id: {type: 'number_sequence', start: 1}\n" )
   y_file.write( "\n" )
   y_file.write( "- base_test: &base_test\n" )
   y_file.write( "  - generator_binds:\n" )
   y_file.write( "    - test_id: test_id\n" )
   y_file.write( "  - url: \"/rpc\"\n" )
   y_file.write( "  - method: \"POST\"\n" )
   y_file.write( "  - validators:\n" )
   y_file.write( "    - extract_test: {jsonpath_mini: \"error\", test: \"not_exists\"}\n" )
   y_file.write( "    - extract_test: {jsonpath_mini: \"result\", test: \"exists\"}\n" )
   y_file.write( "    - json_file_validator: {jsonpath_mini: \"result\", comparator: \"json_compare\", expected: {template: '$test_id'}}\n" )
   y_file.write( "\n\n" )

   rpc_counter = 1
   request = ""
   request_file = 0
   result_file = 0
   for line in file:
      if request == "":
         result = request_pat.search(line)
         if result:
            request = result.group( "request" )
      else:
         result = result_pat.search(line)
         if result == None:
            request = ""
            continue
         response = result.group( "response" )

         if response != "":
            try:
               request_file = open( location + str(rpc_counter) + ".json", "w" )
               result_file = open( location + str(rpc_counter) + ".json.pat", "w" )
            except:
               exit( "Cannot open request/response files in " + location )

            jo = json.loads( request )
            json.dump( jo, request_file, indent=4)
            jo = json.loads( response )
            json.dump( jo, result_file, indent=4, sort_keys=True)

            request_file.close()
            result_file.close()

            y_file.write( "- test:\n" )
            y_file.write( "  - body: {file: \"" + str(rpc_counter) + ".json\"}\n" )
            y_file.write( "  - name: \"test" + str(rpc_counter) + "\"\n" )
            y_file.write( "  - <<: *base_test\n\n" )
            y_file.write( "\n" )

            rpc_counter += 1

         request = ""

   file.close()
   y_file.close()

main()
