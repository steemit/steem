#!/usr/bin/env python3

import json
import os
import random
import signal
import struct
import sys
import time

try:
    import asyncio
except ImportError:
    print("asyncio module not found (try pip install asyncio, or upgrade to Python 3.4 or later)")
    sys.exit(1)

try:
    import websockets
except ImportError:
    print("websockets module not found (try pip install websockets)")
    sys.exit(1)

@asyncio.coroutine
def mainloop():
    ws = yield from websockets.connect('ws://localhost:8090/')
    entropy = struct.unpack("<Q", os.urandom(8))[0]
    rand = random.Random(entropy)
    my_account_id = rand.randrange(0, 90000)

    gpo_get = {"id":1, "method":"call", "params":[0, "get_objects", ["2.0.0"]]}
    dgpo_sub = {"id":2, "method":"call", "params":[0, "subscribe_to_objects", [111, ["2.1.0"]]]}
    acct_sub = {"id":3, "method":"call", "params":[0, "get_full_accounts", [222, ["1.2."+str(my_account_id)], True]]}
    yield from ws.send(json.dumps(gpo_get))
    yield from ws.send(json.dumps(dgpo_sub))
    yield from ws.send(json.dumps(acct_sub))

    next_call_id = 4

    def peek_random_account():
        nonlocal next_call_id
        asyncio.get_event_loop().call_later(rand.uniform(0, 3), peek_random_account)
        peek_account_id = rand.randrange(0, 90000)
        acct_peek = {"id" : next_call_id, "method" : "call", "params":
           [0, "get_objects", [["1.2."+str(peek_account_id)], True]]}
        next_call_id += 1
        yield from ws.send(json.dumps(acct_peek))
        return

    yield from peek_random_account()

    while True:
        result = yield from ws.recv()
        #print(result)
        if result is None:
           break
    yield from ws.close()

child_procs = []

# stress test with 200 instances
while len(child_procs) < 200:
    pid = os.fork()
    if pid == 0:
        asyncio.get_event_loop().run_until_complete(mainloop())
        break
    else:
        child_procs.append(pid)

time.sleep(60)
for pid in child_procs:
    os.kill(pid, signal.SIGTERM)

time.sleep(2)
for pid in child_procs:
    os.kill(pid, signal.SIGKILL)
