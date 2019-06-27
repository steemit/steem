#!/usr/bin/env python3

import datetime
import json
import sys

import matplotlib
matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.ticker import AutoMinorLocator

x = []
y = []

names = ["curate", "content", "producer", "liquidity", "pow"]
inflections = {}
markers = []

colors = iter("mgbr")
shapes = iter("ovx+")

ax = plt.gca()

plt.axis([0,10,5e9,320e9])
ax.set_xticks(range(11))
ax.xaxis.set_minor_locator(AutoMinorLocator(12))
ax.set_yscale("log")
ax.tick_params(axis="y", which="minor", left="off", right="off")
ax.set_yticks([10e6, 20e6, 40e6, 80e6, 160e6, 320e6, 640e6, 1300e6, 2600e6, 5200e6, 10e9, 20e9, 40e9, 80e9, 160e9, 320e9])
ax.set_yticklabels(["10M", "20M", "40M", "80M", "160M", "320M", "640M", "1.3B", "2.6B", "5.2B", "10B", "20B", "40B", "80B", "160B", "320B"])
plt.grid(True, which="major", linestyle="-")
ax.xaxis.grid(True, which="minor", linestyle="-", color="g")

BLOCKS_PER_YEAR = 20*60*24*365

with open(sys.argv[1], "r") as f:
    n = 0
    for line in f:
        n += 1
        d = json.loads(line)
        b = int(d["b"])
        s = int(d["s"])

        if (b%10000) != 0:
            continue

        px = b / BLOCKS_PER_YEAR
        py = s / 1000

        x.append(px)
        y.append(py)
        for i in range(len(names)):
            if i == 1:
                continue
            if names[i] in inflections:
                continue
            if (int(d["rvec"][i*2])%1000) == 0:
                continue
            inflections[names[i]] = d["b"]
            markers.append([[[px],[py],next(colors)+next(shapes)], {"label" : names[i]}])

plt.plot(x, y)
for m in markers:
    print(m)
    plt.plot(*m[0], **m[1])
plt.legend(loc=4)
plt.title("10-year STEEM supply projection")
plt.savefig("myfig.png")
