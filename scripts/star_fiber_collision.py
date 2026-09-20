#!/usr/bin/env python3
"""Test the CORRECT dimension-dependent load bound, derived from the proven
identity total_coord_edges_eq (|dV'| + X + Rk = U*(m+1), U = Rk - D):

    X(V') + (m+1)*D(V') <= C(R) + m*E(R)          [[Phi <= C + mE]]

This is algebraically equivalent to the still-open UniversalLowerBound
hypothesis. An advisor review caught that two prior passes (6d05b1e,
2846a52) instead tested the wrong, dimension-INDEPENDENT bound X+D<=C(R)
(the already-superseded CollisionAdjustedBound shape). That bound is
strictly stronger than what UniversalLowerBound needs -- the m*E(R) term
it drops is exactly the slack that saves the Star Graph counterexample
(low defect D buys exactly the m*(E-D) slack that makes Phi<=C+mE hold
even where X+D<=C(R), or X<=C(R) alone, fails).

Result: Phi<=C+mE holds on every whole-set and per-fiber (depth>=1)
configuration tested, including the literal Star Graph counterexample
dimensions (A(15,7), R=8) that killed CollisionAdjustedBound, and larger
cases where the loose X<=C(R) shape fails outright (A(9,5), R=20: X=108
> C=48, yet Phi=203 <= C+mE=208). The only tested candidate that failed
is a naive aggregate, y-coupled fiber-partition correction with a flat
(m+1)*(R-y) term (mimicking E_seq_list_sum_le's structure) -- that
specific correction shape doesn't work, not the underlying target.

Tests four forms, on Star Graphs and dense random subsets:

    whole-set:  X(V') + (m+1)*D(V') <= C(R) + m*E(R)
    per-fiber:  X(F) + (m+1)*D(F) <= C(c) + m*E(c)            (m fixed by n,k)
    aggregate:  X(V') + (m+1)*D(V') <= sum_s [C(c_s)+m*E(c_s)] + (m+1)*(R-y)
                (the y-coupled correction, mirroring E_seq_list_sum_le -- FAILS)
    variant 6:  X(V') <= C(R) alone (no D, no m term) pushed to scale --
                fails outright once the ambient graph has room to grow.

Run:  python3 scripts/star_fiber_collision.py
"""

import itertools
import random

from lib import e_seq, neighbors


def sum_bit_length(R):
    """Return the cumulative bit-length sum below ``R``."""
    return sum(i.bit_length() for i in range(1, R))


def c_constant(R):
    """Return the collision constant for ``R``."""
    if R == 0:
        return 0
    return (R - 1) + sum_bit_length(R) - e_seq(R)


def defect(A, k):
    """Return the coordinate-fiber defect of ``A``."""
    roots_sum = 0
    for p in range(k):
        roots = set(v[:p] + v[p + 1 :] for v in A)
        roots_sum += len(roots)
    return len(A) * k - roots_sum


def cross_collisions(A, n, k):
    """total_coord_edges - |external_neighbors|, computed by generating
    each vertex's coordinate-p neighbors directly (not scanning A(n,k))."""
    Aset = set(A)
    total_coord_edges = 0
    ext = set()
    for p in range(k):
        bset = set()
        for v in A:
            used = set(v)
            for a in range(n):
                if a not in used:
                    w = v[:p] + (a,) + v[p + 1 :]
                    if w not in Aset:
                        bset.add(w)
        total_coord_edges += len(bset)
        ext |= bset
    return total_coord_edges - len(ext)


def unique_roots(A, _n, _k, p):
    """Return the number of distinct roots at coordinate ``p``."""
    return len(set(v[:p] + v[p + 1 :] for v in A))


def star_graph(n, k, size):
    """Return a radius-one star truncated to ``size`` vertices."""
    center = tuple(range(k))
    nbrs = list(neighbors(center, n))
    return [center] + nbrs[: size - 1]


def dense_random(n, k, size, seed):
    """Return a deterministic random sample of arrangement vertices."""
    vertices = list(itertools.permutations(range(n), k))
    rng = random.Random(seed)
    return rng.sample(vertices, min(size, len(vertices)))


def phi_check(A, n, k, m):
    """Whole-set / per-fiber form: X + (m+1)*D <= C(R) + m*E(R)."""
    R = len(A)
    if R < 1:
        return None
    X = cross_collisions(A, n, k)
    D = defect(A, k)
    lhs = X + (m + 1) * D
    rhs = c_constant(R) + m * e_seq(R)
    return lhs, rhs, lhs <= rhs


def check_recursive(A, n, k, m, depth, stats):
    """Record the minimum slack over a set and its coordinate fibers."""
    if len(A) < 1:
        return
    lhs, rhs, _ok = phi_check(A, n, k, m)
    key = "depth0" if depth == 0 else "depth>=1"
    cur = stats.get(key)
    slack = rhs - lhs
    if cur is None or slack < cur[0]:
        stats[key] = (slack, len(A), lhs, rhs, depth)
    for p in range(k):
        fibers = {}
        for v in A:
            fibers.setdefault(v[p], []).append(v)
        if len(fibers) >= 2:
            for F in fibers.values():
                check_recursive(F, n, k, m, depth + 1, stats)
            break


def check_aggregate(A, n, k, m):
    """Aggregate, y-coupled form mirroring E_seq_list_sum_le's structure."""
    R = len(A)
    if R < 2:
        return None
    for p in range(k):
        fibers = {}
        for v in A:
            fibers.setdefault(v[p], []).append(v)
        if len(fibers) < 2:
            continue
        y = unique_roots(A, n, k, p)
        X_total = cross_collisions(A, n, k)
        D_total = defect(A, k)
        lhs = X_total + (m + 1) * D_total
        sum_rhs_parts = sum(
            c_constant(len(F)) + m * e_seq(len(F)) for F in fibers.values()
        )
        rhs = sum_rhs_parts + (m + 1) * (R - y)
        return {"R": R, "y": y, "lhs": lhs, "rhs": rhs, "ok": lhs <= rhs}
    return None


def main():
    """Run whole-set, per-fiber, and aggregate bound checks."""
    print("=== Phi = X + (m+1)D <= C(R) + m*E(R), whole-set / per-fiber ===")
    for n, k in [(4, 2), (5, 3), (5, 2), (6, 3), (6, 2)]:
        m = n - k
        vertices = list(itertools.permutations(range(n), k))
        maxsize = min(len(vertices), 20)
        stats = {}
        for size in range(2, maxsize + 1):
            check_recursive(star_graph(n, k, size), n, k, m, 0, stats)
        for seed in range(5):
            rand_set = dense_random(n, k, min(maxsize, 12), seed)
            check_recursive(rand_set, n, k, m, 0, stats)
        for key in ("depth0", "depth>=1"):
            r = stats.get(key)
            if r is None:
                print(f"A({n},{k}) m={m} {key}: no data")
                continue
            slack, sz, lhs, rhs, depth = r
            flag = "FAIL" if slack < 0 else "ok"
            print(
                f"A({n},{k}) m={m} {key}: worst slack={slack} |F|={sz} "
                f"lhs={lhs} rhs={rhs} actual_depth={depth} [{flag}]"
            )

    print("\n=== Literal Star Graph counterexample check (R=8,n=15,k=7) ===")
    n, k, R = 15, 7, 8
    m = n - k
    A = star_graph(n, k, R)
    lhs, rhs, ok = phi_check(A, n, k, m)
    print(f"A({n},{k}) R={R} m={m}: X+... lhs={lhs} rhs={rhs} ok={ok}")

    print("\n=== Variant 6 (X<=C(R), no D, no m term) pushed to scale ===")
    print("Expected to fail once the hub fiber has room to grow (X grows")
    print("quadratically, C(R) grows R*log(R)) -- confirms X<=C is the")
    print("wrong target, NOT that Phi<=C+mE fails.")
    for n, k in [(6, 3), (7, 4), (8, 4), (9, 5)]:
        m = n - k
        for size in [6, 10, 15, 20]:
            A = star_graph(n, k, size)
            if len(A) < size:
                continue
            X = cross_collisions(A, n, k)
            D = defect(A, k)
            C = c_constant(len(A))
            E = e_seq(len(A))
            phi = X + (m + 1) * D
            rhs_full = C + m * E
            print(
                f"A({n},{k}) m={m} R={len(A)}: X={X} C={C} X<=C:{X <= C}  |  "
                f"Phi={phi} C+mE={rhs_full} Phi<=C+mE:{phi <= rhs_full}"
            )

    print("\n=== Aggregate, y-coupled form ===")
    worst = None
    for n, k in [(4, 2), (5, 3), (5, 2), (6, 3), (6, 2)]:
        m = n - k
        vertices = list(itertools.permutations(range(n), k))
        maxsize = min(len(vertices), 20)
        for size in range(2, maxsize + 1):
            res = check_aggregate(star_graph(n, k, size), n, k, m)
            if res and not res["ok"]:
                print(f"A({n},{k}) star size={size}: {res} [FAIL]")
                worst = "found"
        for seed in range(5):
            A = dense_random(n, k, min(maxsize, 12), seed)
            res = check_aggregate(A, n, k, m)
            if res and not res["ok"]:
                print(f"A({n},{k}) random seed={seed}: {res} [FAIL]")
                worst = "found"
    if worst is None:
        print("No aggregate violations found across all tested configurations.")


if __name__ == "__main__":
    main()
