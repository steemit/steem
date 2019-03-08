import json

from utils.logger     import log

class ArgsCheckException(Exception):
    def __init__(self, _message):
        self.message = _message

    def __str__(self):
        return self.message

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
    check_call_args(_call_args, response, _arg_prefix)


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
        _message = "[]"
    return json.loads(_message)