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
from collections import defaultdict
from functools import lru_cache


def arrangement_vertices(n, k):
    """Return the vertices of A(n,k) as injective tuples."""
    return tuple(itertools.permutations(range(n), k))


@lru_cache(maxsize=None)
def cross_collisions(vertices, n, k):
    """Return X(V), the coordinate-boundary overlap excess."""
    members = set(vertices)
    directional_total = 0
    external = set()
    for position in range(k):
        directional = set()
        for vertex in vertices:
            used = set(vertex)
            for symbol in range(n):
                if symbol in used:
                    continue
                neighbor = vertex[:position] + (symbol,) + vertex[position + 1 :]
                if neighbor not in members:
                    directional.add(neighbor)
        directional_total += len(directional)
        external |= directional
    return directional_total - len(external)


def split(vertices, position):
    """Return nonempty coordinate fibers in canonical order."""
    fibers = defaultdict(list)
    for vertex in vertices:
        fibers[vertex[position]].append(vertex)
    return tuple(tuple(sorted(fiber)) for _, fiber in sorted(fibers.items()))


def main():
    """Parse arguments, enumerate splits, and report envelope violations."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("n", type=int)
    parser.add_argument("k", type=int)
    parser.add_argument("--max-r", type=int, required=True)
    parser.add_argument("--show", type=int, default=5)
    args = parser.parse_args()

    if args.n < 1 or args.k < 1 or args.k > args.n or args.max_r < 2:
        parser.error("require n >= k >= 1 and --max-r >= 2")

    m = args.n - args.k
    vertices = arrangement_vertices(args.n, args.k)
    max_r = min(args.max_r, len(vertices))
    tested = failures = 0
    max_excess = None
    examples = []

    print(
        f"Testing Delta_X <= (m-1)(c_max-1) on A({args.n},{args.k}), "
        f"m={m}, through R={max_r}."
    )
    for size in range(2, max_r + 1):
        for combination in itertools.combinations(vertices, size):
            parent = tuple(combination)
            parent_x = cross_collisions(parent, args.n, args.k)
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

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
