#!/usr/bin/python3

import subprocess

from utils.test_utils import *
from utils.cmd_args   import args
from utils.logger     import log, init_logger

if __name__ == "__main__":
    try:
        init_logger(__file__)
        error = False
        output = subprocess.run([args.path+"/cli_wallet", "--help"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output_stdout = output.stdout.decode('utf-8')
        args_founded = [ arg for arg in output_stdout.split() if "--" in arg ]
        only_args_to_be_founded = ['--help', '--server-rpc-endpoint', '--cert-authority', '--rpc-endpoint', '--rpc-tls-endpoint', '--rpc-tls-certificate', '--rpc-http-endpoint', '--daemon', '--rpc-http-allowip', '--wallet-file', '--chain-id']
        res = list(set(args_founded)^set(only_args_to_be_founded))
        if res:
            raise ArgsCheckException("There are some additional argument in cli_wallet `{0}`.".format(res))

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



