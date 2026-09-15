#!/usr/bin/env python3
"""Targeted stress test: does the dual-compression existence gap (Strategy 4)
reappear at m=n-k >= 2 for larger R, or is it confined to m=1?

Tests A(5,3) (m=2) and A(6,3) (m=3) at R in {15, 20, 30}, with random and
Swiss-cheese (dense ball + holes) constructions,
using the FULL existence criterion (best achievable Phi over every valid
(a,b,op), not just one random pair) -- the criterion that actually
matters for the induction.

Run:  python3 scripts/dual_compression_m_stress.py
"""

import itertools
import random

from dual_compression_check import best_over_all_pairs, build_fibers


def neighbors(v, n):
    used = set(v)
    out = set()
    for p in range(len(v)):
        for a in range(n):
            if a not in used:
                out.add(v[:p] + (a,) + v[p + 1 :])
    return out


def swiss_cheese(vertices, adj, n, R, hole_frac, rng):
    """BFS ball of size ~R/(1-hole_frac), then randomly punch holes down to R."""
    start = rng.choice(vertices)
    seen = {start}
    order = [start]
    queue = [start]
    target = int(R / max(1e-9, 1 - hole_frac)) + 5
    while queue and len(order) < target:
        x = queue.pop(0)
        for y in adj[x]:
            if y not in seen:
                seen.add(y)
                order.append(y)
                queue.append(y)
    if len(order) < R:
        return None
    return tuple(rng.sample(order, R))


def main():
    rng = random.Random(23)
    total = 0
    fails = []

    for n, k, Rs in [(5, 3, [15, 20, 30]), (6, 3, [15, 20, 30])]:
        vertices = list(itertools.permutations(range(n), k))
        fibers = build_fibers(vertices, k)
        adj = {v: neighbors(v, n) for v in vertices}
        m = n - k
        for R in Rs:
            if R > len(vertices):
                continue
            trials = []
            for _ in range(15):
                trials.append(tuple(sorted(rng.sample(vertices, R))))
            for hf in (0.1, 0.2, 0.3):
                for _ in range(10):
                    s = swiss_cheese(vertices, adj, n, R, hf, rng)
                    if s:
                        trials.append(s)
            for V in {t for t in trials if len(set(t)) == R}:
                p0, best = best_over_all_pairs(V, fibers, k, m, n)
                total += 1
                if best is None or best < p0:
                    fails.append((n, k, R, V, p0, best))

    print(f"m>=2 stress trials tested: {total}")
    print(f"existence failures (best achievable Phi < Phi0): {len(fails)}")
    for n, k, R, V, p0, best in fails[:15]:
        print(f"  FAIL A({n},{k}) m={n-k} R={R} Phi0={p0} best={best}")
        print(f"    V'={V}")


if __name__ == "__main__":
    main()
