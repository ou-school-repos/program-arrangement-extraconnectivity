#!/usr/bin/env python3
"""Per-slice data hunt for the coverage-avoidance envelope.

Builds the same corpus as whiteboard_check.py and writes one row per slice
with L_{c,i} > 0 to /tmp/opencode/slice_data.csv, recording:
  R_i (raw size), |P_i| (transversal pool), C(R_i), E(R_i), D_loc(Q_i).

Pure stdlib. Run:  python3 scripts/slice_hunt.py
Then analyse, e.g.:
  python3 - <<'EOF'
  import csv
  from collections import defaultdict
  rows = list(csv.DictReader('/tmp/opencode/slice_data.csv'))
  env = defaultdict(list)
  for r in rows:
      env[int(r['Ri'])].append(r)
  for Ri in sorted(env):
      rs = env[Ri]
      print(Ri, len(rs), max(int(r['Lci']) for r in rs))
  EOF
"""

import csv
import itertools
import math
import random


def popcount(j):
    return bin(j).count("1")


def Eseq(R):
    return sum(popcount(j) for j in range(R))


def Cconst(R):
    if R <= 0:
        return 0
    d = 0 if R == 1 else math.ceil(math.log2(R))
    return R * (d + 1) - (1 << d) - Eseq(R)


def nbrs(v, n):
    v = list(v)
    out = []
    for p in range(len(v)):
        for a in range(n):
            if a not in v:
                w = list(v)
                w[p] = a
                out.append(tuple(w))
    return out


class Arr:
    def __init__(self, n, k):
        self.n, self.k = n, k
        self.V = list(itertools.permutations(range(n), k))


def slice_rows(A, S, c=0):
    n, k = A.n, A.k
    S = set(S)
    R = len(S)
    rest = [p for p in range(k) if p != c]
    pi = {tuple(v[p] for p in rest) for v in S}
    rows = []
    for i in range(n):
        Qi = {tuple(v[p] for p in rest) for v in S if v[c] == i}
        Ri = len(Qi)
        Pi = {x for x in pi if i not in set(x)}
        bnd = set()
        for y in Qi:
            for z in nbrs(y, n):
                if len(set(z)) == len(z) and z not in Qi:
                    bnd.add(z)
        Lci = len((Pi - Qi) & bnd)
        u = 0
        for q in range(k - 1):
            u += len({tuple(x[p] for p in range(k - 1) if p != q) for x in Qi})
        Dloc = (k - 1) * Ri - u
        rows.append(
            dict(
                n=n,
                k=k,
                m=n - k,
                R=R,
                pi=len(pi),
                i=i,
                Ri=Ri,
                Pi=len(Pi),
                C=Cconst(Ri),
                E=Eseq(Ri),
                Dloc=Dloc,
                Lci=Lci,
            )
        )
    return rows


def corpus():
    random.seed(7)
    A43, A53, A63 = Arr(4, 3), Arr(5, 3), Arr(6, 3)
    out = []
    for R in (2, 3):
        for S in itertools.combinations(A43.V, R):
            out.append((A43, S))
    for A, R, cnt in [
        (A43, 4, 150),
        (A43, 6, 150),
        (A43, 8, 60),
        (A53, 5, 150),
        (A53, 8, 150),
        (A53, 10, 80),
        (A63, 6, 120),
        (A63, 9, 100),
        (A63, 12, 60),
    ]:
        for _ in range(cnt):
            out.append((A, tuple(random.sample(A.V, R))))
    for A, R in [(A43, 6), (A53, 8), (A63, 9)]:
        x0 = A.V[0][1:]
        compat = [i for i in range(A.n) if i not in set(x0)]
        out.append(
            (A, tuple(tuple([i]) + tuple(x0) for i in compat[: min(R, len(compat))]))
        )
        out.append((A, tuple(v for v in A.V if v[0] == 0)[:R]))
        out.append((A, tuple(sorted(A.V)[:R])))
        for _ in range(40):
            start = random.choice(A.V)[1:]
            pi = {start}
            front = [start]
            while len(pi) < R and front:
                x = front.pop()
                ys = [y for y in nbrs(x, A.n) if len(set(y)) == len(y) and y not in pi]
                random.shuffle(ys)
                for y in ys[:2]:
                    if len(pi) < R:
                        pi.add(y)
                        front.append(y)
            pi = list(pi)[:R]
            syms = [0, 1, 2][: min(3, A.n)]
            S = []
            for j, x in enumerate(pi):
                cands = [c for c in syms if c not in set(x)]
                if cands:
                    S.append(tuple([cands[j % len(cands)]] + list(x)))
            if len(S) >= 3:
                out.append((A, tuple(S)))
    return out


with open("/tmp/opencode/slice_data.csv", "w", newline="") as f:
    w = csv.DictWriter(
        f,
        fieldnames=["n", "k", "m", "R", "pi", "i", "Ri", "Pi", "C", "E", "Dloc", "Lci"],
    )
    w.writeheader()
    for A, S in corpus():
        for r in slice_rows(A, S):
            if r["Lci"] > 0:
                w.writerow(r)
print("wrote /tmp/opencode/slice_data.csv")
