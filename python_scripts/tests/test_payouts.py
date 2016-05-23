"""
This test module will only run on a POSIX system. Windows support *may* be added at some point in the future.
"""
# Global imports
import json, operator, os, signal, sys

from argparse import ArgumentParser
from pathlib import Path
from time import sleep

# local imports
from steemdebugnode import DebugNode
from steemapi.steemnoderpc import SteemNodeRPC

WAITING = True

def main( ):
   global WAITING
   if( os.name != "posix" ):
      print( "This script only works on POSIX systems" )
      return

   parser = ArgumentParser( description='Run a steemd debug node on an existing chain, trigger a hardfork' \
                              ' and verify hardfork does not break invariants or block production' )
   parser.add_argument( '--steemd', '-s', type=str, required=True, help='The location of a steemd binary to run the debug node' )
   parser.add_argument( '--data-dir', '-d', type=str, required=True, help='The location of an existing data directory. ' + \
                        'The debug node will pull blocks from this directory when replaying the chain. The directory ' + \
                        'will not be changed.' )
   parser.add_argument( '--pause-node', '-p', type=bool, required=False, default=True, \
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

   signal.signal( signal.SIGINT, sigint_handler )

   debug_node = DebugNode( str( steemd ), str( data_dir ) )

   with debug_node :

      run_steemd_tests( debug_node )

      if( args.pause_node ):
         print( "Letting the node hang for manual inspection..." )
      else:
         WAITING = False

      while( WAITING ):
         sleep( 1 )


def run_steemd_tests( debug_node ):
   from steemapi.steemnoderpc import SteemNodeRPC

   try:
      print( 'Replaying blocks...', )
      sys.stdout.flush()
      total_blocks = 0
      while( total_blocks % 100000 == 0 ):
         total_blocks += debug_node.debug_push_blocks( 100000, skip_validate_invariants=True )
         print( 'Blocks Replayed: ' + str( total_blocks ) )
         sys.stdout.flush()

      print( "Triggering payouts" )
      sys.stdout.flush()
      debug_node.debug_generate_blocks_until( 1467590400 )

      print( "Generating blocks to verify nothing broke" )
      assert( debug_node.debug_generate_blocks( 10 ) == 10 )

      print( "Done!" )
      print( "Getting comment dump:" )
      sys.stdout.flush()
      rpc = SteemNodeRPC( 'ws://127.0.0.1:8095', '', '' )
      ret = rpc.get_discussions_by_cashout_time( '', '', str( 0xFFFFFFFF ) );

      print( 'author, url, total_payout_value, abs_rshares, num_active_votes' )

      for comment in ret:
         print( comment[ 'author' ] + ', ' + comment[ 'url' ] + ', ' + comment[ 'total_payout_value' ] + ', ' + comment[ 'cashout_time' ] )

   except ValueError as val_err:
      print( str( val_err ) )


def sigint_handler( signum, frame ):
   global WAITING
   WAITING = False
   sleep( 3 )
   sys.exit( 0 )

main()