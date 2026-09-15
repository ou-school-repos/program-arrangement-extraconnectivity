#!/usr/bin/env python3
"""Corrected re-test of the n-ary collision-ceiling candidate, fixing a bug
the advisor caught: the previous version's "worst" tracker only ever
reported depth=0 (the whole set), so it silently re-derived the known
global Star Graph refutation and never actually tested a genuine fiber
split (depth >= 1).

This version:
  1. Tracks depth==0 and depth>=1 worst slack SEPARATELY, so a depth=0
     failure can no longer masquerade as a fiber-level one.
  2. Tests the untested variant 6: X(F) <= C(|F|) (no -D term).
  3. Tests the AGGREGATE, y-coupled form that actually mirrors
     E_seq_list_sum_le's structure (sum E(c_s) + R - y <= E(R)), since a
     per-fiber bound with no y term is not the real analogue:
         X(V') <= sum_s (C(c_s) - D(F_s)) + (R - y)          [variant 5, agg]
         X(V') <= sum_s C(c_s) + (R - y)                     [variant 6, agg]
     where y = unique_roots at the split coordinate p.
  4. Runs on both Star Graphs (tiny fibers, fast) and denser/random subsets
     (large fibers), since Star Graph fibers degenerate too quickly to
     stress-test depth>=1 at all.

Run:  python3 scripts/star_fiber_collision.py
"""
import itertools
import random


def neighbors(v, n):
    used = set(v)
    out = set()
    for p in range(len(v)):
        for a in range(n):
            if a not in used:
                out.add(v[:p] + (a,) + v[p + 1:])
    return out


def e_seq(R):
    return sum(bin(i).count("1") for i in range(R))


def sum_bit_length(R):
    return sum(i.bit_length() for i in range(1, R))


def c_constant(R):
    if R == 0:
        return 0
    return (R - 1) + sum_bit_length(R) - e_seq(R)


def defect(A, k):
    roots_sum = 0
    for p in range(k):
        roots = set(v[:p] + v[p + 1:] for v in A)
        roots_sum += len(roots)
    return len(A) * k - roots_sum


def cross_collisions(A, n, k):
    total_coord_edges = 0
    ext = set()
    for p in range(k):
        bset = set()
        roots_A = set(v[:p] + v[p + 1:] for v in A)
        for w in itertools.permutations(range(n), k):
            if w in A:
                continue
            if (w[:p] + w[p + 1:]) in roots_A:
                bset.add(w)
        total_coord_edges += len(bset)
        ext |= bset
    return total_coord_edges - len(ext)


def unique_roots(A, n, k, p):
    return len(set(v[:p] + v[p + 1:] for v in A))


def star_graph(n, k, size):
    vertices = list(itertools.permutations(range(n), k))
    center = vertices[0]
    nbrs = list(neighbors(center, n))
    return [center] + nbrs[: size - 1]


def dense_random(n, k, size, seed):
    vertices = list(itertools.permutations(range(n), k))
    rng = random.Random(seed)
    return rng.sample(vertices, min(size, len(vertices)))


def check_recursive(A, n, k, depth, stats):
    """Per-fiber (non-aggregate) variant-5/6 check, depth tracked separately."""
    if len(A) < 2:
        return
    X = cross_collisions(A, n, k)
    D = defect(A, k)
    C = c_constant(len(A))
    slack5 = (C - D) - X
    slack6 = C - X
    key = "depth0" if depth == 0 else "depth>=1"
    for name, slack in (("v5", slack5), ("v6", slack6)):
        cur = stats.setdefault((key, name), None)
        if cur is None or slack < cur[0]:
            stats[(key, name)] = (slack, len(A), X, C, D, depth)
    for p in range(k):
        fibers = {}
        for v in A:
            fibers.setdefault(v[p], []).append(v)
        if len(fibers) >= 2:
            for F in fibers.values():
                check_recursive(F, n, k, depth + 1, stats)
            break


def check_aggregate(A, n, k):
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
        sum_C = sum(c_constant(len(F)) for F in fibers.values())
        sum_D = sum(defect(F, k) for F in fibers.values())
        rhs5 = sum_C - sum_D + (R - y)
        rhs6 = sum_C + (R - y)
        return {
            "R": R, "y": y, "X": X_total,
            "v5_ok": X_total <= rhs5, "v5_rhs": rhs5,
            "v6_ok": X_total <= rhs6, "v6_rhs": rhs6,
        }
    return None


def main():
    print("=== Per-fiber, depth-separated (fixes prior depth-0 bug) ===")
    for n, k in [(4, 2), (5, 3), (5, 2), (6, 3), (6, 2)]:
        vertices = list(itertools.permutations(range(n), k))
        maxsize = min(len(vertices), 20)
        stats = {}
        for size in range(2, maxsize + 1):
            check_recursive(star_graph(n, k, size), n, k, 0, stats)
        for seed in range(5):
            check_recursive(dense_random(n, k, min(maxsize, 12), seed), n, k, 0, stats)
        for key in ("depth0", "depth>=1"):
            for name in ("v5", "v6"):
                r = stats.get((key, name))
                if r is None:
                    print(f"A({n},{k}) {key} {name}: no data")
                    continue
                slack, sz, X, C, D, depth = r
                flag = "FAIL" if slack < 0 else "ok"
                print(
                    f"A({n},{k}) {key} {name}: worst slack={slack} |F|={sz} "
                    f"X={X} C={C} D={D} actual_depth={depth} [{flag}]"
                )

    print("\n=== Aggregate, y-coupled form (real analogue of E_seq_list_sum_le) ===")
    for n, k in [(4, 2), (5, 3), (5, 2), (6, 3), (6, 2)]:
        vertices = list(itertools.permutations(range(n), k))
        maxsize = min(len(vertices), 20)
        for size in list(range(2, maxsize + 1)):
            res = check_aggregate(star_graph(n, k, size), n, k)
            if res:
                print(
                    f"A({n},{k}) star size={size}: R={res['R']} y={res['y']} "
                    f"X={res['X']} v5_rhs={res['v5_rhs']} v5_ok={res['v5_ok']} "
                    f"v6_rhs={res['v6_rhs']} v6_ok={res['v6_ok']}"
                )
        for seed in range(5):
            A = dense_random(n, k, min(maxsize, 12), seed)
            res = check_aggregate(A, n, k)
            if res:
                print(
                    f"A({n},{k}) random seed={seed}: R={res['R']} y={res['y']} "
                    f"X={res['X']} v5_rhs={res['v5_rhs']} v5_ok={res['v5_ok']} "
                    f"v6_rhs={res['v6_rhs']} v6_ok={res['v6_ok']}"
                )


if __name__ == "__main__":
    main()
