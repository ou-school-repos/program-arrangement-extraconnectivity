#!/usr/bin/env python3
"""Test whether pairwise coordinate incidences determine a Delta-X split menu.

For every V through a size cap, form the complete family of labelled pairwise
coordinate contingency tables.  Symbol names are quotiented by a simultaneous
global alphabet relabelling; coordinate positions remain labelled so that the
Delta-X values for corresponding coordinate splits can be compared.

For each resulting finite pairwise state, this script tests whether all member
sets have the same tuple of per-coordinate Delta-X values.  A pass is finite
evidence only.  A failure is an explicit witness that pairwise incidence data
does not determine the Delta-X menu.
"""

import argparse
import itertools
from collections import defaultdict
from functools import lru_cache


def arrangement_vertices(n, k):
    """Return all injective k-tuples over range(n)."""
    return tuple(itertools.permutations(range(n), k))


@lru_cache(maxsize=None)
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


def split(vertices, position):
    """Return coordinate fibers in a canonical order."""
    fibers = defaultdict(list)
    for vertex in vertices:
        fibers[vertex[position]].append(vertex)
    return tuple(tuple(sorted(fiber)) for _, fiber in sorted(fibers.items()))


def raw_pairwise_signature(vertices, n, k, relabel):
    """Return labelled pairwise contingency tables after one symbol relabelling."""
    signature = []
    for first in range(k):
        for second in range(first + 1, k):
            table = [0] * (n * n)
            for vertex in vertices:
                row = relabel[vertex[first]]
                col = relabel[vertex[second]]
                table[row * n + col] += 1
            signature.append(tuple(table))
    return tuple(signature)


@lru_cache(maxsize=None)
def pairwise_state(vertices, n, k):
    """Canonicalize all pairwise tables under one global alphabet permutation."""
    return min(
        raw_pairwise_signature(vertices, n, k, relabel)
        for relabel in itertools.permutations(range(n))
    )


def delta_x_menu(vertices, n, k):
    """Return Delta-X by coordinate, or None for a trivial split."""
    parent_x = cross_collisions(vertices, n, k)
    menu = []
    for position in range(k):
        children = split(vertices, position)
        if len(children) < 2:
            menu.append(None)
            continue
        menu.append(parent_x - sum(cross_collisions(child, n, k) for child in children))
    return tuple(menu)


def main():
    """Enumerate a finite cell and compare Delta-X menus within pairwise states."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("n", type=int)
    parser.add_argument("k", type=int)
    parser.add_argument("--max-r", required=True, type=int)
    parser.add_argument("--show", default=5, type=int)
    args = parser.parse_args()
    if args.n < 1 or args.k < 2 or args.k > args.n or args.max_r < 1:
        parser.error("require n >= k >= 2 and --max-r >= 1")
    if args.n > 7:
        parser.error("n > 7 is disabled: global relabelling costs n! per set")

    vertices = arrangement_vertices(args.n, args.k)
    max_r = min(args.max_r, len(vertices))
    representatives = {}
    member_counts = defaultdict(int)
    conflicts = []
    total_sets = 0

    print(
        "Testing pairwise-state determination of Delta_X menus on "
        f"A({args.n},{args.k}) "
        f"through R={max_r}."
    )
    for size in range(1, max_r + 1):
        for combination in itertools.combinations(vertices, size):
            current = tuple(combination)
            state = pairwise_state(current, args.n, args.k)
            menu = delta_x_menu(current, args.n, args.k)
            total_sets += 1
            member_counts[state] += 1
            if state not in representatives:
                representatives[state] = (current, menu)
            elif representatives[state][1] != menu and len(conflicts) < args.show:
                previous, previous_menu = representatives[state]
                conflicts.append((state, previous, previous_menu, current, menu))

    multi_states = sum(count > 1 for count in member_counts.values())
    print(f"Sets checked: {total_sets}")
    print(f"Pairwise states: {len(member_counts)}")
    print(f"States with multiple realizations: {multi_states}")
    print(f"Conflicting Delta_X menus: {len(conflicts)}")
    for _, first_set, first_menu, second_set, second_menu in conflicts:
        print("FAIL")
        print(f"  V1={first_set} menu={first_menu}")
        print(f"  V2={second_set} menu={second_menu}")

    return 1 if conflicts else 0


if __name__ == "__main__":
    raise SystemExit(main())
