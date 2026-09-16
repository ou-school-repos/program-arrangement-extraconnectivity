#!/usr/bin/env python3
"""Check whether cumulative slack is governed by max branch occupancy.

For a fixed target size R = 1 + sum(counts), every way of distributing the
R-1 leaves across branches (each branch holding at most m leaves) gives a
distinct vertex set in A(k+m,k). This enumerates those partitions for a
range of R, computes X, D, P, and cumulative slack P - (X + (m+1)*D) for
each partition (path-independent, since X and D depend only on the final
vertex set), and reports them grouped by max occupancy (the largest number
of leaves on any single branch).

This is a bounded, explicit-enumeration check across leaf partitions only
-- it does not cover non-Star shapes with non-adjacent leaves (radius > 1
vertices) or partial coordinate sharing beyond the branch structure, so a
clean occupancy trend here is evidence for, not proof of, an occupancy-
driven mechanism.
"""

import argparse
from itertools import combinations_with_replacement


def cross_collisions(vertices, n, k):
    """Return X(vertices), the directional-boundary overlap excess."""
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


def defect(vertices, k):
    """Return D(vertices)."""
    roots = 0
    for position in range(k):
        roots += len(
            {vertex[:position] + vertex[position + 1 :] for vertex in vertices}
        )
    return len(vertices) * k - roots


def potential(size, overhang):
    """Return P(size) = C(size) + overhang * E(size)."""
    if size == 0:
        return 0
    sbl = sum(value.bit_length() for value in range(1, size))
    e_val = sum(value.bit_count() for value in range(size))
    return size - 1 + sbl + (overhang - 1) * e_val


def branch_partitions(leftover, m, max_branches):
    """Yield descending tuples of branch-leaf-counts in 1..m summing to leftover."""
    seen = set()
    for num_branches in range(1, min(max_branches, leftover) + 1):
        for combo in combinations_with_replacement(range(1, m + 1), num_branches):
            if sum(combo) == leftover:
                counts = tuple(sorted(combo, reverse=True))
                if counts not in seen:
                    seen.add(counts)
                    yield counts


def build(counts, k):
    """Build the vertex set: center plus leaves per branch count."""
    center = tuple(range(k))
    vertices = [center]
    for position, count in enumerate(counts):
        for symbol in range(k, k + count):
            vertices.append(center[:position] + (symbol,) + center[position + 1 :])
    return vertices


def sweep(m, r_min, r_max):
    """Evaluate every branch partition at each R and report by occupancy."""
    for r in range(r_min, r_max + 1):
        leftover = r - 1
        max_branches = leftover
        k = max_branches
        n = k + m
        print(f"\n=== R={r}, m={m} (leftover={leftover}, k={k}) ===")
        print(" occ  branches  X     D   cum_slack  counts")
        print("-" * 60)
        rows = []
        for counts in branch_partitions(leftover, m, max_branches):
            vertices = build(counts, k)
            x = cross_collisions(vertices, n, k)
            d = defect(vertices, k)
            p = potential(len(vertices), m)
            cum_slack = p - (x + (m + 1) * d)
            rows.append((max(counts), len(counts), x, d, cum_slack, counts))
        rows.sort(key=lambda row: (row[0], -row[4]))
        for occ, branches, x, d, cum_slack, counts in rows:
            print(f"{occ:4d}  {branches:8d}  {x:4d}  {d:3d}   {cum_slack:9d}  {counts}")


def main():
    """Run the occupancy sweep for the requested m and R range."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--m", type=int, required=True)
    parser.add_argument(
        "--r-range", type=int, nargs=2, metavar=("R_MIN", "R_MAX"), required=True
    )
    args = parser.parse_args()
    r_min, r_max = args.r_range
    if r_min < 2 or r_max < r_min:
        parser.error("require 2 <= R_MIN <= R_MAX")
    sweep(args.m, r_min, r_max)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
