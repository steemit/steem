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
                            args.rpc_tls_endpoint,
                            args.rpc_tls_cert,
                            args.rpc_http_endpoint,
                            args.deamon, 
                            args.rpc_allowip,
                            args.wallet_file,
                            args.chain_id  )
        wallet.set_and_run_wallet()

        error_msg_x  = "The value `x` for `_last_id` argument is invalid, it should be integer type."
        resp_error_x = wallet.list_proposals(args.creator, "creator", "asc", 50, "all", "x")
        log.info(resp_error_x)
        if resp_error_x.find(error_msg_x) == -1:
            raise ArgsCheckException("Assertion `{0}` is required.".format(error_msg_x))

        error_msg_y  = "The value `y` for `_last_id` argument is invalid, it should be integer type."
        resp_error_y = wallet.list_proposals(args.creator, "creator", "asc", 50, "all", "y")
        log.info(resp_error_y)
        if resp_error_y.find(error_msg_y) == -1:
            raise ArgsCheckException("Assertion `{0}` is required.".format(error_msg_y))

        error_msg_10 = "The value `10` for `_last_id` argument is invalid, it should be integer type."
        resp_10      = wallet.list_proposals(args.creator, "creator", "asc", 50, "all", "10")
        log.info(resp_10)
        if resp_10.find(error_msg_10) != -1:
            raise ArgsCheckException("There should be no assertion `{0}`.".format(error_msg_10))


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

