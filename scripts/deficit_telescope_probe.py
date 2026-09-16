#!/usr/bin/env python3
"""Probe whether the Phi-vs-P deficit telescopes.

Test on arbitrary subsets of A(n,k), not just Star Graphs.  For each
set V' and each coordinate split, record:

    gap = P_surplus - overhead

where overhead = Phi(V') - sum Phi(F_s) and
      P_surplus = P(R) - sum P(c_s).

Then test candidate telescoping corrections G(state) such that
    Phi(V') + G(state(V')) <= P(R) + G(state(parent))
holds at every split, with G bounded and computable from the
fiber-state data (t, fiber sizes, etc.).
"""

import itertools
import math
import random
from collections import Counter, defaultdict

# ---------------------------------------------------------------------------
# Arithmetic primitives
# ---------------------------------------------------------------------------


def sbl(R):
    """sum(bit_length(i) for 1 <= i < R)."""
    return sum(value.bit_length() for value in range(1, R))


def e_seq(R):
    """Cumulative popcount, OEIS A000788."""
    return sum(value.bit_count() for value in range(R))


def c_constant(R):
    """Hamming-ball collision constant C(R)."""
    if R == 0:
        return 0
    return R - 1 + sbl(R) - e_seq(R)


def potential(R, m):
    """P(R) = C(R) + m * E(R)."""
    return c_constant(R) + m * e_seq(R)


# ---------------------------------------------------------------------------
# Geometry primitives
# ---------------------------------------------------------------------------


def neighbors(vertex, alphabet_size):
    """Neighbors of an injective tuple in the arrangement graph."""
    used = set(vertex)
    result = set()
    for pos in range(len(vertex)):
        for sym in range(alphabet_size):
            if sym not in used:
                result.add(vertex[:pos] + (sym,) + vertex[pos + 1 :])
    return result


def cross_collisions(vertices, n, k):
    """Root-incidence excess over the external vertex boundary."""
    vset = set(vertices)
    total = 0
    external = set()
    for p in range(k):
        directional = set()
        for v in vertices:
            used = set(v)
            for a in range(n):
                if a not in used:
                    w = v[:p] + (a,) + v[p + 1 :]
                    if w not in vset:
                        directional.add(w)
        total += len(directional)
        external |= directional
    return total - len(external)


def defect(vertices, k):
    """R*k minus the total number of occupied coordinate roots."""
    root_count = 0
    for p in range(k):
        root_count += len({v[:p] + v[p + 1 :] for v in vertices})
    return len(vertices) * k - root_count


def phi(vertices, n, k, m):
    """Phi = cross_collisions + (m+1) * defect."""
    return cross_collisions(vertices, n, k) + (m + 1) * defect(vertices, k)


# ---------------------------------------------------------------------------
# Set generators
# ---------------------------------------------------------------------------


def star_graph(n, k, size):
    """Radius-one star centered at tuple(range(k))."""
    center = tuple(range(k))
    available = sorted(neighbors(center, n))
    if size <= 0 or size > 1 + len(available):
        return None
    return [center] + available[: size - 1]


def random_subset(n, k, size, seed):
    """Uniformly random k-permutations of {0..n-1}."""
    verts = list(itertools.permutations(range(n), k))
    rng = random.Random(seed)
    if size > len(verts):
        return None
    return rng.sample(verts, size)


def hamming_ball(n, k, size):
    """Greedy ball: start from center, add nearest neighbors."""
    center = tuple(range(k))
    ball = [center]
    candidate_pool = set(neighbors(center, n))
    while len(ball) < size and candidate_pool:
        best = min(
            candidate_pool,
            key=lambda v: -sum(
                1 for b in ball if sum(a != b_ for a, b_ in zip(v, b)) <= 1
            ),
        )
        ball.append(best)
        candidate_pool.discard(best)
        candidate_pool |= neighbors(best, n) - set(ball)
    if len(ball) < size:
        return None
    return ball


def cliques(n, k, size):
    """Build by greedily adding vertices sharing roots with existing set."""
    center = tuple(range(k))
    ball = [center]
    pool = set(neighbors(center, n)) - {center}
    while len(ball) < size and pool:
        best = max(
            pool,
            key=lambda v: sum(
                1
                for b in ball
                if any(v[:p] + v[p + 1 :] == b[:p] + b[p + 1 :] for p in range(k))
            ),
        )
        ball.append(best)
        pool.discard(best)
        pool |= neighbors(best, n) - set(ball)
    if len(ball) < size:
        return None
    return ball


# ---------------------------------------------------------------------------
# Split analysis
# ---------------------------------------------------------------------------


def analyze_split(vertices, n, k, m, coord):
    """Analyze splitting vertices at coordinate coord."""
    fibers = {}
    for v in vertices:
        fibers.setdefault(v[coord], []).append(v)
    if len(fibers) < 2:
        return None

    R = len(vertices)
    t = len(fibers)
    sizes = tuple(sorted(len(f) for f in fibers.values()))

    phi_V = phi(vertices, n, k, m)
    phi_fibers = sum(phi(f, n, k, m) for f in fibers.values())
    overhead = phi_V - phi_fibers

    P_R = potential(R, m)
    P_fibers = sum(potential(s, m) for s in sizes)
    P_surplus = P_R - P_fibers

    gap = P_surplus - overhead

    return {
        "R": R,
        "t": t,
        "sizes": sizes,
        "m": m,
        "k": k,
        "n": n,
        "overhead": overhead,
        "P_surplus": P_surplus,
        "gap": gap,
        "sbl_surp": sbl(R) - sum(sbl(s) for s in sizes),
        "e_surp": e_seq(R) - sum(e_seq(s) for s in sizes),
        "lin": t - 1,
    }


# ---------------------------------------------------------------------------
# Candidate telescoping corrections
# ---------------------------------------------------------------------------


def G_constant(_state, c=0):
    """G = c (constant). Trivially telescopes but absorbs nothing."""
    return c


def G_num_fibers(state):
    """G = t (number of fibers)."""
    return state["t"]


def G_max_fiber(state):
    """G = max fiber size."""
    return max(state["sizes"]) if state["sizes"] else 0


def G_sum_sizes(state):
    """G = sum of fiber sizes = R (trivially telescopes via R itself)."""
    return sum(state["sizes"])


def G_entropy(state):
    """G = -sum (c_s/R) * log2(c_s/R) * R, a crude entropy measure."""
    R = sum(state["sizes"])
    if R == 0:
        return 0
    ent = 0
    for s in state["sizes"]:
        if s > 0:
            p = s / R
            ent -= p * math.log2(p) * R
    return ent


def G_profile(state):
    """G = number of distinct fiber sizes."""
    return len(set(state["sizes"]))


def G_sbl_profile(state):
    """G = sum sbl(c_s)."""
    return sum(sbl(s) for s in state["sizes"])


def test_telescoping(records, _G_fn, label, verbose=False):
    """Test if Phi(V) + G(state) <= P(R) + G(parent_state) at each split.

    We model G(parent_state) as G(state_of_V_before_split).
    Since state_of_V = (R, sizes_of_fibers), the parent state is
    characterized by R alone (the parent IS V').  So we test:
        Phi(V') + G(t, sizes) <= P(R) + G_parent(R)
    where G_parent(R) is a function only of R (the parent's total size).

    For true telescoping, G_parent must itself be derivable from the
    child states.  The simplest test: does there exist f(R) such that
    for all splits of all R-sets:
        Phi(V') + G(t, sizes) <= P(R) + f(R)?
    And does f(R) satisfy f(R) = G(R) (i.e., the parent's state at
    its own parent level)?
    """
    # Group by (R, m, k)
    grouped = defaultdict(list)
    for r in records:
        grouped[(r["R"], r["m"], r["k"])].append(r)

    worst_by_params = {}
    for (R, m, k), entries in grouped.items():
        deficits = [-e["gap"] for e in entries if e["gap"] < 0]
        if deficits:
            worst_by_params[(R, m, k)] = (max(deficits), len(deficits))

    if verbose:
        print(f"\n  {label}:")
        print(f"    {'R':>4} {'m':>3} {'k':>3} | {'worst_def':>10} {'n_fail':>7}")
        print("    " + "-" * 35)
        for (R, m, k), (worst, nf) in sorted(worst_by_params.items()):
            print(f"    {R:4d} {m:3d} {k:3d} | {worst:10d} {nf:7d}")

    return worst_by_params


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    """Run all telescope probes and print results."""
    print("=== Generating test sets ===")
    records = []

    configs = [(5, 3), (5, 2), (6, 3), (6, 4), (7, 4), (8, 4), (8, 5)]

    for n, k in configs:
        m = n - k
        max_size = min(20, 1 + k * (n - k))  # capacity-valid

        # Star graphs
        for sz in range(4, max_size + 1):
            V = star_graph(n, k, sz)
            if V is None or len(V) < sz:
                continue
            for coord in range(k):
                r = analyze_split(V, n, k, m, coord)
                if r:
                    r["set_type"] = "star"
                    r["coord"] = coord
                    records.append(r)

        # Random subsets
        for sz in range(4, max_size + 1):
            for seed in range(5):
                V = random_subset(n, k, sz, seed)
                if V is None:
                    continue
                for coord in range(k):
                    r = analyze_split(V, n, k, m, coord)
                    if r:
                        r["set_type"] = "random"
                        r["coord"] = coord
                        records.append(r)

        # Hamming balls
        for sz in range(4, max_size + 1):
            V = hamming_ball(n, k, sz)
            if V is None or len(V) < sz:
                continue
            for coord in range(k):
                r = analyze_split(V, n, k, m, coord)
                if r:
                    r["set_type"] = "hamming_ball"
                    r["coord"] = coord
                    records.append(r)

        # High-collision cliques
        for sz in range(4, min(max_size + 1, 12)):
            V = cliques(n, k, sz)
            if V is None or len(V) < sz:
                continue
            for coord in range(k):
                r = analyze_split(V, n, k, m, coord)
                if r:
                    r["set_type"] = "clique"
                    r["coord"] = coord
                    records.append(r)

    print(f"Total splits analyzed: {len(records)}")

    # Overall gap distribution
    gaps = [r["gap"] for r in records]
    neg = [g for g in gaps if g < 0]
    print("\n=== Gap distribution ===")
    print(f"  Total: {len(gaps)}, negative: {len(neg)}")
    if neg:
        print(f"  Worst gap: {min(gaps)}")
        print(f"  Gap histogram: {sorted(Counter(gaps).items())}")

    # Breakdown by set type
    print("\n=== Worst gap by set type ===")

    for stype in ["star", "random", "hamming_ball", "clique"]:
        subset = [r for r in records if r["set_type"] == stype]
        if not subset:
            continue
        sub_gaps = [r["gap"] for r in subset]
        sub_neg = [g for g in sub_gaps if g < 0]
        print(
            f"  {stype:15s}: {len(subset):5d} splits, "
            f"{len(sub_neg):4d} negative, worst={min(sub_gaps)}"
        )

    # Group by (R, m, k) and show worst deficit
    print("\n=== Worst deficit by (R, m, k) ===")
    grouped = defaultdict(list)
    for r in records:
        grouped[(r["R"], r["m"], r["k"])].append(r)

    print(
        f"  {'R':>4} {'m':>3} {'k':>3} | {'worst_gap':>10} {'n_neg':>6} {'n_total':>8}"
    )
    print("  " + "-" * 45)
    for (R, m, k), entries in sorted(grouped.items()):
        sub_gaps = [e["gap"] for e in entries]
        n_neg = sum(1 for g in sub_gaps if g < 0)
        print(
            f"  {R:4d} {m:3d} {k:3d} | {min(sub_gaps):10d}"
            f" {n_neg:6d} {len(sub_gaps):8d}"
        )

    # Show actual worst cases
    print("\n=== Top 15 worst gaps (full detail) ===")
    worst = sorted(records, key=lambda r: r["gap"])[:15]
    for r in worst:
        print(
            f"  A({r['n']},{r['k']}) {r['set_type']:12s}"
            f" R={r['R']:2d} coord={r['coord']}"
            f" t={r['t']:2d} sizes={r['sizes']}"
            f" | overhead={r['overhead']:4d}"
            f" P_surp={r['P_surplus']:4d} gap={r['gap']:3d}"
        )

    # Test telescoping candidates
    print("\n=== Telescoping candidate tests ===")
    candidates = [
        ("G=0 (baseline)", G_constant),
        ("G=t", G_num_fibers),
        ("G=max_size", G_max_fiber),
        ("G=num_profiles", G_profile),
        ("G=sbl_profile", G_sbl_profile),
    ]

    for label, gfn in candidates:
        test_telescoping(records, gfn, label, verbose=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
