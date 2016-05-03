"""
This test module will only run on a POSIX system. Windows support *may* be added at some point in the future.
"""
# Global imports
import json, operator, os, sys

from argparse import ArgumentParser
from pathlib import Path
from time import sleep

# local imports
from steemdebugnode import DebugNode
from steemapi.steemnoderpc import SteemNodeRPC


def main( ):
   if( os.name != "posix" ):
      print( "This script only works on POSIX systems" )
      return

   parser = ArgumentParser( description='Run a steemd debug node on an existing chain, trigger a hardfork' \
                              ' and verify hardfork does not break invariants or block production' )
   parser.add_argument( '--steemd', '-s', type=str, required=True, help='The location of a steemd binary to run the debug node' )
   parser.add_argument( '--data-dir', '-d', type=str, required=True, help='The location of an existing data directory. ' + \
                        'The debug node will pull blocks from this directory when replaying the chain. The directory ' + \
                        'will not be changed.' )
   parser.add_argument( '--pause-node', '-p', type=bool, required=False, default=False, \
                        help='True if the debug node should pause after it\'s tests. Default: false' )

   args = parser.parse_args()

   steemd = Path( args.steemd )
   if( not steemd.exists() ):
      print( 'Error: steemd does not exist.' )
      return

   steemd = steemd.resolve()
   if( not steemd.is_file() ):
      print( 'Error: steemd is not a file.' )
      return

   data_dir = Path( args.data_dir )
   if( not data_dir.exists() ):
      print( 'Error: data_dir does not exist or is not a properly constructed steemd data directory' )

   data_dir = data_dir.resolve()
   if( not data_dir.is_dir() ):
      print( 'Error: data_dir is not a directory' )

   debug_node = DebugNode( str( steemd ), str( data_dir ) )

   with debug_node :

      run_steemd_tests( debug_node )

      # Term on completion?
      while( args.pause_node ):
         sleep( 1 )


def run_steemd_tests( debug_node ):
   from steemapi.steemnoderpc import SteemNodeRPC

   try:
      print( "Playing blockchain..." )
      assert( debug_node.debug_push_blocks( 5000 ) == 5000 )

      print( "Setting the hardfork now" ) # TODO: Grab most recent hardfork num from build directory
      sys.stdout.flush()
      debug_node.debug_set_hardfork( 4 )

      print( "Generating blocks after the hardfork" )
      assert( debug_node.debug_generate_blocks( 5000 ) == 5000 )

      print( "Done!" )
      print( "Calculating block producer distribution:" )
      sys.stdout.flush()
      rpc = SteemNodeRPC( 'ws://127.0.0.1:8090', '', '' )
      block_producers = {}
      for i in range( 5001, 10001 ):
         ret = rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_block",[' + str( i ) + ']], "id":4}' ) )
         if( ret[ "witness" ] in block_producers ):
            block_producers[ ret[ "witness" ] ] += 1
         else:
            block_producers[ ret[ "witness" ] ] = 1

      sorted_block_producers = sorted( block_producers.items(), key=operator.itemgetter( 1 ) )
      for (k, v) in sorted_block_producers:
         ret = rpc.rpcexec( json.loads( '{"jsonrpc": "2.0", "method": "call", "params": [0,"get_witness_by_account",["' + k + '"]], "id":5}' ) )
         print( '{"witness":"' + k + '","votes":' + str( ret["votes"] ) + ',"blocks":' + str( v ) + '}' )

   except ValueError as val_err:
      print( str( val_err ) )

main()