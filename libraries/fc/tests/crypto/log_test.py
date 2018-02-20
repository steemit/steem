#!/usr/bin/env python3

# Independent implementation of algorithm
# To create log_test.txt, run ./log_test.py > log_test.txt

import random

rand = random.Random(1234)

result = set()

result.add((0, 256))
result.add(((1 << 256)-1, 0))
for i in range(256):
    y = (1 << i)
    result.add((y, 255-i))
    for j in range(32):
        result.add((y+rand.randrange(0, y), 255-i))

def get_sem_32(y):
    bs = "{:0256b}".format(y)
    if "1" not in bs:
        return 0
    bs += 32*"0"
    i = bs.index("1")
    return ((255-i) << 24) | int(bs[i+1:i+25], 2)

for y, lz in sorted(result):
    print("{:02x}".format(lz), "{:064x}".format(y), "{:08x}".format(get_sem_32(y)))
