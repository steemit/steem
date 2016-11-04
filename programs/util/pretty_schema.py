#!/usr/bin/env python3

import requests

import itertools
import json

id_count = itertools.count(1)

result = requests.post( "http://127.0.0.1:18390", data=json.dumps({"jsonrpc": "2.0", "method": "call", "params": ["debug_node_api", "debug_get_json_schema", []], "id": next(id_count)}) )
str_schema = result.json()["result"]
schema = json.loads(str_schema)

def maybe_json(s):
    try:
        return json.loads(s)
    except ValueError:
        return s

new_types = {k : maybe_json(v) for k, v in schema["types"].items()}
schema["types"] = new_types

print(json.dumps(schema, indent=3, sort_keys=True))
