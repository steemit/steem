import json
import requests

from utils.logger     import log
from utils.cmd_args   import args

class ArgsCheckException(Exception):
    def __init__(self, _message):
        self.message = _message

    def __str__(self):
        return self.message

user_name = list("aaaaaaaaaaaa")


def unifie_to_string(_arg):
    if not isinstance(_arg, str):
        prepared = str(_arg)
    else:
        prepared = _arg
    prepared = prepared.strip()
    prepared = prepared.replace(" ", "")
    return prepared


def check_call_args(_call_args, _response, _arg_prefix):
    splited_response = _response.split()
    for idx, line in enumerate(splited_response):
        if _arg_prefix in line:
            key = line[line.find(_arg_prefix+".")+len(_arg_prefix+"."):-1]
            #unifie to string
            call = unifie_to_string(_call_args[key])
            spli = unifie_to_string(splited_response[idx+1])

            if call in spli:
                _call_args.pop(key)
            else:
                log.error("Call arg `{0}` expected `{1}`".format(_call_args[key], str(splited_response[idx+1])))
                raise ArgsCheckException("Incossisten value for `{0}` key".format(key))
            if not _call_args:
                break
    if _call_args:
        raise ArgsCheckException("No all values checked, those `{0}` still remains. ".format(_call_args))


def call_and_check(_func, _call_args, _arg_prefix):
    response = _func(*_call_args.values())
    log.info("Call response: {0}".format(response))
    check_call_args(_call_args, response, _arg_prefix)
    return response


def call_and_check_transaction(_func, _call_args, _arg_prefix, _broadcast):
    response = _func(*_call_args.values(), _broadcast)
    check_call_args(_call_args, response, _arg_prefix)


def last_message_as_json( _message):
    if "message:" in _message:
        _message = _message[_message.rfind("message:")+len("message:"):]
        _message.strip()
        o = 0
        #lame... but works
        for index, ch in enumerate(_message):
            if str(ch) == "{":
                o +=1
                continue
            if str(ch) == "}":
                o -=1
                if o == 0:
                    _message = _message[:index+1]
                    break
    else:
        _message = "{}"
    return json.loads(_message)


def find_creator_proposals(_creator, _proposal_list):
    proposals = []
    if "result" in _proposal_list:
        result = _proposal_list["result"]
        for rs in result:
            if rs["creator"] == _creator:
                proposals.append(rs)
    return proposals


def find_voter_proposals(_voter, _proposal_list):
    if "result" in _proposal_list:
        result = _proposal_list["result"]
        for user, user_propsals in result.items():
            if user == _voter:
                return user_propsals
    return []


def ws_to_http(_url):
    pos = _url.find(":")
    return "http" + _url[pos:]


def get_valid_steem_account_name():
    http_url = ws_to_http(args.server_rpc_endpoint)
    while True:
        params = {"jsonrpc":"2.0", "method":"condenser_api.get_accounts", "params":[["".join(user_name)]], "id":1}
        resp = requests.post(http_url, json=params)
        data = resp.json()
        if "result" in data:
            if len(data["result"]) == 0:
                return ''.join(user_name)
        if ord(user_name[0]) >= ord('z'):
            for i, _ in enumerate("".join(user_name)):
                if user_name[i] >= 'z':
                    user_name[i] = 'a'
                    continue
                user_name[i] = chr(ord(user_name[i]) + 1)
                break
        else:
            user_name[0] = chr(ord(user_name[0]) + 1)
        if len(set(user_name)) == 1 and user_name[0] == 'z':
            break

def make_user_for_tests(_cli_wallet, _value_for_vesting = None,  _value_for_transfer_tests = None, _value_for_transfer_tbd = None):
    value_for_vesting           = _value_for_vesting        if _value_for_vesting else "20.000 TESTS"
    value_for_transfer_tests    = _value_for_transfer_tests if _value_for_transfer_tests else "20.000 TESTS"
    value_for_transfer_tbd      = _value_for_transfer_tbd   if _value_for_transfer_tbd else "20.000 TBD"

    creator = get_valid_steem_account_name()
    _cli_wallet.create_account( args.creator, creator, "", "true")
    receiver = get_valid_steem_account_name()
    _cli_wallet.create_account( args.creator, receiver, "", "true")

    _cli_wallet.transfer_to_vesting( args.creator, creator, value_for_vesting, "true")
    _cli_wallet.transfer( args.creator, creator, value_for_transfer_tests, "initial transfer", "true" )
    _cli_wallet.transfer( args.creator, creator, value_for_transfer_tbd, "initial transfer", "true")
    return creator, receiver
