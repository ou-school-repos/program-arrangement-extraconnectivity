#!/usr/bin/env python3
"""Exhaustively test the all-subsets UniversalLowerBound on small A(n,k).

This is deliberately separate from ``predict.cpp`` (which evaluates only the
Hamming-ball construction) and ``arrangement.cpp`` (which grows connected
configurations).  UniversalLowerBound quantifies over *every* R-set, so this
script enumerates combinations of all arrangement-graph vertices.

Examples:
  ./scripts/check_universal_lower_bound.py --profile small
  ./scripts/check_universal_lower_bound.py 5 3 4
"""

from __future__ import annotations

import argparse
from itertools import combinations, permutations
from math import comb


def e_seq(size: int) -> int:
    return sum(value.bit_count() for value in range(size))


def c_constant(size: int) -> int:
    return (
        (size - 1) + sum(value.bit_length() for value in range(1, size)) - e_seq(size)
    )


def neighbors(vertex: tuple[int, ...], alphabet_size: int) -> set[tuple[int, ...]]:
    result: set[tuple[int, ...]] = set()
    used = set(vertex)
    for position in range(len(vertex)):
        for symbol in range(alphabet_size):
            if symbol not in used:
                result.add(vertex[:position] + (symbol,) + vertex[position + 1 :])
    return result


def is_connected(
    subset: tuple[tuple[int, ...], ...],
    adjacency: dict[tuple[int, ...], set[tuple[int, ...]]],
) -> bool:
    if len(subset) < 2:
        return True
    target = set(subset)
    seen = {subset[0]}
    frontier = [subset[0]]
    for vertex in frontier:
        for neighbor in adjacency[vertex] & target:
            if neighbor not in seen:
                seen.add(neighbor)
                frontier.append(neighbor)
    return len(seen) == len(target)


def check(n: int, k: int, size: int) -> None:
    vertices = list(permutations(range(n), k))
    adjacency = {vertex: neighbors(vertex, n) for vertex in vertices}
    rhs = max(0, (size * k - e_seq(size)) * (n - k) - c_constant(size))
    minimum = float("inf")
    minimum_is_connected = True
    minimum_disconnected = float("inf")

    for subset in combinations(vertices, size):
        members = set(subset)
        boundary = len(set().union(*(adjacency[vertex] for vertex in subset)) - members)
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

    disconnected = (
        "-" if minimum_disconnected == float("inf") else str(int(minimum_disconnected))
    )
    kind = "connected" if minimum_is_connected else "disconnected"
    print(
        f"A({n},{k}) R={size}: C({len(vertices)},{size})={comb(len(vertices), size)} "
        f"rhs={rhs} min={int(minimum)} ({kind}) min_disconnected={disconnected}: PASS"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("n", type=int, nargs="?")
    parser.add_argument("k", type=int, nargs="?")
    parser.add_argument("size", type=int, nargs="?")
    parser.add_argument("--profile", choices=["small"])
    args = parser.parse_args()
    if args.profile == "small":
        for n, k, maximum_size in ((3, 2, 6), (4, 2, 12), (4, 3, 6), (5, 3, 4)):
            for size in range(1, maximum_size + 1):
                check(n, k, size)
        return
    if None in (args.n, args.k, args.size):
        parser.error("give n k R, or use --profile small")
    check(args.n, args.k, args.size)


if __name__ == "__main__":
    main()
