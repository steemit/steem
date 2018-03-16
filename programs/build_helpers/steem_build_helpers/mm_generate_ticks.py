#!/usr/bin/env python3

from .mm_generate_millenium import millenium
from .buildj2 import overwrite_if_different

import argparse
import fractions
import json
import os
import sys

bound = 2**63
pow10 = [1]
while True:
    y = pow10[-1]*10
    if y >= bound:
        break
    pow10.append(y)

num_denom = (
     [[1, p] for p in reversed(pow10)]
   + [[p, 1] for p in pow10[1:]]
     )

ticks = []
for n, d in num_denom:
    for t in millenium:
        ticks.append([t*n, d])
sticks = sorted(ticks, key=lambda x : fractions.Fraction(*x))
if ticks != sticks:
    raise RuntimeError("ticks != sticks")
ticks = [[str(n), str(d)] for n, d in ticks if n < bound and d < bound]

json_output = json.dumps({"mm_ticks" : ticks}, separators=(",", ":"))

parser = argparse.ArgumentParser( description="Build the manifest library" )
parser.add_argument( "--output", "-o", type=str, default="-", help="Filename of JSON context file")
args = parser.parse_args()

if args.output == "-":
    sys.stdout.write(json_output)
    sys.stdout.flush()
else:
    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    overwrite_if_different( args.output, json_output )
