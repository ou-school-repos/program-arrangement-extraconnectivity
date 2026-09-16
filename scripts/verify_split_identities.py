#!/usr/bin/env python3
"""Check exact split accounting on bounded pools; finite checks are not proofs."""

import argparse
import itertools
import math
import sys
from collections import defaultdict
from contextlib import nullcontext


def boundary(vertices, adjacency, k):
    """Return external neighbor sets separately for every coordinate direction."""
    members = set(vertices)
    directions = [set() for _ in range(k)]
    for vertex in vertices:
        for position, neighbors in enumerate(adjacency[vertex]):
            directions[position].update(neighbors)
    return tuple(direction - members for direction in directions)


def statistics(vertices, adjacency, k):
    """Compute defect from roots and X from directional union excess."""
    roots = tuple(
        {vertex[:p] + vertex[p + 1 :] for vertex in vertices} for p in range(k)
    )
    directions = boundary(vertices, adjacency, k)
    external = set().union(*directions)
    defect = k * len(vertices) - sum(map(len, roots))
    collisions = sum(map(len, directions)) - len(external)
    return defect, collisions, roots, directions


def potential(size, m):
    """Compute P directly from binary digit sums."""
    e_value = sum(value.bit_count() for value in range(size))
    s_value = sum(value.bit_length() for value in range(1, size))
    return size - 1 + s_value + (m - 1) * e_value


def main():
    """Enumerate a guarded pool and fail immediately with context on mismatch."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, required=True)
    parser.add_argument("--k", type=int, required=True)
    parser.add_argument("--max-r", type=int, required=True)
    parser.add_argument("--max-sets", type=int, default=100000)
    parser.add_argument("--input", help="Tuple file, or - for stdin")
    args = parser.parse_args()
    if not 1 <= args.k <= args.n or args.max_r < 2 or args.max_sets < 1:
        parser.error("require n >= k >= 1, max-r >= 2, max-sets >= 1")
    count = math.perm(args.n, args.k)
    max_r = min(args.max_r, count)
    estimate = sum(math.comb(count, r) for r in range(2, max_r + 1))
    if not args.input and estimate > args.max_sets:
        parser.error(f"pool has {estimate} sets; raise --max-sets explicitly")
    explicit = None
    if args.input:
        try:
            with (
                nullcontext(sys.stdin)
                if args.input == "-"
                else open(args.input, encoding="utf-8")
            ) as source:
                lines = [line.strip() for line in source if line.strip()]
            explicit = tuple(
                (
                    tuple(map(int, line.replace(",", " ").split()))
                    if " " in line or "," in line
                    else tuple(map(int, line))
                )
                for line in lines
            )
        except (OSError, ValueError) as error:
            parser.error(str(error))
        if (
            not explicit
            or len(set(explicit)) != len(explicit)
            or any(
                len(v) != args.k
                or len(set(v)) != args.k
                or any(not 0 <= c < args.n for c in v)
                for v in explicit
            )
        ):
            parser.error("require distinct injective k-tuples in the alphabet")
        max_r = len(explicit)
    vertices = (
        explicit
        if explicit is not None
        else tuple(itertools.permutations(range(args.n), args.k))
    )
    adjacency = {
        vertex: tuple(
            tuple(
                vertex[:p] + (symbol,) + vertex[p + 1 :]
                for symbol in range(args.n)
                if symbol not in vertex
            )
            for p in range(args.k)
        )
        for vertex in vertices
    }
    budgets = [0] + [potential(r, args.n - args.k) for r in range(1, max_r + 1)]
    tested_sets = tested_splits = all_negative = 0
    for size in ([len(explicit)] if explicit is not None else range(2, max_r + 1)):
        size_negative = 0
        pool = (
            [explicit]
            if explicit is not None
            else itertools.combinations(vertices, size)
        )
        for parent in pool:
            parent_d, parent_x, roots, directions = statistics(
                parent, adjacency, args.k
            )
            multiplicities = defaultdict(int)
            for direction in directions:
                for neighbor in direction:
                    multiplicities[neighbor] += 1
            gaps = []
            for position in range(args.k):
                fibers = defaultdict(list)
                for vertex in parent:
                    fibers[vertex[position]].append(vertex)
                children = tuple(tuple(fiber) for fiber in fibers.values())
                child_stats = [statistics(f, adjacency, args.k) for f in children]
                delta_d = parent_d - sum(s[0] for s in child_stats)
                delta_x = parent_x - sum(s[1] for s in child_stats)
                expected_d = size - len(roots[position])
                expected_x = sum(multiplicities[w] >= 2 for w in directions[position])
                if (delta_d, delta_x) != (expected_d, expected_x):
                    print("FAIL", parent, "p=", position, "fibers=", children)
                    print("parent D,X:", parent_d, parent_x)
                    print("child D,X:", [s[:2] for s in child_stats])
                    print(
                        "actual:", delta_d, delta_x, "expected:", expected_d, expected_x
                    )
                    print("directions:", directions, "mu:", dict(multiplicities))
                    return 1
                tested_splits += 1
                if explicit is not None:
                    surplus = budgets[size] - sum(budgets[len(f)] for f in children)
                    gap = surplus - delta_x - (args.n - args.k + 1) * delta_d
                    print(
                        f"p={position}: sizes={sorted(map(len, children))} "
                        f"surplus={surplus} dX={delta_x} dD={delta_d} "
                        f"gap={gap}"
                    )
                if len(children) > 1:
                    surplus = budgets[size] - sum(budgets[len(f)] for f in children)
                    gaps.append(surplus - delta_x - (args.n - args.k + 1) * delta_d)
            if gaps and max(gaps) < 0:
                if all_negative == 0:
                    print("First all-negative set:", parent, "gaps:", gaps)
                all_negative += 1
                size_negative += 1
            tested_sets += 1
        print(f"R={size}: all-negative sets={size_negative}", flush=True)
    print(
        f"PASS A({args.n},{args.k}): sets={tested_sets}, "
        f"splits={tested_splits}, identity mismatches=0, "
        f"all-negative sets={all_negative}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
