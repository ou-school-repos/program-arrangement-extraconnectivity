#!/usr/bin/env python3
"""Dual root-compression averaging check (Strategy 4 / "Bridge 2").

Pinto-style dual compression on A(n,k): a pair of operators C, D, keyed
to a symbol pair (a, b), s.t. neither is individually non-worsening for

    Phi(V') = X(V') + (m+1) * D_defect(V')     (m = n-k)

but the averaging inequality  Phi(V') <= (Phi(C(V')) + Phi(D(V'))) / 2
holds, which would still be enough to drive an induction (iterate,
picking the better of the two each time, to reach a canonical shape
without Phi ever increasing along the chosen branch).

Operators (as recorded in docs/proof-sketch-weighted-potential.md,
"Strategy 4 sketch", first crucible test):

  C(V'): guarded compressSet. For each v containing b, replace b->a;
         skip (leave v unchanged) if v already contains a.
  D(V'): push-up repair. For each v containing b: if v also contains a
         (the guard-blocked case for C), apply the full transposition
         (a b) to the whole vertex instead of skipping; otherwise same
         as C (replace b->a).

Known degenerate case: if *every* vertex in V' contains b, D reduces to
the global automorphism (a b) applied to all of V', which trivially
preserves Phi and proves nothing. This script filters those out and
requires at least one vertex without b, so D does genuine, non-global,
partial relabeling.

Run:  python3 scripts/dual_compression_check.py
"""

import itertools
import random


def neighbors(v, n):
    """Return all one-coordinate substitutions of ``v``."""
    used = set(v)
    out = set()
    for p in range(len(v)):
        for a in range(n):
            if a not in used:
                out.add(v[:p] + (a,) + v[p + 1 :])
    return out


def e_seq(size):
    """Return the cumulative binary popcount below ``size``."""
    return sum(bin(x).count("1") for x in range(size))


def c_constant(size):
    """Return the collision constant for a set of ``size`` vertices."""
    if size <= 0:
        return 0
    return (size - 1) + sum(x.bit_length() for x in range(1, size)) - e_seq(size)


def boundary_metrics(subset, fibers, k):
    """Return external boundary, defect, and cross-collision metrics."""
    members = set(subset)
    coord_b = []
    unique_roots = 0
    for p in range(k):
        roots = {v[:p] + v[p + 1 :] for v in subset}
        unique_roots += len(roots)
        coord_b.append(set().union(*(fibers[p][r] for r in roots)) - members)
    boundary = len(set().union(*coord_b))
    defect = len(subset) * k - unique_roots
    total = sum(len(s) for s in coord_b)
    collisions = total - boundary
    return boundary, defect, collisions


def phi(subset, fibers, k, m):
    _, defect, collisions = boundary_metrics(subset, fibers, k)
    return collisions + (m + 1) * defect


def compress_c(V, a, b):
    """Guarded compressSet: b -> a, skip if a already present."""
    out = []
    for v in V:
        if b in v and a not in v:
            out.append(tuple(a if s == b else s for s in v))
        else:
            out.append(v)
    return tuple(out)


def compress_d(V, a, b):
    """Push-up repair: b -> a normally; full swap (a b) when guard-blocked."""
    out = []
    for v in V:
        if b in v and a in v:
            out.append(tuple(a if s == b else (b if s == a else s) for s in v))
        elif b in v:
            out.append(tuple(a if s == b else s for s in v))
        else:
            out.append(v)
    return tuple(out)


def build_fibers(vertices, k):
    """Build coordinate-root fiber lookup tables."""
    fibers = [{} for _ in range(k)]
    for v in vertices:
        for p in range(k):
            root = v[:p] + v[p + 1 :]
            fibers[p].setdefault(root, set()).add(v)
    return fibers


def best_over_all_pairs(V, fibers, k, m, _n):
    """Try every (a,b) symbol pair; return the best achievable Phi via C or D."""
    p0 = phi(V, fibers, k, m)
    symbols_present = set()
    for v in V:
        symbols_present.update(v)
    best = None
    for a, b in itertools.permutations(symbols_present, 2):
        has_b = any(b in v for v in V)
        no_b = any(b not in v for v in V)
        if not has_b or not no_b:
            continue
        for op in (compress_c, compress_d):
            V1 = op(V, a, b)
            if len(set(V1)) != len(V):
                continue
            if any(len(set(v)) != k for v in V1):
                continue
            p1 = phi(V1, fibers, k, m)
            if best is None or p1 > best:
                best = p1
    return p0, best


def main():
    """Run randomized checks of the dual-compression operators."""
    random.seed(11)
    tested = 0
    avg_holds = 0
    both_worsen = 0
    fails = []
    degenerate_skipped = 0

    for n, k in [(4, 2), (4, 3), (5, 3), (5, 4), (6, 3)]:
        vertices = list(itertools.permutations(range(n), k))
        fibers = build_fibers(vertices, k)
        m = n - k
        for R in range(3, min(9, len(vertices)) + 1):
            trials = 60 if len(vertices) > 40 else min(200, len(vertices) - R + 1)
            seen = set()
            attempts = 0
            while len(seen) < trials and attempts < trials * 8:
                attempts += 1
                V = tuple(sorted(random.sample(vertices, R)))
                if V in seen:
                    continue
                seen.add(V)
                symbols_present = set()
                for v in V:
                    symbols_present.update(v)
                cand_syms = [s for s in range(n) if s in symbols_present]
                if len(cand_syms) < 2:
                    continue
                a, b = random.sample(cand_syms, 2)
                has_b = [v for v in V if b in v]
                no_b = [v for v in V if b not in v]
                if not has_b or not no_b:
                    degenerate_skipped += 1
                    continue
                Vc = compress_c(V, a, b)
                Vd = compress_d(V, a, b)
                # Both must remain valid R-element injective subsets.
                if len(set(Vc)) != R or len(set(Vd)) != R:
                    continue
                if any(len(set(v)) != k for v in Vc) or any(
                    len(set(v)) != k for v in Vd
                ):
                    continue
                p0 = phi(V, fibers, k, m)
                pc = phi(Vc, fibers, k, m)
                pd = phi(Vd, fibers, k, m)
                tested += 1
                avg_ok = p0 <= (pc + pd) / 2
                exists_safe = pc >= p0 or pd >= p0  # what the induction actually needs
                if avg_ok:
                    avg_holds += 1
                if not exists_safe:
                    both_worsen += 1
                if not avg_ok:
                    fails.append((n, k, R, a, b, V, p0, pc, pd, exists_safe))

    exist_fails = [f for f in fails if not f[-1]]
    print(f"non-degenerate trials tested: {tested}")
    print(f"degenerate (all/none contain b) skipped: {degenerate_skipped}")
    print(
        f"averaging inequality Phi(V)<=avg(Phi(C),Phi(D)) holds: {avg_holds}/{tested}"
    )
    print(
        "EXISTENCE (max(PhiC,PhiD) >= Phi0) holds: " f"{tested - both_worsen}/{tested}"
    )
    print(
        f"both C and D strictly decrease Phi (existence fails): {both_worsen}/{tested}"
    )
    print(
        f"averaging failures: {len(fails)} "
        f"(of those, existence also fails: {len(exist_fails)})"
    )
    for n, k, R, a, b, V, p0, pc, pd, safe in fails[:10]:
        tag = "EXIST-OK" if safe else "EXIST-FAIL"
        print(
            f"  {tag} A({n},{k}) R={R} (a,b)=({a},{b}) V'={V} "
            f"Phi0={p0} PhiC={pc} PhiD={pd} avg={(pc+pd)/2}"
        )

    print("\n--- re-checking single-pair EXIST-FAILs against ALL symbol pairs ---")
    single_pair_fails = list(exist_fails)
    still_fail = 0
    checked = 0
    fibers_cache = {}
    for n, k, R, a, b, V, p0, pc, pd, _ in single_pair_fails:
        key = (n, k)
        if key not in fibers_cache:
            vs = list(itertools.permutations(range(n), k))
            fibers_cache[key] = build_fibers(vs, k)
        fibers = fibers_cache[key]
        m = n - k
        p0b, best = best_over_all_pairs(V, fibers, k, m, n)
        assert p0b == p0
        checked += 1
        if best is None or best < p0:
            still_fail += 1
            print(
                f"  STILL FAILS over ALL pairs: A({n},{k}) R={R} V'={V} "
                f"Phi0={p0} best={best}"
            )
    print(f"checked {checked} single-pair-existence-failures against all symbol pairs")
    print(f"still fail (no (a,b,op) rescues it): {still_fail}/{checked}")


if __name__ == "__main__":
    main()
