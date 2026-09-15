#!/usr/bin/env python3
"""Test the split-and-recombine cross-collision bound.

Conjecture (for all disjoint A, B ⊂ V(A(n,k))):

    |∂A ∩ ∂B|  ≤  m · min(|A|, |B|)

where m = n - k and ∂A = external neighbors of A.

This is the key inequality for the inductive merge argument: if true,
the cross-collision cost of merging two sub-partitions never exceeds
the arithmetic surplus m · ΔE ≥ m · min(|A|, |B|) provided by the
proven subadditivity lemma E(A+B) ≥ E(A) + E(B) + min(A,B).

Exhaustive over all disjoint pairs (A, B) with |A| = a, |B| = b
for small A(n,k).

Examples:
  ./scripts/cross_collision_bound.py 5 3 2 2
  ./scripts/cross_collision_bound.py --profile quick
  ./scripts/cross_collision_bound.py --profile thorough
"""

from __future__ import annotations

import argparse
import sys
import time
from itertools import combinations


def e_seq(size: int) -> int:
    return sum(value.bit_count() for value in range(size))


def c_constant(size: int) -> int:
    if size == 0:
        return 0
    return (
        (size - 1) + sum(value.bit_length() for value in range(1, size)) - e_seq(size)
    )


def rhs(n: int, k: int, R: int) -> int:
    return (R * k - e_seq(R)) * (n - k) - c_constant(R)


def neighbors(vertex: tuple[int, ...], alphabet_size: int) -> set[tuple[int, ...]]:
    result: set[tuple[int, ...]] = set()
    used = set(vertex)
    for position in range(len(vertex)):
        for symbol in range(alphabet_size):
            if symbol not in used:
                result.add(vertex[:position] + (symbol,) + vertex[position + 1 :])
    return result


def build_adjacency(vertices: list[tuple[int, ...]], n: int):
    adj = {}
    for v in vertices:
        adj[v] = neighbors(v, n)
    return adj


def build_fibers(vertices: list[tuple[int, ...]], k: int):
    fibers = [dict() for _ in range(k)]
    for v in vertices:
        for p in range(k):
            root = v[:p] + v[p + 1 :]
            fibers[p].setdefault(root, set()).add(v)
    return fibers


def boundary_metrics(
    subset: tuple[tuple[int, ...], ...],
    fibers: list[dict[tuple[int, ...], set[tuple[int, ...]]]],
) -> tuple[int, int, int]:
    """Return (|∂V'|, D(V'), X(V')) using the Lean fiber definitions."""
    members = set(subset)
    coordinate_boundaries: list[set[tuple[int, ...]]] = []
    unique_roots = 0
    for position, position_fibers in enumerate(fibers):
        roots = {vertex[:position] + vertex[position + 1 :] for vertex in subset}
        unique_roots += len(roots)
        coordinate_boundaries.append(
            set().union(*(position_fibers[root] for root in roots)) - members
        )
    total_coordinate_boundary = sum(map(len, coordinate_boundaries))
    boundary = len(set().union(*coordinate_boundaries))
    defect = len(subset) * len(fibers) - unique_roots
    collisions = total_coordinate_boundary - boundary
    return boundary, defect, collisions


def run_check(n: int, k: int, a: int, b: int, adj: dict, verbose: bool = False):
    """Check |∂A ∩ ∂B| ≤ m · min(a, b) for all disjoint A, B with |A|=a, |B|=b."""
    m = n - k
    R = a + b
    vertices = list(adj.keys())
    V = set(vertices)

    bound_rhs = m * min(a, b)
    worst_slack: int | None = None
    worst_pair: tuple | None = None
    n_pairs = 0
    n_violations = 0

    # Precompute external boundary for every subset is infeasible;
    # instead iterate over all (A, B) pairs.
    for A_tuple in combinations(vertices, a):
        A = set(A_tuple)
        # Compute ∂A
        bdA: set = set()
        for v in A:
            bdA |= adj[v]
        bdA -= A

        remaining = V - A
        for B_tuple in combinations(remaining, b):
            B = set(B_tuple)
            n_pairs += 1

            # Compute ∂B
            bdB: set = set()
            for v in B:
                bdB |= adj[v]
            bdB -= B

            # Cross-collision overlap
            cross = len(bdA & bdB)

            slack = bound_rhs - cross
            if slack < 0:
                n_violations += 1
                if verbose:
                    print(
                        f"  VIOLATION: |∂A∩∂B|={cross} > {bound_rhs}"
                        f" = m·min(a,b)  (A={sorted(A_tuple)[:3]}...,"
                        f" B={sorted(B_tuple)[:3]}...)"
                    )
            if worst_slack is None or slack < worst_slack:
                worst_slack = slack
                worst_pair = (sorted(A_tuple), sorted(B_tuple))

    return {
        "n": n,
        "k": k,
        "a": a,
        "b": b,
        "m": m,
        "R": R,
        "bound": bound_rhs,
        "n_pairs": n_pairs,
        "n_violations": n_violations,
        "worst_slack": worst_slack,
        "worst_pair": worst_pair,
    }


def run_full_boundary_check(
    n: int, k: int, a: int, b: int, adj: dict, fibers: list, verbose: bool = False
):
    """Check the correct merge inequality with all three overlap terms.

    The identity is:
        |∂(A∪B)| = |∂A| + |∂B| - |∂A ∩ B| - |∂B ∩ A| - |∂A ∩ ∂B|

    So the merge cost has three terms:
        cost = |∂A ∩ B| + |∂B ∩ A| + |∂A ∩ ∂B|

    The inductive bound requires:
        |∂A| + |∂B| - cost  ≥  f(a+b)
        i.e.  cost  ≤  |∂A| + |∂B| - f(a+b)

    Also check the arithmetic version:
        cost  ≤  m·ΔE - ΔC
    where ΔE = E(a+b)-E(a)-E(b), ΔC = C(a+b)-C(a)-C(b).
    """
    m = n - k
    R = a + b
    vertices = list(adj.keys())
    V = set(vertices)
    f_a = rhs(n, k, a)
    f_b = rhs(n, k, b)
    f_R = rhs(n, k, R)
    delta_E = e_seq(R) - e_seq(a) - e_seq(b)
    delta_C = c_constant(R) - c_constant(a) - c_constant(b)
    arith_budget = m * delta_E - delta_C

    worst_slack = None
    n_pairs = 0
    n_violations = 0

    for A_tuple in combinations(vertices, a):
        A = set(A_tuple)
        bdA: set = set()
        for v in A:
            bdA |= adj[v]
        bdA -= A

        remaining = V - A
        for B_tuple in combinations(remaining, b):
            B = set(B_tuple)
            n_pairs += 1

            bdB: set = set()
            for v in B:
                bdB |= adj[v]
            bdB -= B

            cross = len(bdA & bdB)  # |∂A ∩ ∂B|
            ab_hit = len(bdA & B)  # |∂A ∩ B|  (B-vertices adj to A)
            ba_hit = len(bdB & A)  # |∂B ∩ A|  (A-vertices adj to B)
            total_cost = cross + ab_hit + ba_hit

            # ∂(A∪B) = (∂A \ B) ∪ (∂B \ A)
            bdAB = (bdA - B) | (bdB - A)
            bdAB_size = len(bdAB)

            # Check: |∂(A∪B)| ≥ f(a) + f(b) - total_cost
            inductive_bound = f_a + f_b - total_cost
            slack = bdAB_size - inductive_bound

            if slack < 0:
                n_violations += 1
                if verbose:
                    print(
                        f"  VIOLATION:"
                        f" |∂(A∪B)|={bdAB_size} < f(a)+f(b)-cost={inductive_bound}"
                        f"  (cross={cross}, A∩B={ab_hit}, B∩A={ba_hit})"
                    )
            if worst_slack is None or slack < worst_slack:
                worst_slack = slack
                worst_pair = (
                    sorted(A_tuple),
                    sorted(B_tuple),
                    cross,
                    ab_hit,
                    ba_hit,
                    bdAB_size,
                )

    return {
        "n": n,
        "k": k,
        "a": a,
        "b": b,
        "m": m,
        "R": R,
        "f_a": f_a,
        "f_b": f_b,
        "f_R": f_R,
        "delta_E": delta_E,
        "delta_C": delta_C,
        "arith_budget": arith_budget,
        "n_pairs": n_pairs,
        "n_violations": n_violations,
        "worst_slack": worst_slack,
        "worst_pair": worst_pair,
    }


PROFILES = {
    "quick": [
        (4, 2, 1, 1),
        (4, 2, 2, 1),
        (4, 2, 3, 1),
        (5, 3, 1, 1),
        (5, 3, 2, 1),
        (5, 3, 2, 2),
    ],
    "small": [
        (4, 2, 1, 1),
        (4, 2, 2, 1),
        (4, 2, 2, 2),
        (4, 2, 3, 1),
        (5, 3, 1, 1),
        (5, 3, 2, 1),
        (5, 3, 2, 2),
        (5, 3, 3, 1),
    ],
    "medium": [
        (5, 3, 1, 1),
        (5, 3, 2, 1),
        (5, 3, 2, 2),
        (5, 3, 3, 1),
        (5, 3, 3, 2),
        (5, 3, 4, 1),
        (5, 3, 4, 2),
    ],
    "thorough": [(5, 3, a, b) for a in range(1, 5) for b in range(1, 6) if a + b <= 5],
}


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("n", nargs="?", type=int, help="Alphabet size")
    parser.add_argument("k", nargs="?", type=int, help="Tuple length")
    parser.add_argument("a", nargs="?", type=int, help="Size of subset A")
    parser.add_argument("b", nargs="?", type=int, help="Size of subset B")
    parser.add_argument(
        "--profile",
        choices=list(PROFILES.keys()),
        default="quick",
        help="Predefined test profile",
    )
    parser.add_argument("--verbose", "-v", action="store_true")
    parser.add_argument(
        "--full",
        action="store_true",
        help="Also run full boundary merge check (slower)",
    )
    args = parser.parse_args()

    if args.n and args.k and args.a and args.b:
        cases = [(args.n, args.k, args.a, args.b)]
    else:
        cases = PROFILES[args.profile]

    print("=" * 72)
    print("Cross-Collision Bound Test")
    print("Conjecture: |∂A ∩ ∂B| ≤ m · min(|A|, |B|)  for disjoint A, B")
    print("=" * 72)

    total_pairs = 0
    total_violations = 0
    t0 = time.time()

    for n, k, a, b in cases:
        m = n - k
        R = a + b
        vertices = list(v for v in __import__("itertools").permutations(range(n), k))

        if len(vertices) < a + b:
            print(
                f"\n  SKIP A({n},{k}) a={a} b={b}:"
                f" only {len(vertices)} vertices, need {a+b}"
            )
            continue

        adj = build_adjacency(vertices, n)
        E_R = e_seq(R)
        C_R = c_constant(R)
        f_R = (R * k - E_R) * m - C_R

        print(f"\n  A({n},{k})  R={R}  a={a}  b={b}  m={m}")
        print(f"  E({R})={E_R}  C({R})={C_R}  f(R)={f_R}")
        print(f"  Conjectured bound: m·min(a,b) = {m}·{min(a, b)} = {m * min(a, b)}")

        result = run_check(n, k, a, b, adj, verbose=args.verbose)
        total_pairs += result["n_pairs"]
        total_violations += result["n_violations"]

        status = (
            "PASS"
            if result["n_violations"] == 0
            else f"FAIL ({result['n_violations']} violations)"
        )
        print(f"  Pairs tested: {result['n_pairs']:,}")
        print(f"  Result: {status}")
        print(f"  Worst slack: {result['worst_slack']}")
        if result["worst_pair"]:
            print(
                f"  Worst case: A={result['worst_pair'][0][:3]}..."
                f" B={result['worst_pair'][1][:3]}..."
            )

        if args.full:
            print("\n  --- Full boundary merge check ---")
            result2 = run_full_boundary_check(
                n, k, a, b, adj, build_fibers(vertices, k), verbose=args.verbose
            )
            status2 = (
                "PASS"
                if result2["n_violations"] == 0
                else f"FAIL ({result2['n_violations']})"
            )
            print(
                f"  Inductive bound: f({a})+f({b}) = {result2['f_a']}+{result2['f_b']}"
                f" = {result2['f_a']+result2['f_b']}"
            )
            print(f"  |∂(A∪B)| ≥ f(a)+f(b)-X_cross: {status2}")
            print(f"  Worst slack: {result2['worst_slack']}")
            total_pairs += result2["n_pairs"]
            total_violations += result2["n_violations"]

    elapsed = time.time() - t0
    print("\n" + "=" * 72)
    print(
        f"TOTAL: {total_pairs:,} pairs, {total_violations} violations, {elapsed:.1f}s"
    )
    if total_violations > 0:
        print("CONJECTURE IS FALSE")
    else:
        print("CONJECTURE HOLDS for all tested cases")
    print("=" * 72)
    return 1 if total_violations > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
