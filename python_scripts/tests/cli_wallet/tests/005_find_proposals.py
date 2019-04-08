#!/usr/bin/python3

from utils.test_utils import *
from utils.cmd_args   import args
from utils.cli_wallet import CliWallet
from utils.logger     import log, init_logger

if __name__ == "__main__":
    try:
        init_logger(__file__)
        error = False
        wallet = CliWallet( args.path,
                            args.server_rpc_endpoint,
                            args.cert_auth,
                            #args.rpc_endpoint,
                            args.rpc_tls_endpoint,
                            args.rpc_tls_cert,
                            args.rpc_http_endpoint,
                            args.deamon, 
                            args.rpc_allowip,
                            args.wallet_file,
                            args.chain_id  )
        wallet.set_and_run_wallet()

        id_sets = [ [1], [2], [3], [1,2], [1,2,3], [2,3], [3,4], [4,5], [1,2,3,4,5,6,7]]

        for ids in id_sets:
            call_args = {"id_set":ids}
            call_and_check(wallet.find_proposals, call_args, "args")
        
    except Exception as _ex:
        log.exception(str(_ex))
        error = True
    finally:
        if error:
            log.error("TEST `{0}` failed".format(__file__))
            exit(1)
        else:
            log.info("TEST `{0}` passed".format(__file__))
            exit(0)



