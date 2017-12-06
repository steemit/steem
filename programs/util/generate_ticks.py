#!/usr/bin/env python3

import json

def compute_stop_initial_digits():

    r = 10**(1.0 / 48)
    u = [int(r**x * 100 + 0.5) for x in range(48)]

    deltas = [u[i] - u[i-1] for i in range(1, len(u))]
    deltas.sort()

    u = [100]
    for d in deltas:
        u.append(u[-1]+d)

    return u

    #for i in range(1, len(u)):
    #    print(u[i] / u[i-1], u[i] - u[i-1])

def compute_tick_list():

    # Powers of 10 that fit in 63 bits
    p10_63 = []
    x = 1
    while True:
        if x > 2**63:
            break
        p10_63.append(x)
        x *= 10

    num_denom_pairs = []
    for denom in reversed(p10_63):
        num_denom_pairs.append((1, denom))
    for num in p10_63[1:]:
        num_denom_pairs.append((num, denom))

    tick_list = []

    stop_initial_digits = compute_stop_initial_digits()
    for num, denom in num_denom_pairs:
        for digits in stop_initial_digits:
            a = num*digits
            b = denom
            if a > 2**63:
                continue
            tick_list.append([a,b])
    return tick_list

if __name__ == "__main__":
    print(json.dumps(compute_tick_list(), separators=(",", ":")))

