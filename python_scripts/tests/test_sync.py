from steemapi.steemnoderpc import SteemNodeRPC
import sys
from time import sleep
import os
import subprocess
import threading
import shutil
import datetime
from tempfile import TemporaryDirectory
import getopt
from pathlib import Path

class WatchDog( object ):
    def __init__( self, steemd, config_file=None, seed_file=None, args='', plugins=[], apis=[], steemd_out=None, steemd_err=None ):
        """ Creates a steemd WatchDog.
          It can be ran by using 'with WatchDog:'
          While in the context of 'with' the debug node will continue to run.
          Upon exit of 'with' the debug will exit and clean up temporary files.
          This class also contains methods to allow basic manipulation of the blockchain.
          For all other requests, the python-steem library should be used.
          args:
             steemd -- The string path to the location of the steemd binary
             config_file -- Path to a config.ini file to use.
             args -- Other string args to pass to steemd.
             plugins -- Any additional plugins to start with the debug node. Modify plugins DebugNode.plugins
             apis -- Any additional APIs to have available. APIs will retain this order for accesibility starting at id 3.
                database_api is 0, login_api is 1, and debug_node_api is 2. Modify apis with DebugNode.api
             steemd_stdout -- A stream for steemd's stdout. Default is to pipe to /dev/null
             steemd_stderr -- A stream for steemd's stderr. Default is to pipe to /dev/null
        """
        self._FNULL = None
        self._steemd_bin = steemd
        self._config_file = config_file
        self._seed_file = seed_file
        self._temp_data_dir = TemporaryDirectory()

        self.plugins = plugins
        self.apis = apis

        if( args != '' ):
         self._args = args.split( "\\s" )
        else:
         self._args = list()

        self._FNULL = open( os.devnull, 'w' )
        if( steemd_out != None ):
         self.steemd_out = open(steemd_out, 'w')
        else:
         self.steemd_out = self._FNULL

        if( steemd_err != None ):
         self.steemd_err = open(steemd_err, 'w')
        else:
         self.steemd_err = self._FNULL

    def _get_config(self):
       return "# no seed-node in config file or command line\n" \
           + "rpc-endpoint = 127.0.0.1:8090              # bind to localhost to secure RPC API access\n" \
           + "seed-node = seed.riversteem.com:2001       # riverhead\n" \
           + "seed-node = steem-seed1.abit-more.com:2001 # abit\n" \
           + "seed-node = 52.74.152.79:2001              # smooth\n" \
           + "enable-plugin = witness "+ " ".join( self.plugins ) + "\n" \
           + "public-api = database_api login_api " + " ".join( self.apis ) + "\n"

    def panic(self, stl, lbn):
        print("\nWATCHDOG: Error syncing between block #"+str(stl)+" and #"+str(lbn))
        steemd.kill()
        sys.exit(1)

    def watchForHangDurringSync(self, steemd):
        duplicates = 0
        last_local_block_number = 0
        second_to_last = 0
        localRpc = SteemNodeRPC("ws://localhost:8090", "", "", num_retries=3)
        print("WATCHDOG: Connection Established")
        remoteRpc = SteemNodeRPC("wss://steemit.com/wspa", "", "", num_retries=3)
        remoteProps = remoteRpc.get_dynamic_global_properties()
        while True:
            localProps = localRpc.get_dynamic_global_properties()
            local_block_number = localProps['last_irreversible_block_num']
            if (last_local_block_number == local_block_number) or (local_block_number == 0):
                duplicates+=1
                if duplicates > 10:
                    panic(second_to_last, local_block_number) # We have been on the same block for 100 seconds.
                    return
            else:
                duplicates=0
                second_to_last = last_local_block_number
                last_irreversible_block_num = remoteProps['last_irreversible_block_num']
                last_local_block_number = local_block_number
                if(local_block_number>=last_irreversible_block_num):
                    if(localRpc.get_block(last_irreversible_block_num)['witness_signature'] == remoteRpc.get_block(last_irreversible_block_num)['witness_signature']):
                        print("\nWATCHDOG: Successfully synced")
                        steemd.kill()
                        sys.exit(0)
                    else:
                        print("\nWATCHDOG: Fatal error... client forked")
                        steemd.kill()
                        sys.exit(2)
                        return # We are all the way synced
                # print(local_block_number) #DEBUG
            sleep(10)
        return
    def begin(self):
        os.makedirs(self._temp_data_dir.name+"/witness_node_data_dir")
        configiniSource = self._get_config()
        if(self._config_file is not None):
            with open(self._config_file) as configFile:
                configiniSource = configFile.read()
        with open(self._temp_data_dir .name+"/witness_node_data_dir/config.ini", 'w') as configini:
            configini.write(configiniSource)
        steemd_args = []
        steemd_args.append(self._steemd_bin)
        with open(self._seed_file, "r") as f:
            for line in f:
                steemd_args.append(" --seed-node="+line.split(' ',1)[0])
        print(steemd_args)
        steemdaemon = subprocess.Popen(steemd_args, cwd=self._temp_data_dir.name, stdout=self.steemd_out, stderr=self.steemd_err)
        sleep(5)
        t1 = threading.Thread(target=self.watchForHangDurringSync, args=[steemdaemon]) # Begin watching in another thread.
        t1.start()
        t1.join()
        rc = steemdaemon.wait()
        if(rc != 0 and rc != 1 and rc != -9):
            print("\nWATCHDOG: Something went wrong while syncing! Exit code: " + str(rc))
            sys.exit(rc)

def main():
    from argparse import ArgumentParser
    parser = ArgumentParser( description='Begins a steemd instance and attempts to sync it to the network. ' + \
                             'In the event the instance fails to sync with the network, the program exits ' + \
                             'with a non-zero return code.' )
    parser.add_argument( '--steemd', '-s', type=str, required=True, help='The location of a steemd binary to run the debug node' )
    parser.add_argument( '--config-file', '-c', type=str, required=False, help='The location of an existing config file' )
    parser.add_argument( '--log-file', '-l', type=str, required=False, help='The location of the file which will be logged to.' )
    parser.add_argument( '--seed-nodes', '-sn', type=str, required=False, help='The location of the file which contains seed nodes.' )
    args = parser.parse_args()

    config = None
    seeds = None
    logs = None
    steemd = Path( args.steemd )
    if( not steemd.exists() ):
       print( 'Error: steemd does not exist.' )
       return

    steemd = steemd.resolve()
    if( not steemd.is_file() ):
       print( 'Error: steemd is not a file.' )
       return

    if(args.config_file is not None):
        config = Path( args.config_file )
        if( not config.exists() ):
           print( 'Error: config does not exist.' )
           return

        config = config.resolve()
        if( not config.is_file() ):
           print( 'Error: config is not a file.' )
           return

        config = args.config_file

    if(args.log_file is not None):
        logs = args.log_file

    if(args.seed_nodes is not None):
        seeds = Path( args.seed_nodes )
        if( not seeds.exists() ):
           print( 'Error: seeds does not exist.' )
           return

        seeds = seeds.resolve()
        if( not seeds.is_file() ):
           print( 'Error: seeds is not a file.' )
           return

        seeds = args.seed_nodes
    watchdog = WatchDog(args.steemd, config, seed_file=seeds, steemd_out=logs, steemd_err=logs)
    watchdog.begin()

if __name__ == "__main__":
    main()
