#!/usr/bin/env python3
"""Restricted cross-collision test: A=Q_i, B=Q_j, i != j, for a fixed sweep
coordinate c=0. Only tests |dA n dB| <= m*min(|A|,|B|) on configurations the
split-and-recombine merge step would actually produce: A = all vertices with
symbol i at coord 0, B = all vertices with symbol j at coord 0, i != j.

Uses prefix sub-slices (Qi[:a], Qj[:b]) as a cheap stand-in for partial
fills rather than exhaustive sub-slice choices.

Run:  python3 scripts/slice_cross_collision.py
"""

import itertools


def neighbors(v, n):
    """All single-substitution neighbors of an injective tuple."""
    used = set(v)
    out = set()
    for p in range(len(v)):
        for a in range(n):
            if a not in used:
                out.add(v[:p] + (a,) + v[p + 1 :])
    return out


def boundary(A, n):
    """External boundary of subset A in A(n,k)."""
    b = set()
    for v in A:
        b |= neighbors(v, n)
    return b - set(A)


def main():
    """Test |∂A ∩ ∂B| ≤ m·min(|A|,|B|) on real Q_i/Q_j slice pairs."""
    for n, k in [(4, 2), (5, 3), (6, 3), (5, 4)]:
        m = n - k
        vertices = list(itertools.permutations(range(n), k))
        # partition by symbol at coord 0
        slices = {i: [v for v in vertices if v[0] == i] for i in range(n)}
        worst = None
        worst_ctx = None
        violations = 0
        pairs = 0
        for i, j in itertools.combinations(range(n), 2):
            Qi, Qj = slices[i], slices[j]
            if not Qi or not Qj:
                continue
            # try all sub-sizes a<=|Qi|, b<=|Qj| by taking prefixes AND random-ish
            # subsets; exhaustive over sub-slices is too expensive, so test the
            # FULL slices (the natural merge unit) plus a few partial sizes.
            for a in range(1, len(Qi) + 1):
                for b in range(1, len(Qj) + 1):
                    # exhaustive over which 'a' elements of Qi / 'b' of Qj is
                    # too expensive for larger n; use first-a/first-b as the
                    # canonical partial-fill representative (matches how a
                    # slice fills up during the merge walk), plus check full.
                    A = Qi[:a]
                    B = Qj[:b]
                    bdA = boundary(A, n)
                    bdB = boundary(B, n)
                    cross = len(bdA & bdB)
                    bound = m * min(a, b)
                    pairs += 1
                    if cross > bound:
                        violations += 1
                        if worst is None or (cross - bound) > worst:
                            worst = cross - bound
                            worst_ctx = (i, j, a, b, cross, bound, A, B)
        print(f"A({n},{k}) m={m}: slice-pairs tested={pairs} violations={violations}")
        if worst_ctx:
            i, j, a, b, cross, bound, A, B = worst_ctx
            print(
                f"  worst: slices i={i},j={j} a={a} b={b} cross={cross} bound={bound}"
            )
            print(f"    A={A}")
            print(f"    B={B}")


if __name__ == "__main__":
    main()
