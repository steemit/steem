#!/usr/bin/env python3

import argparse
import collections
import datetime
import json
import math
import sys

# how long does it last?

time_fields = set(["days", "hours", "minutes", "seconds", "milliseconds", "microseconds"])

def json_to_seconds(data):
    args = {k : v for k, v in data.items() if k in time_fields}
    delta = datetime.timedelta(**args)
    return delta.total_seconds()

def main(argv):
    parser = argparse.ArgumentParser( description="Compute discount for ephemeral state" )
    parser.add_argument( "--halflife", type=str, default='{"days":90}', help="Halflife JSON" )
    args = parser.parse_args()

    halflife_seconds = json_to_seconds(json.loads(args.halflife))
    k = math.log(2.0) / halflife_seconds

    for line in sys.stdin:
        if line == "":
            break
        json_line = json.loads(line)
        delta_seconds = json_to_seconds(json_line)
        f = lambda x : (1.0 - math.exp(-k*x))
        print(int(10000*f(delta_seconds)+1))
    return

if __name__ == "__main__":
    main(sys.argv)
