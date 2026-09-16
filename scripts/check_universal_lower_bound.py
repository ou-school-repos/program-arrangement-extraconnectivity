#!/usr/bin/env python3
"""Exhaustively test the all-subsets UniversalLowerBound on small A(n,k).

This is deliberately separate from ``predict.cpp`` (which evaluates only the
Hamming-ball construction) and ``arrangement.cpp`` (which grows connected
configurations).  UniversalLowerBound quantifies over *every* R-set, so this
script enumerates combinations of all arrangement-graph vertices.

Examples:
  ./scripts/check_universal_lower_bound.py --profile small
  ./scripts/check_universal_lower_bound.py --profile extended
  ./scripts/check_universal_lower_bound.py 5 3 4
"""

from __future__ import annotations

import argparse
from itertools import combinations
from math import comb

from lib import (arrangement_vertices, boundary_metrics, build_fibers,
                 c_constant, e_seq, is_connected, neighbors)


def check(n: int, k: int, size: int) -> None:
    """Check the universal lower bound for one arrangement-graph size."""
    vertices = arrangement_vertices(n, k)
    adjacency = {vertex: neighbors(vertex, n) for vertex in vertices}
    fibers = build_fibers(vertices, k)
    rhs = max(0, (size * k - e_seq(size)) * (n - k) - c_constant(size))
    minimum = float("inf")
    minimum_is_connected = True
    minimum_disconnected = float("inf")
    minimum_compensated_slack = float("inf")

    for subset in combinations(vertices, size):
        boundary, defect, collisions = boundary_metrics(subset, fibers)
        connected = is_connected(subset, adjacency)
        if boundary < minimum:
            minimum = boundary
            minimum_is_connected = connected
        if not connected:
            minimum_disconnected = min(minimum_disconnected, boundary)
        if boundary < rhs:
            raise AssertionError(
                f"counterexample: A({n},{k}), R={size}, V'={subset}, "
                f"boundary={boundary}, lower_bound={rhs}, connected={connected}"
            )
        # When the target's natural-number right side is positive, the exact
        # fiber identity makes the boundary inequality equivalent to
        # X(V) ≤ C(R)-E(R)+(n-k+1)(E(R)-D(V)).  Check that candidate directly,
        # rather than confusing X with the outward edge count.
        if rhs > 0:
            compensated_rhs = (
                c_constant(size) - e_seq(size) + (n - k + 1) * (e_seq(size) - defect)
            )
            slack = compensated_rhs - collisions
            minimum_compensated_slack = min(minimum_compensated_slack, slack)
            if slack < 0:
                raise AssertionError(
                    f"compensated-collision counterexample: A({n},{k}), R={size}, "
                    f"V'={subset}, D={defect}, X={collisions}, "
                    f"candidate_rhs={compensated_rhs}"
                )

    disconnected = (
        "-" if minimum_disconnected == float("inf") else str(int(minimum_disconnected))
    )
    kind = "connected" if minimum_is_connected else "disconnected"
    compensated = (
        "n/a (truncated target)"
        if minimum_compensated_slack == float("inf")
        else str(int(minimum_compensated_slack))
    )
    print(
        f"A({n},{k}) R={size}: C({len(vertices)},{size})={comb(len(vertices), size)} "
        f"rhs={rhs} min={int(minimum)} ({kind}) min_disconnected={disconnected} "
        f"min_compensated_slack={compensated}: PASS"
    )


def main() -> None:
    """Parse command-line options and run the requested exhaustive checks."""
    parser = argparse.ArgumentParser()
    parser.add_argument("n", type=int, nargs="?")
    parser.add_argument("k", type=int, nargs="?")
    parser.add_argument("size", type=int, nargs="?")
    parser.add_argument("--profile", choices=["small", "extended"])
    args = parser.parse_args()
    if args.profile:
        cases = [(3, 2, 6), (4, 2, 12), (4, 3, 6), (5, 3, 4)]
        if args.profile == "extended":
            # Deliberately remains exhaustive: the largest row is C(42, 5).
            cases.extend([(6, 2, 5), (7, 2, 3), (7, 2, 4), (7, 2, 5)])
        for n, k, maximum_size in cases:
            for size in range(1, maximum_size + 1):
                check(n, k, size)
        return
    if None in (args.n, args.k, args.size):
        parser.error("give n k R, or use --profile small")
    check(args.n, args.k, args.size)


if __name__ == "__main__":
    main()
