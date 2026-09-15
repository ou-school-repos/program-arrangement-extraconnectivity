#!/usr/bin/env python3
"""Test whether cross_collisions(F) <= C_constant(|F|) - D(F) survives on
genuine multi-way fiber splits of Star-graph-like configurations (the
proposed n-ary DeltaC lemma), even though the whole-set (l=[R]) version is
provably false by the Star Graph per docs/lean-proof-status.md.

Recursively splits each Star-graph subset at the first disagreeing
coordinate and re-checks the bound at every fiber, at every depth, since
the candidate lemma must hold at every granularity to be usable by
defect_fiber_bound's induction, not just at the top-level split.

Run:  python3 scripts/star_fiber_collision.py
"""
import itertools


def neighbors(v, n):
    used = set(v)
    out = set()
    for p in range(len(v)):
        for a in range(n):
            if a not in used:
                out.add(v[:p] + (a,) + v[p + 1:])
    return out


def e_seq(R):
    total = 0
    for i in range(R):
        total += bin(i).count("1")
    return total


def bit_length(x):
    return x.bit_length()


def sum_bit_length(R):
    return sum(bit_length(i) for i in range(1, R))


def c_constant(R):
    if R == 0:
        return 0
    return (R - 1) + sum_bit_length(R) - e_seq(R)


def defect(A, k):
    # D(A) = |A|*k - sum_unique_roots(A)
    roots_sum = 0
    for p in range(k):
        roots = set(v[:p] + v[p + 1:] for v in A)
        roots_sum += len(roots)
    return len(A) * k - roots_sum


def cross_collisions(A, n, k):
    total_coord_edges = 0
    ext = set()
    per_coord_boundaries = []
    for p in range(k):
        bset = set()
        roots_A = set(v[:p] + v[p + 1:] for v in A)
        for w in itertools.permutations(range(n), k):
            if w in A:
                continue
            if (w[:p] + w[p + 1:]) in roots_A:
                bset.add(w)
        per_coord_boundaries.append(bset)
        total_coord_edges += len(bset)
        ext |= bset
    return total_coord_edges - len(ext)


def star_graph(n, k, size):
    # center vertex + (size-1) neighbors
    vertices = list(itertools.permutations(range(n), k))
    center = vertices[0]
    nbrs = list(neighbors(center, n))
    A = [center] + nbrs[: size - 1]
    return A


def check_recursive(A, n, k, depth=0, worst=None):
    if len(A) < 2:
        return worst
    X = cross_collisions(A, n, k)
    D = defect(A, k)
    C = c_constant(len(A))
    slack = (C - D) - X
    if worst is None or slack < worst[0]:
        worst = (slack, len(A), X, C, D, depth)
    # split by first coordinate where vertices disagree
    for p in range(k):
        fibers = {}
        for v in A:
            fibers.setdefault(v[p], []).append(v)
        if len(fibers) >= 2:
            for F in fibers.values():
                worst = check_recursive(F, n, k, depth + 1, worst)
            break
    return worst


def main():
    for n, k in [(4, 2), (5, 3), (5, 2), (6, 3), (6, 2)]:
        vertices = list(itertools.permutations(range(n), k))
        maxsize = min(len(vertices), 25)
        for size in range(2, maxsize + 1):
            A = star_graph(n, k, size)
            if len(A) < size:
                continue
            worst = check_recursive(A, n, k)
            if worst is None:
                continue
            slack, sz, X, C, D, depth = worst
            flag = "FAIL" if slack < 0 else "ok"
            if slack < 0 or size == maxsize:
                print(
                    f"A({n},{k}) star size={size}: worst fiber |F|={sz} "
                    f"X={X} C={C} D={D} slack={slack} depth={depth} [{flag}]"
                )


if __name__ == "__main__":
    main()
