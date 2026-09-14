#!/usr/bin/env python3
"""Coverage-avoidance whiteboard rig (merged).

Verifies the support-field formulation of the coordinate-local
coverage-avoidance step on small A(n,k), collects per-slice data, and
prints the L_{c,i} envelope table. Pure stdlib.

Run:  python3 scripts/coverage_avoidance.py
"""

import csv
import itertools
import math
import os
import random
from collections import defaultdict


def popcount(j):
    """Binary digit sum."""
    return bin(j).count("1")


def Eseq(R):
    """A000788 partial sums: E(R) = sum_{j<R} popcount(j)."""
    return sum(popcount(j) for j in range(R))


def Cconst(R):
    """Correction constant C(R) = R(d+1) - 2^d - E(R)."""
    if R <= 0:
        return 0
    d = 0 if R == 1 else math.ceil(math.log2(R))
    return R * (d + 1) - (1 << d) - Eseq(R)


def nbrs(v, n):
    """Single-substitution neighbours of v over alphabet [n]."""
    v = list(v)
    out = []
    for p in range(len(v)):
        for a in range(n):
            if a not in v:
                w = list(v)
                w[p] = a
                out.append(tuple(w))
    return out


class Arr:  # pylint: disable=too-few-public-methods
    """Vertex list of the arrangement graph A(n,k)."""

    def __init__(self, n, k):
        self.n, self.k = n, k
        self.V = list(itertools.permutations(range(n), k))


def analyze(A, S, c=0):
    """Full per-set record plus one dict per slice i."""
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
    blo = {}
    for i in range(n):
        b = set()
        for y in Q[i]:
            for z in nbrs(y, n):
                if len(set(z)) == len(z) and z not in Q[i]:
                    b.add(z)
        blo[i] = b
    Lc_i = {i: (P[i] - Q[i]) & blo[i] for i in range(n)}
    Lc = sum(len(v) for v in Lc_i.values())
    # (1) swallow iff, per empty slot
    iff_ok = True
    for i in range(n):
        for x in P[i] - Q[i]:
            swallowed = x in blo[i]
            wit = any(i in Sx.get(y, set()) for y in locN[x] if y in pi)
            if swallowed != wit:
                iff_ok = False
    # (2) gradient bound, per root and global
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
    # (3) recombination identity D(S) - sum D_loc(Q_i) = R - |pi|
    U = sum(len({tuple(v[p] for p in range(k) if p != q) for v in S}) for q in range(k))
    D = k * R - U
    Dloc = {}
    for i in range(n):
        u = 0
        for q in range(k - 1):
            proj = {tuple(x[p] for p in range(k - 1) if p != q) for x in Q[i]}
            u += len(proj)
        Dloc[i] = (k - 1) * len(Q[i]) - u
    recomb = D - sum(Dloc.values())
    conc = R - len(pi)
    # (4) target T - Lc >= mR - dC - m*dE, T = (m+1)|pi| - R
    Ri = [len(Q[i]) for i in range(n)]
    dC = Cconst(R) - sum(Cconst(r) for r in Ri)
    dE = Eseq(R) - sum(Eseq(r) for r in Ri)
    T = (m + 1) * len(pi) - R
    gap = m * R - dC - m * dE
    target_ok = T - Lc >= gap
    # (5) multi-way bonus dE >= R - |pi|
    multi_ok = dE >= conc
    rec = {
        "R": R,
        "pi": len(pi),
        "m": m,
        "iff": iff_ok,
        "perx": perx_ok,
        "Lc": Lc,
        "grad": grad,
        "recomb": recomb,
        "conc": conc,
        "dC": dC,
        "dE": dE,
        "T": T,
        "gap": gap,
        "target": target_ok,
        "multi": multi_ok,
        "suff_fail": (grad + (m + 1) * conc > dC + m * dE),
    }
    rows = [
        {
            "n": n,
            "k": k,
            "m": m,
            "R": R,
            "pi": len(pi),
            "i": i,
            "Ri": len(Q[i]),
            "Pi": len(P[i]),
            "C": Cconst(len(Q[i])),
            "E": Eseq(len(Q[i])),
            "Dloc": Dloc[i],
            "Lci": len(Lc_i[i]),
        }
        for i in range(n)
    ]
    return rec, rows


def corpus():
    """Shared corpus: exhaustive A(4,3) R<=3, random sets, structured sets."""
    random.seed(7)
    A43, A53, A63 = Arr(4, 3), Arr(5, 3), Arr(6, 3)
    out = []
    for R in (2, 3):
        for S in itertools.combinations(A43.V, R):
            out.append((f"A43-R{R}", A43, S))
    specs = [
        (A43, 4, 150),
        (A43, 6, 150),
        (A43, 8, 60),
        (A53, 5, 150),
        (A53, 8, 150),
        (A53, 10, 80),
        (A63, 6, 120),
        (A63, 9, 100),
        (A63, 12, 60),
    ]
    for A, R, cnt in specs:
        for t in range(cnt):
            out.append((f"A{A.n}r-R{R}-{t}", A, tuple(random.sample(A.V, R))))
    for A, R in [(A43, 6), (A53, 8), (A63, 9)]:
        x0 = A.V[0][1:]
        compat = [i for i in range(A.n) if i not in set(x0)]
        fib = tuple(tuple([i]) + tuple(x0) for i in compat[: min(R, len(compat))])
        out.append((f"fiber-{A.n}", A, fib))
        out.append((f"slice-{A.n}", A, tuple(v for v in A.V if v[0] == 0)[:R]))
        out.append((f"HB-{A.n}", A, tuple(sorted(A.V)[:R])))
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
                cands = [c for c in syms if c not in set(x)]
                if cands:
                    S.append(tuple([cands[j % len(cands)]] + list(x)))
            if len(S) >= 3:
                out.append((f"rr-{A.n}-{t}", A, tuple(S)))
    return out


def main():
    """Run verify, collect, envelope, and minimizer phases."""
    recs = []
    srows = []
    fails = 0
    worst_loose = 0.0
    suff_fails = []
    for tag, A, S in corpus():
        r, rows = analyze(A, S)
        recs.append((tag, r, rows))
        srows.extend(rows)
        ok = (
            r["iff"]
            and r["perx"]
            and r["recomb"] == r["conc"]
            and r["target"]
            and r["multi"]
        )
        if not ok:
            print(f"FAIL {tag}: {r}")
            fails += 1
        if r["Lc"] > 0:
            worst_loose = max(worst_loose, r["grad"] / r["Lc"])
        if r["suff_fail"] and r["target"]:
            suff_fails.append((tag, r))
    print(
        f"sets={len(recs)} fails={fails} "
        f"max_grad_over_Lc={worst_loose:.2f} suff_fails={len(suff_fails)}"
    )
    for tag, r in suff_fails[:8]:
        print(
            f"  SUFF-FAIL {tag}: R={r['R']} pi={r['pi']} Lc={r['Lc']} "
            f"grad={r['grad']} conc={r['conc']} dC={r['dC']} dE={r['dE']} "
            f"T={r['T']} gap={r['gap']} T-Lc={r['T']-r['Lc']}"
        )
    # per-slice data: only slices with swallowed slots
    os.makedirs("/tmp/opencode", exist_ok=True)
    csv_path = "/tmp/opencode/slice_data.csv"
    live = [r for r in srows if r["Lci"] > 0]
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(
            f,
            fieldnames=[
                "n",
                "k",
                "m",
                "R",
                "pi",
                "i",
                "Ri",
                "Pi",
                "C",
                "E",
                "Dloc",
                "Lci",
            ],
        )
        w.writeheader()
        w.writerows(live)
    print(f"wrote {csv_path} ({len(live)} rows with Lci>0)")
    # envelope table
    bad = [r for r in live if r["Lci"] > r["Pi"] - r["Ri"]]
    print(f"violations of trivial Lci<=Pi-Ri: {len(bad)}")
    env = defaultdict(list)
    for r in live:
        env[r["Ri"]].append(r)
    print(f"{'Ri':>4} {'n':>5} {'maxLci':>7} {'max(Pi-Ri)':>10} {'maxC+mE':>9}")
    for Ri in sorted(env):
        rs = env[Ri]
        print(
            f"{Ri:>4} {len(rs):>5} {max(r['Lci'] for r in rs):>7} "
            f"{max(r['Pi'] - r['Ri'] for r in rs):>10} "
            f"{max(r['C'] + r['m'] * r['E'] for r in rs):>9}"
        )
    # ascii scatter of Lci vs Ri
    rmax = max(r["Ri"] for r in live)
    lmax = max(r["Lci"] for r in live)
    grid = [[" " for _ in range(rmax + 1)] for _ in range(lmax + 1)]
    for r in live:
        grid[r["Lci"]][r["Ri"]] = "#"
    print(f"Lci ^  (x-axis: Ri 0..{rmax})")
    for L in range(lmax, -1, -1):
        print(f"{L:>3}|{''.join(grid[L])}")
    print("    " + "-" * (rmax + 1))
    # slack-minimizer report. True target slack:
    #   S = dC + m*dE - (m+1)*conc - Lc   (>= 0 iff the target holds)
    scored = []
    for tag, r, rows in recs:
        s = r["dC"] + r["m"] * r["dE"] - (r["m"] + 1) * r["conc"] - r["Lc"]
        scored.append((s, tag, r, rows))
    scored.sort(key=lambda t: t[0])
    nzero = sum(1 for s, _, _, _ in scored if s == 0)
    print(f"min slack={scored[0][0]}  sets with slack==0: {nzero}")

    def show(s, tag, r, rows):
        parts = sorted((q["Ri"] for q in rows if q["Ri"] > 0), reverse=True)
        live_s = [
            (q["i"], q["Ri"], q["Lci"]) for q in rows if q["Ri"] > 0 or q["Lci"] > 0
        ]
        if len(parts) == 1:
            shape = "single"
        elif parts and max(parts) == 1:
            shape = "shattered"
        elif len(parts) == 2:
            shape = "carved"
        else:
            shape = "mixed"
        print(
            f"  slack={s} {tag} R={r['R']} pi={r['pi']} conc={r['conc']} "
            f"Lc={r['Lc']} dC={r['dC']} m*dE={r['m'] * r['dE']} [{shape}]"
        )
        print(f"    parts={parts} slices(i,Ri,Lci)={live_s}")

    for s, tag, r, rows in scored[:12]:
        show(s, tag, r, rows)
    collide = [t for t in scored if t[2]["Lc"] > 0]
    print(f"sets with Lc>0: {len(collide)}  min slack: {collide[0][0]}")
    for s, tag, r, rows in collide[:12]:
        show(s, tag, r, rows)


main()
