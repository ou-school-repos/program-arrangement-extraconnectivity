#!/usr/bin/env python3
"""Falsify or support a proposed local cross-collision envelope.

For every subset V of A(n,k) through a cardinality cap and every nontrivial
coordinate split p, test

    Delta_X(V,p) <= (m - 1) * (c_max - 1),   m = n-k,

where c_max is the largest fiber of the split.  The right-hand side agrees
with Delta_X on a full-Star spine, but this script deliberately tests arbitrary
geometry.  Passing a finite pool is evidence only; a failure is an explicit
counterexample to this particular envelope.
"""

import argparse
import itertools

from lib import cross_collisions, split


def arrangement_vertices(n, k):
    """Return the vertices of A(n,k) as injective tuples."""
    return tuple(itertools.permutations(range(n), k))


def pairwise_cross_corners(vertices, k):
    """Return Q(V), the sum of valid empty pairwise projection corners.

    For each labelled coordinate pair, take two occupied projection cells in
    different rows and columns.  Each unoccupied crossed cell whose two symbols
    are distinct is counted once.  Q deliberately retains only pairwise data;
    it is a proposed diagnostic statistic, not assumed to equal X.
    """
    total = 0
    for first in range(k):
        for second in range(first + 1, k):
            cells = {(vertex[first], vertex[second]) for vertex in vertices}
            corners = set()
            for (row_a, col_a), (row_b, col_b) in itertools.combinations(cells, 2):
                if row_a == row_b or col_a == col_b:
                    continue
                for corner in ((row_a, col_b), (row_b, col_a)):
                    if corner[0] != corner[1] and corner not in cells:
                        corners.add(corner)
            total += len(corners)
    return total


def main():
    """Parse arguments, enumerate splits, and report envelope violations."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("n", type=int)
    parser.add_argument("k", type=int)
    parser.add_argument("--max-r", type=int, required=True)
    parser.add_argument("--show", type=int, default=5)
    parser.add_argument(
        "--test-pairwise-q",
        action="store_true",
        help="also compare the pairwise cross-corner statistic Q(V) with X(V)",
    )
    args = parser.parse_args()

    if args.n < 1 or args.k < 1 or args.k > args.n or args.max_r < 2:
        parser.error("require n >= k >= 1 and --max-r >= 2")

    m = args.n - args.k
    vertices = arrangement_vertices(args.n, args.k)
    max_r = min(args.max_r, len(vertices))
    tested = failures = 0
    max_excess = None
    examples = []
    pairwise_min_difference = None
    pairwise_max_difference = None
    pairwise_examples = []

    print(
        f"Testing Delta_X <= (m-1)(c_max-1) on A({args.n},{args.k}), "
        f"m={m}, through R={max_r}."
    )
    for size in range(2, max_r + 1):
        for combination in itertools.combinations(vertices, size):
            parent = tuple(combination)
            parent_x = cross_collisions(parent, args.n, args.k)
            if args.test_pairwise_q:
                q_value = pairwise_cross_corners(parent, args.k)
                q_difference = q_value - parent_x
                if (
                    pairwise_min_difference is None
                    or q_difference < pairwise_min_difference
                ):
                    pairwise_min_difference = q_difference
                if (
                    pairwise_max_difference is None
                    or q_difference > pairwise_max_difference
                ):
                    pairwise_max_difference = q_difference
                if q_difference != 0 and len(pairwise_examples) < args.show:
                    pairwise_examples.append((parent, parent_x, q_value))
            for position in range(args.k):
                children = split(parent, position)
                if len(children) < 2:
                    continue
                delta_x = parent_x - sum(
                    cross_collisions(child, args.n, args.k) for child in children
                )
                sizes = tuple(sorted((len(child) for child in children), reverse=True))
                envelope = (m - 1) * (sizes[0] - 1)
                excess = delta_x - envelope
                tested += 1
                if max_excess is None or excess > max_excess:
                    max_excess = excess
                if excess > 0:
                    failures += 1
                    if len(examples) < args.show:
                        examples.append(
                            (parent, position, sizes, delta_x, envelope, excess)
                        )

    print(f"Splits tested: {tested}")
    print(f"Maximum Delta_X - envelope: {max_excess}")
    print(f"Violations: {failures}")
    for parent, position, sizes, delta_x, envelope, excess in examples:
        print("FAIL")
        print(f"  V={parent}")
        print(
            f"  p={position} sizes={sizes} Delta_X={delta_x} "
            f"envelope={envelope} excess={excess}"
        )

    if args.test_pairwise_q:
        print(
            f"Q(V) - X(V) range: [{pairwise_min_difference}, {pairwise_max_difference}]"
        )
        print(f"Q(V) != X(V) examples: {len(pairwise_examples)}")
        for parent, x_value, q_value in pairwise_examples:
            print("PAIRWISE-Q MISMATCH")
            print(f"  V={parent}")
            print(f"  X={x_value} Q={q_value} Q-X={q_value - x_value}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
