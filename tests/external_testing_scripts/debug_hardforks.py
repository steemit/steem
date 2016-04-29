"""
This test module will only run on a POSIX system. Windows support *may* be added at some point in the future.
"""

import json, os, signal, subprocess, sys, threading, time
from pathlib import Path

steemd_lock = threading.Lock()
steemd_subprocess = None
test_dir = None

def main( ):
   global test_dir
   global steemd_subprocess
   global steemd_lock
   if( os.name != "posix" ):
      print( "This script only works on POSIX systems" )
      return

   signal.signal( signal.SIGINT, sigint_handler )

   argv = sys.argv

   # TODO: Increase cli args robustness and flexibility
   if len( argv ) != 3 or ( len( argv ) >= 3 and 'help' in argv[1] ):
      print_help()
      return

   data_dir = Path( argv[1] ).resolve()
   if( not data_dir.exists() or not data_dir.is_dir() ):
      print( "Specified witness node data_dir does not exist or is not a directory" )
      return

   block_dir = data_dir / 'blockchain/database/block_num_to_block'
   if( not block_dir.exists() or not block_dir.is_dir() ):
      print( "Block db in data_dir is non existent or configured incorrectly" )
      return

   steem_root = Path( os.getcwd() + '/' +  __file__.replace( "/debug_hardforks.py", "" ) )
   steem_root = steem_root / '..' / '..'
   steem_root = steem_root.resolve()

   if( not steem_root.exists() ):
      print( "Wasn't able to determine steem root directory? This is embarassing..." )
      return

   steemd = steem_root / 'programs/steemd'
   if( not steemd.exists() ):
      print( "steemd binary doesn't exists. Did you forget to build steemd?" )

   test_dir = Path( "/tmp/steem_debug_node" )

   if( not test_dir.exists() ):
      print( "Test directory does not exist, creating it now...", end="\t" )
      sys.stdout.flush()
      os.system( 'mkdir -p ' + str( test_dir ) )
      print( "Done!" )

   test_dir = test_dir.resolve()
   print( "Using test directory: " + str( test_dir ) )
   print( "Setting up test directory...", end="\t" )
   sys.stdout.flush()

   test_blocks = test_dir / 'test_blocks'

   if( not test_blocks.exists() ):
      test_blocks.mkdir()

   test_data_dir = test_dir / 'test_data_dir'
   if( not test_data_dir.exists() ):
      test_data_dir.mkdir()

   config = test_data_dir / 'config.ini'
   if( not config.exists() ):
      config.touch()

   os.system( 'echo "' + get_config() + '" > ' + str( config ) )

   print( "Done!" )

   print( "Copying in blocks from existing blockchain...", end="\t" )
   sys.stdout.flush()
   os.system( 'cp -Rp ' + str( block_dir ) + '/ ' + str( test_dir / 'test_blocks/' ) )
   print( "Done!" )

   steemd_lock.acquire()

   print( "Starting up steemd with the debug node plugin...", end="\t")
   sys.stdout.flush()
   FNULL = open( os.devnull, 'w' )
   steemd_subprocess = subprocess.Popen( [ str( steemd / 'steemd' ), '--data-dir="' + str( test_data_dir ) + '"' ], stdout=FNULL, stderr=FNULL )
   print( "Done!" )

   steemd_lock.release()

   run_steemd_tests( test_blocks )

   # Term on completion?
   while( not argv[2] ):
      time.sleep( 1 )

   # cleanup
   return cleanup()

def cleanup():
   global test_dir
   global steemd_subprocess
   global steemd_lock

   ret = 0

   steemd_lock.acquire()
   print( "Checking steemd status" )
   if( steemd_subprocess ):
      steemd_subprocess.poll()

      if( not steemd_subprocess.returncode ):
         print( "steemd still running, requesting shutdown..." )
         sys.stdout.flush()
         steemd_subprocess.send_signal( signal.SIGINT )

         time.sleep( 5 )
         steemd_subprocess.poll()

         if( not steemd_subprocess.returncode ):
            print( "steemd did not shutdown as requested, killing it now!" )
            steemd_subprocess.send_signal( signal.SIGTERM )
            steemd_subprocess.poll()

         if( steemd_subprocess.returncode ):
            print( "steemd shutdown" )
            ret = steemd_subprocess.returncode
         else:
            print( "steemd still not shutdown... May require user intervention" )
            ret = 1

   steemd_lock.release()

   if( test_dir and test_dir.exists() ):
      print( "Cleaning up test dir...", end="\t" )
      sys.stdout.flush()
      ret = max( os.system( 'rm -rf ' + str( test_dir) ), ret )
      print ( "Done!" )

   return ret

def sigint_handler( signum, frame ):
   exit( cleanup() )

def print_help():
   print( "Usage: debug_hardforks.py existing_data_dir kill_on_completion" )
   print( "If test_dir does not exist, it will be created. It should be empty as it is used as a scratch area for this test." )

def get_config():
   return "# no seed-node in config file or command line\n" \
          + "p2p-endpoint = 127.0.0.1:2001       # bind to localhost to prevent remote p2p nodes from connecting to us\n" \
          + "rpc-endpoint = 127.0.0.1:8090       # bind to localhost to secure RPC API access\n" \
          + "enable-plugin = witness account_history debug_node\n" \
          + "public-api = database_api login_api debug_node_api\n"

def run_steemd_tests( test_blocks ):
   from steemapi.steemnoderpc import SteemNodeRPC

   try:
      print( "Connecting to rpc endpoint...", end="\t" )
      sys.stdout.flush()
      rpc = SteemNodeRPC( "ws://127.0.0.1:8090", "", "" )
      print( "Done!" )
      print( "Playing blockchain..." )

      num_blocks = 0

      while( True ):
         ret = rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_push_blocks",["' + str( test_blocks ) + '", 10000]], "id": 1}' ) )
         num_blocks += int( ret )
         if( num_blocks % 100000 == 0 ):
            print( "Blocks pushed: " + str( num_blocks ) )

         if( ret != 10000 ):
            break

      print( "Setting the hardfork now" ) # TODO: Grab most recent hardfork num from build directory
      sys.stdout.flush()
      rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_set_hardfork",[4]], "id":4}' ) )

      print( "Generating blocks after the hardfork" )
      debug_key = '5JHNbFNDg834SFj8CMArV6YW7td4zrPzXveqTfaShmYVuYNeK69' # get_dev_key steem debug --- Do not use this on the live chain for obvious reasons!

      new_blocks = 5000
      ret = rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [2,"debug_generate_blocks",["' + debug_key + '",' + str( new_blocks ) + ']], "id": 3}' ) )
      if( ret != new_blocks ):
         print( "Debug node did not generate requested blocks. Likely failed trying..." )
         print( "Returned: " + ret )
         return

      print( "Generating new blocks...", end="\t" )
      sys.stdout.flush()
      block_producers = {}
      for i in range( num_blocks + 1, num_blocks + 1 + new_blocks ):
         ret = rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_block",[' + str( i ) + ']], "id":4}' ) )
         if( ret[ "witness" ] in block_producers ):
            block_producers[ ret[ "witness" ] ] += 1
         else:
            block_producers[ ret[ "witness" ] ] = 1

      print( "Done!" )
      print( "Calculating block producer distribution:" )
      import operator
      sorted_block_producers = sorted( block_producers.items(), key=operator.itemgetter( 1 ) )

      for (k, v) in sorted_block_producers:
         ret = rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_witness_by_account",["' + k + '"]], "id":5}' ) )
         print( '{"witness":"' + k + '","votes":' + str( ret["votes"] ) + ',"blocks":' + str( v ) + '}' )

   except ValueError as val_err:
      print( str( val_err ) )
   except:
      print( "Unknown exception" )
      print( str( sys.exc_info()[0] ) )

main()