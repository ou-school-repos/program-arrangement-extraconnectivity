#!/usr/bin/env python3
"""Whiteboard verification rig for the support-field formulation of the
coordinate-local coverage-avoidance step.

Checks on small A(n,k), coordinate c=0:
  (1) swallow iff: empty slot (i,x) swallowed <=> exists y~x in pi with i in S_y
  (2) gradient bound: Lc(x) <= sum_{y~x} |S_y \\ S_x| and Lc <= sum_e |Sx Delta Sy|
  (3) recombination identity: D(S) - sum_i D_loc(Q_i) = R - |pi|   (exact)
  (4) target: T - Lc >= mR - dC - m*dE, with T = (m+1)|pi| - R
  (5) multi-way bonus: dE >= R - |pi|
  (6) hunt: grad + (m+1)(R-|pi|) > dC + m*dE while target holds
       (sufficient-condition failure => gradient too lossy to charge directly)

Pure stdlib. Run:  python3 scripts/whiteboard_check.py
"""

import itertools
import math
import random
from collections import defaultdict


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


def analyze(A, S, c=0):
    n, k, m = A.n, A.k, A.n - A.k
    S = set(S)
    R = len(S)
    rest = [p for p in range(k) if p != c]
    pi = {tuple(v[p] for p in rest) for v in S}
    Sx = defaultdict(set)
    for v in S:
        Sx[tuple(v[p] for p in rest)].add(v[c])
    Q = {}
    for i in range(n):
        Qi = set()
        for v in S:
            if v[c] == i:
                Qi.add(tuple(v[p] for p in rest))
        Q[i] = Qi
    P = {i: {x for x in pi if i not in set(x)} for i in range(n)}
    locN = {}
    for x in pi:
        locN[x] = [y for y in nbrs(x, n) if len(set(y)) == len(y)]
    blo = {i: set() for i in range(n)}  # local boundary of Q_i (full-alphabet sense)
    for i in range(n):
        b = set()
        for y in Q[i]:
            for z in nbrs(y, n):
                if len(set(z)) == len(z) and z not in Q[i]:
                    b.add(z)
        blo[i] = b
    Lc_i = {i: (P[i] - Q[i]) & blo[i] for i in range(n)}
    Lc = sum(len(v) for v in Lc_i.values())
    # (1) iff check per empty slot
    iff_ok = True
    for i in range(n):
        for x in P[i] - Q[i]:
            swallowed = x in blo[i]
            wit = any(i in Sx.get(y, set()) for y in locN[x] if y in pi)
            if swallowed != wit:
                iff_ok = False
    # (2) gradient
    Lcx = {}
    for x in pi:
        compat = [i for i in range(n) if i not in set(x)]
        Lcx[x] = sum(1 for i in compat if i not in Sx[x] and x in blo[i])
    assert sum(Lcx.values()) == Lc
    gradx = {
        x: sum(len(Sx.get(y, set()) - Sx[x]) for y in locN[x] if y in pi) for x in pi
    }
    perx_ok = all(Lcx[x] <= gradx[x] for x in pi)
    edges = []
    for x in pi:
        for y in locN[x]:
            if y in pi and x < y:
                edges.append((x, y))
    grad = sum(len(Sx[x] ^ Sx[y]) for x, y in edges)
    # (3) recombination: D(S) - sum D_loc(Q_i)
    U = sum(len({tuple(v[p] for p in range(k) if p != q) for v in S}) for q in range(k))
    D = k * R - U
    Dloc = {}
    for i in range(n):
        u = 0
        for q in range(k - 1):
            u += len({tuple(x[p] for p in range(k - 1) if p != q) for x in Q[i]})
        Dloc[i] = (k - 1) * len(Q[i]) - u
    recomb = D - sum(Dloc.values())
    conc = R - len(pi)
    # (4) target
    Ri = [len(Q[i]) for i in range(n)]
    dC = Cconst(R) - sum(Cconst(r) for r in Ri)
    dE = Eseq(R) - sum(Eseq(r) for r in Ri)
    T = (m + 1) * len(pi) - R
    gap = m * R - dC - m * dE
    target_ok = T - Lc >= gap
    # (5) multiway
    multi_ok = dE >= conc
    return dict(
        R=R,
        pi=len(pi),
        iff=iff_ok,
        perx=perx_ok,
        Lc=Lc,
        grad=grad,
        recomb=recomb,
        conc=conc,
        dC=dC,
        dE=dE,
        T=T,
        gap=gap,
        target=target_ok,
        multi=multi_ok,
        suff_fail=(grad + (m + 1) * conc > dC + m * dE),
    )


def check(tag, A, S):
    r = analyze(A, S)
    ok = (
        r["iff"]
        and r["perx"]
        and (r["recomb"] == r["conc"])
        and r["target"]
        and r["multi"]
    )
    if not ok:
        print(f"FAIL {tag}: {r}")
        return (r, False)
    return (r, True)


def main():
    random.seed(7)
    fails = 0
    nsets = 0
    worst_loose = 0.0
    suff_fails = []

    def feed(tag, A, S):
        nonlocal fails, nsets, worst_loose
        r, ok = check(tag, A, S)
        nsets += 1
        if not ok:
            fails += 1
        if r["Lc"] > 0:
            worst_loose = max(worst_loose, r["grad"] / r["Lc"])
        if r["suff_fail"] and r["target"]:
            suff_fails.append((tag, r))

    A43 = Arr(4, 3)
    V = A43.V
    # exhaustive R<=3
    for R in (2, 3):
        for S in itertools.combinations(V, R):
            feed(f"A43-R{R}", A43, S)
    # random + structured
    for A, R, n in [(A43, 4, 150), (A43, 6, 150), (A43, 8, 60)]:
        for t in range(n):
            feed(f"A43r-R{R}-{t}", A, random.sample(A.V, R))
    A53 = Arr(5, 3)
    for R, n in [(5, 150), (8, 150), (10, 80)]:
        for t in range(n):
            feed(f"A53r-R{R}-{t}", A53, random.sample(A53.V, R))
    A63 = Arr(6, 3)
    for R, n in [(6, 120), (9, 100), (12, 60)]:
        for t in range(n):
            feed(f"A63r-R{R}-{t}", A63, random.sample(A63.V, R))
    # structured: fiber stacks, slices, Hamming balls, round-robins
    for A, R in [(A43, 6), (A53, 8), (A63, 9)]:
        x0 = A.V[0][1:]
        compat = [i for i in range(A.n) if i not in set(x0)]
        feed(
            f"fiber-{A.n}",
            A,
            [tuple([i] + list(x0)) for i in compat[: min(R, len(compat))]],
        )
        feed(f"slice-{A.n}", A, [v for v in A.V if v[0] == 0][:R])
        feed(f"HB-{A.n}", A, sorted(A.V)[:R])
        # round-robin transversal on a random connected root cluster
        for t in range(40):
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
                cands = [i for i in syms if i not in set(x)]
                if cands:
                    S.append(tuple([cands[j % len(cands)]] + list(x)))
            if len(S) >= 3:
                feed(f"rr-{A.n}-{t}", A, S)
    print(
        f"sets={nsets} fails={fails} "
        f"max_grad_over_Lc={worst_loose:.2f} suff_fails={len(suff_fails)}"
    )
    for tag, r in suff_fails[:8]:
        print(
            f"  SUFF-FAIL {tag}: R={r['R']} pi={r['pi']} Lc={r['Lc']} "
            f"grad={r['grad']} conc={r['conc']} dC={r['dC']} dE={r['dE']} "
            f"T={r['T']} gap={r['gap']} T-Lc={r['T']-r['Lc']}"
        )


main()
