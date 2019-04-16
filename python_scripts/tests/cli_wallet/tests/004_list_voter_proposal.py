#!/usr/bin/python3

from collections      import OrderedDict

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

        active          = ["active", "inactive", "all"]
        order_by        = ["creator", "start_date", "end_date", "total_votes"]
        order_direction = ["asc", "desc"]

        for by in order_by:
            for direct in  order_direction:
                for act in active:
                    call_args = OrderedDict()
                    call_args["start"]=args.creator 
                    call_args["order_by"]=by 
                    call_args["order_direction"]=direct
                    call_args["limit"]=10
                    call_args["status"]=act
                    call_args["last_id"]=""
                    resp = last_message_as_json( call_and_check(wallet.list_voter_proposals, call_args, "args"))
                    if not "result" in resp:
                        raise ArgsCheckException("No `result` in response")


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



