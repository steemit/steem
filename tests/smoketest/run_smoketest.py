#!/usr/bin/env python3
"""
Prepare (update sources & build) tested steemd for use in smoketest script. 
Usage: run_smoketest.py <tested_branch_name> <reference_branch_dir>
Example: run_smoketest.py 2041-bmic-replacement reference_src
"""
import logging
import multiprocessing
import os
import subprocess
import sys

TESTED_DIR = os.path.expanduser("~/src/steemit/steem")
TESTED_STEEMD = os.path.join( TESTED_DIR, "build", "programs", "steemd", "steemd" )
SMOKETEST = os.path.join( TESTED_DIR, "tests", "smoketest", "smoketest.sh" )
TEST_BLOCKCHAIN_DIR = os.path.expanduser("~/src/blockchain/1")
REF_BLOCKCHAIN_DIR = os.path.expanduser("~/src/blockchain/2")
BLOCKS_NUMBER = "300000"
JOBS = "3"

start_dir = os.getcwd()
logger = logging.getLogger()

def exit_on_error( return_code, message ):
   if return_code != 0:
      logger.fatal( message )
      os.chdir( start_dir )
      exit()

def main():
   this_script_name = os.path.basename(__file__)
   formatter = logging.Formatter()
   formatter._fmt = this_script_name + ": " + formatter._fmt
   handler = logging.StreamHandler()
   handler.setFormatter(formatter)
   logger.addHandler(handler)
   logger.setLevel(logging.INFO)


   if len( sys.argv ) != 3:
      exit_on_error( 1, "Usage: run_smoketest.py <tested_branch_name> <reference_branch_dir>" )

   if not os.path.isdir( sys.argv[2] ) :
      exit_on_error( 1, sys.argv[2] + " not found or is not a directory" )
   reference_dir = os.path.join( sys.argv[2], "steemit", "steem" )
   if not os.path.isdir( reference_dir ) :
      exit_on_error( 1, sys.argv[2] + " has no steemit/steem subdirectory" )
   reference_steemd = os.path.join( reference_dir, "build", "Release", "programs", "steemd", "steemd" )
   if not os.path.isfile( reference_steemd ) :
      exit_on_error( 1, "Reference steemd not found (" + reference_steemd + ")" )

   if not os.path.isdir( TESTED_DIR ) :
      exit_on_error( 1, TESTED_DIR + " not found or is not a directory" )
   if not os.path.isfile( SMOKETEST ) :
      exit_on_error( 1, SMOKETEST + " not found or is not a file" )

   os.chdir( TESTED_DIR )

   # Checkout provided branch into standard test directory.
   tested_branch = sys.argv[1]
   msg = "branch " + tested_branch + " to " + TESTED_DIR
   logger.info( "Checking out " + msg )
   exit_on_error( subprocess.call(["git", "checkout", tested_branch]), "FATAL: Failed to checkout " + msg )
   exit_on_error( subprocess.call(["git", "submodule", "update", "--init", "--recursive"]), "FATAL: Failed to update submodules" )

   build_dir = os.path.join( os.getcwd() , "build" )
   if not os.path.isdir( build_dir ) :
      try :
         os.mkdir( build_dir )
      except :
         exit_on_error( 1, "FATAL: Failed to create " + build_dir + "directory" )

   os.chdir( build_dir )

   logger.info( "Building tested steemd" )
   exit_on_error( subprocess.call(["cmake", "-DCMAKE_BUILD_TYPE=Release", ".."]), "FATAL: cmake fail" )
   exit_on_error( subprocess.call(["make", "-j" + str( multiprocessing.cpu_count() ), "steemd"]), "FATAL: make fail" )

   if not os.path.isfile( TESTED_STEEMD ) :
      exit_on_error( 1, "Tested steemd not found (" + TESTED_STEEMD + ")" )

   os.chdir( os.path.join( TESTED_DIR, "tests", "smoketest" ) )

   logger.info( "Running smoketest with built tested steemd" )
   result = subprocess.call([SMOKETEST, TESTED_STEEMD, reference_steemd, TEST_BLOCKCHAIN_DIR, REF_BLOCKCHAIN_DIR, BLOCKS_NUMBER, JOBS])
   exit_on_error( result, "Smoketest failed with code " + str(result) )

   os.chdir( start_dir )

if __name__ == "__main__":
    main()
