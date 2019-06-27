#!/usr/bin/env python3

import collections
import json
import subprocess
import sys

argv = list(sys.argv)

dpnd = None
i_dpnd = -1
for i, a in enumerate(argv):
    if a.startswith("--dpnd="):
        _, dpnd = a.split("=", 1)
        i_dpnd = i

if dpnd is None:
    sys.stderr.write("missing --dpnd argument\n")
    sys.exit(1)

del argv[i_dpnd]

with subprocess.Popen( [dpnd] + argv[1:], stdout=subprocess.PIPE, stderr=subprocess.STDOUT ) as outproc:
    while True:
        line = outproc.stdout.readline()
        line = line.decode("utf-8")

        #print("got line:", repr(line))

        if ("smt_test_plugin" in line) and (
            "tx:" in line):
            #print("triggered line check")
            q = line.index("tx:")
            json_tx = line[q+3:].strip()
            tx = json.loads(json_tx, object_pairs_hook=collections.OrderedDict)
            p = line.rindex("]", 0, q)
            name = line[p+1:q].strip()
            print("""
######################################
# {0}
######################################

{1}
""".format(name, json.dumps(tx, indent=1, separators=(", ", " : "))))
        elif "listening for http requests" in line:
            #print("triggered term check")
            outproc.terminate()
            outproc.wait()
            break
