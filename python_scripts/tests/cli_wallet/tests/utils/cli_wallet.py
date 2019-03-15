#!/usr/bin/python3

import sys
import json
import queue
import requests
import threading
import subprocess

from .logger import log

from .test_utils import last_message_as_json, ws_to_http

class CliWalletException(Exception):
    def __init__(self, _message):
        self.message = _message

    def __str__(self):
        return self.message    

class CliWallet(object):

    class CliWalletArgs(object):
        def __init__(self, _path_to_executable,
                       _server_rpc_endpoint,
                       _cert_auth,
                       #_rpc_endpoint,
                       _rpc_tls_endpoint,
                       _rpc_tls_cert,
                       _rpc_http_endpoint,
                       _deamon,
                       _rpc_allowip,
                       _wallet_file,
                       _chain_id ):
            self.path = _path_to_executable+'/cli_wallet'
            self.server_rpc_endpoint = _server_rpc_endpoint
            self.cert_auth = _cert_auth
            #self.rpc_endpoint = _rpc_endpoint
            self.rpc_tls_endpoint = _rpc_tls_endpoint
            self.rpc_tls_cert = _rpc_tls_cert
            self.rpc_http_endpoint = _rpc_http_endpoint
            self.deamon = _deamon
            self.rpc_allowip = _rpc_allowip
            self.wallet_file = _wallet_file
            self.chain_id  = _chain_id

        def args_to_list(self):
            test_args = []
            args = {"server_rpc_endpoint": self.server_rpc_endpoint}
            args["cert_auth"]         = self.cert_auth
            #args["rpc_endpoint"]      = self.rpc_endpoint
            args["rpc_tls_endpoint"]  = self.rpc_tls_endpoint
            args["rpc_tls_cert"]      = self.rpc_tls_cert
            args["rpc_http_endpoint"] =self.rpc_http_endpoint
            args["deamon"]            = self.deamon
            args["rpc_allowip"]       = self.rpc_allowip
            args["wallet_file"]       = self.wallet_file
            args["chain_id"]          = self.chain_id
            for key, val in args.items():
                if val :
                    test_args.append("--"+key.replace("_","-")+ " ")
                    test_args.append(val)
            test_args = " ".join(test_args)
            return test_args


    def __init__(self, _path_to_executable,
                       _server_rpc_endpoint="ws://127.0.0.1:8090",
                       _cert_auth="_default",
                       #_rpc_endpoint="127.0.0.1:8091", 
                       _rpc_tls_endpoint="127.0.0.1:8092", 
                       _rpc_tls_cert="server.pem", 
                       _rpc_http_endpoint="127.0.0.1:8093", 
                       _deamon=False,  
                       _rpc_allowip=[],
                       _wallet_file="wallet.json", 
                       _chain_id="18dcf0a285365fc58b71f18b3d3fec954aa0c141c44e4e5cb4cf777b9eab274e"):

        self.cli_args = CliWallet.CliWalletArgs(_path_to_executable, _server_rpc_endpoint, _cert_auth, #_rpc_endpoint,
                                      _rpc_tls_endpoint, _rpc_tls_cert, 
                                      _rpc_http_endpoint, _deamon, _rpc_allowip, _wallet_file, _chain_id  )
        self.cli_proc = None
        self.response = ""
        self.q = queue.Queue()
        self.t = threading.Thread(target=self.output_reader, args=())
        

    def __getattr__(self, _method_name):
        if self.cli_proc:
            self.method_name = _method_name
            return self
        else:
            log.error("Cli_wallet is not set")
            raise CliWalletException("Cli_wallet is not set")


    def __call__(self,*_args):
        try:
            self.response = ""
            self.send_and_read(self.prepare_args(*_args))
            return self.response
        except Exception as _ex:
            log.exception("Exception `{0}` occuress while calling `{1}` with `{2}` args.".format(str(_ex), self.method_name, list(_args)))


    def set_and_run_wallet(self):
        try:
            log.info("Calling cli_wallet with args `{0}`".format([self.cli_args.path+ " " + self.cli_args.args_to_list()]))
            self.cli_proc = subprocess.Popen([self.cli_args.path+ " " + self.cli_args.args_to_list()], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=1, shell=True)
            if not self.cli_proc:
                raise CliWalletException("Failed to run cli_wallet")
            self.t.daemon=True
            self.t.start()
            self.set_password("{0}".format("testpassword"))
            self.unlock("{0}".format("testpassword"))
            self.import_key("{0}".format("5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n"))
        except Exception as _ex:
            log.exception("Exception `{0}` occuress while while running cli_wallet.".format(str(_ex)))


    #we dont have stansaction status api, so we need to combine...
    def wait_for_transaction_approwal(self):
        json_resp = last_message_as_json(self.response)
        block_num = json_resp["result"]["block_num"]
        trans_id  = json_resp["result"]["id"]
        url = ws_to_http(self.cli_args.server_rpc_endpoint)
        idx = -1
        while True:
            param = {"jsonrpc":"2.0", "method":"block_api.get_block", "params":{"block_num":block_num+idx}, "id":1}
            resp = requests.post(url, json=param)
            data = resp.json()
            if "result" in data and "block" in data["result"]:
                block_transactions = data["result"]["block"]["transaction_ids"]
                if trans_id in block_transactions:
                    log.info("Transaction `{0}` founded in block `{1}`".format(trans_id, block_num+idx))
                    break
            idx += 1


    def check_if_transaction(self):
        json_resp = last_message_as_json(self.response)
        if "result" in json_resp:
            if "id" in json_resp["result"]:
                return True
        return False


    def read_output(self, _timeout):
        while True:
            try:
                self.response += self.q.get(block=True, timeout=_timeout)
            except queue.Empty:
                break


    def send(self, _data):
        self.cli_proc.stdin.write(_data.encode("utf-8"))
        self.cli_proc.stdin.flush()


    def send_and_read(self, _data):
        log.info("Sending {0}".format(_data))
        self.send(_data)
        self.read_output(3)

        #asserions does not occures after above flush, so we need to send additiona `Enter`        
        self.send("\n")
        self.read_output(0.5)
        if self.check_if_transaction():
            self.wait_for_transaction_approwal()

        return self.response


    def exit_wallet(self):
        try:
            if not self.cli_proc:
                log.info("Cannot exit wallet, because wallet was not set - please run it first by using `run_wallet` metode.")
            self.cli_proc.communicate()
            return self.cli_proc
        except Exception as _ex:
            log.exception("Exception `{0}` occuress while while running cli_wallet.".format(str(_ex)))


    def output_reader(self):
        while True:
            try:
                for line in iter(self.cli_proc.stdout.readline, b''):
                    self.q.put_nowait(line.decode('utf-8') )
            except queue.Full:
                pass


    def prepare_args(self, *_args):
        name = self.method_name
        args = _args
        prepared_args = name + " "
        for arg in args:
            if isinstance(arg, int):
                prepared_args += str(arg) + " "
            elif isinstance(arg, str):
                if arg:
                    prepared_args += "\"{0}\"".format(arg) + " "
                else:
                    prepared_args += '\"\"' + " "
            else:
                prepared_args += "{0}".format(arg) + " "
        return prepared_args + "\n"

