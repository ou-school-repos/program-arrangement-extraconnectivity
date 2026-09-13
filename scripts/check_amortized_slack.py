#!/usr/bin/env python3
"""Test the amortized-slack lemma candidate for Proposition 5.3.

Context: docs/proof-sketch-weighted-potential.md, "amortized slack" trailhead
(2026-09-13 session). For a split V' = F_a ⊔ F_b (sizes c_a, c_b, no direct
edges between F_a and F_b), the exact identity

    S(V') = s_A + s_B + Delta - I

holds, where S(X) = |extbd(X)| - rhs(|X|) is the slack of X against the
conjectured minimum rhs(R) = (Rk - E_seq(R))*(n-k) - C_constant(R),
Delta = rhs(c_a) + rhs(c_b) - rhs(c_a+c_b) is the recombination budget, and
I = |extbd(F_a) ∩ extbd(F_b)| is the shared-boundary overcounting.

The critical case for an amortized induction is s_A = s_B = 0 (both fibers
individually tight/optimal, i.e. achieving rhs(c_a), rhs(c_b) exactly --
the induction-hypothesis boundary case). This script exhaustively checks,
for that critical case, whether I ever reaches or exceeds Delta. Verified
so far (by hand, up to size (3,3)): it never does, with growing margin.

Usage:
  ./scripts/check_amortized_slack.py 5 3 4 4      # A(5,3), c_a=c_b=4
  ./scripts/check_amortized_slack.py 6 3 3 4      # A(6,3), c_a=3, c_b=4

Warning: this is exhaustive over all disjoint pairs of tight fibers, which
grows fast. (n,k,c_a,c_b) = (5,3,4,4) or (6,3,3,3)/(6,3,3,4) are the natural
next sizes to try; go further only if a run completes in reasonable time.
"""

from __future__ import annotations

import itertools
import sys
from math import comb


def e_seq(size: int) -> int:
    return sum(v.bit_count() for v in range(size))


def c_constant(size: int) -> int:
    return (
        0
        if size == 0
        else ((size - 1) + sum(v.bit_length() for v in range(1, size)) - e_seq(size))
    )


def rhs(n: int, k: int, R: int) -> int:
    return (R * k - e_seq(R)) * (n - k) - c_constant(R)


def neighbors(vertex: tuple[int, ...], n: int) -> set[tuple[int, ...]]:
    used = set(vertex)
    out: set[tuple[int, ...]] = set()
    for pos in range(len(vertex)):
        for s in range(n):
            if s not in used:
                out.add(vertex[:pos] + (s,) + vertex[pos + 1 :])
    return out


def eb(subset, n: int) -> set[tuple[int, ...]]:
    members = set(subset)
    bd: set[tuple[int, ...]] = set()
    for v in subset:
        bd |= neighbors(v, n)
    return bd - members


def run(n: int, k: int, ca: int, cb: int) -> int:
    verts = list(itertools.permutations(range(n), k))
    target_a = rhs(n, k, ca)
    target_b = rhs(n, k, cb)
    delta = rhs(n, k, ca) + rhs(n, k, cb) - rhs(n, k, ca + cb)

    print(
        f"A({n},{k}) c_a={ca} c_b={cb}: enumerating tight (s=0) fibers "
        f"out of C({len(verts)},{ca})={comb(len(verts), ca):,} / "
        f"C({len(verts)},{cb})={comb(len(verts), cb):,} candidates ...",
        flush=True,
    )

    opt_a = [
        frozenset(c)
        for c in itertools.combinations(verts, ca)
        if len(eb(c, n)) == target_a
    ]
    opt_b = (
        opt_a
        if ca == cb
        else [
            frozenset(c)
            for c in itertools.combinations(verts, cb)
            if len(eb(c, n)) == target_b
        ]
    )

    print(f"  #tight_a={len(opt_a)} #tight_b={len(opt_b)} Delta={delta}", flush=True)

    worst = -(10**9)
    worst_pair = None
    checked = 0
    eb_cache = {}

    def get_eb(fs):
        if fs not in eb_cache:
            eb_cache[fs] = eb(fs, n)
        return eb_cache[fs]

    for i, Fa in enumerate(opt_a):
        ebA = get_eb(Fa)
        for Fb in opt_b:
            if Fa & Fb:
                continue
            ebB = get_eb(Fb)
            I = len(ebA & ebB)
            checked += 1
            if I - delta > worst:
                worst = I - delta
                worst_pair = (Fa, Fb, I)
        if (i + 1) % 200 == 0:
            print(
                f"  ... {i+1}/{len(opt_a)} tight-A fibers done, "
                f"{checked:,} pairs checked so far, running max(I-Delta)={worst}",
                flush=True,
            )

    print(
        f"  checked {checked:,} disjoint tight-fiber pairs; " f"max(I-Delta) = {worst}"
    )
    if worst >= 0:
        print("  *** CRITICAL-CASE VIOLATION (I >= Delta) ***")
        print("  witness:", worst_pair)
    return worst


def main() -> None:
    if len(sys.argv) != 5:
        print(__doc__)
        sys.exit(1)
    n, k, ca, cb = (int(x) for x in sys.argv[1:5])
    run(n, k, ca, cb)


if __name__ == "__main__":
    main()
