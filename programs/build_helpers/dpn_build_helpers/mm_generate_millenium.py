#!/usr/bin/env python3

import fractions

"""
Compute a set of 3-digit numbers called ticks.

The set of ticks is called the *millenium* because it goes up to 1000.

- All multiples of 100 between 0 and 1000 are ticks.
- All ticks are multiples of 5.
- For any two successive multiples of 100, the gap between any two
successive ticks can have at most one of two distinct values.
- Any two successive ticks have a ratio of at most 6%.

The algorithm is as follows:

- For a, b each pair of multiples of 5 less than 100, by brute force find a_count, b_count such that a*a_count + b*b_count = 100
- For each such sequence, create a sum sequence for each base a multiple of 100 less than 1000
- The sum sequence for a, a_count, b, b_count, base = [15, 4, 20, 2, 300] is [300, 315, 330, 345, 360, 380, 400]
- Define a sum sequence's score to be the greatest ratio between two successive elements
- Drop each sum sequence whose score is greater than bound = 1.06
- For each base value, pick the shortest remaining sum sequence, using score to break any tie

Annotated output is as follows:

[
100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170, 180, 190,     # 100 : 5/10
200, 210, 220, 230, 240, 250, 260, 270, 285,                                             # 200 : 10/15
300, 315, 330, 345, 360, 380,                                                            # 300 : 15/20
400, 420, 440, 460, 480,                                                                 # 400 : 20
500, 525, 550, 575,                                                                      # 500 : 25
600, 630, 665,                                                                           # 600 : 30/35
700, 730, 765,                                                                           # 700 : 30/35
800, 830, 865,                                                                           # 800 : 30/35
900, 950                                                                                 # 900 : 50
]
"""

def generate_counts():
    m5 = list(range(5, 100, 5))

    for i in range(len(m5)):
        for j in range(i+1, len(m5)):
            a = m5[i]
            b = m5[j]
            n = (100 // b)+2
            for b_count in range(n):
                a_count = (100 - b_count * b) // a
                if a_count < 0:
                    continue
                if a*a_count + b*b_count != 100:
                    continue
                if a_count == 0:
                    yield ((b_count, b), )
                elif b_count == 0:
                    yield ((a_count, a), )
                else:
                    yield ((a_count, a), (b_count, b))

def sequence_from_counts(counts):
    s = 0
    for n, k in counts:
        for i in range(n):
            yield s
            s += k

def compute_sequences():
    seqs_uniq = []
    s = set()
    for c in generate_counts():
        seq = tuple(sequence_from_counts(c))
        if seq not in s:
            s.add(seq)
            seqs_uniq.append(seq)
    return seqs_uniq

def score_sequence(base, s):
    u = [e+base for e in s]
    n = len(u)
    u.append(base+100)
    return max(fractions.Fraction(u[i+1], u[i]) for i in range(n))

def score_sequences(base):
    for s in compute_sequences():
        yield (score_sequence(base, s), s)

#bound = fractions.Fraction(1068, 1000)
bound = fractions.Fraction(1060, 1000)

millenium = []

for base in range(100, 1000, 100):
    passing_seqs = [(score, seq) for score, seq in score_sequences(base) if score < bound]
    best_score, best_seq = min(passing_seqs, key=lambda ss: (len(ss[1]), ss[0]))
    millenium.extend([x+base for x in best_seq])

if __name__ == "__main__":
    print(millenium)
