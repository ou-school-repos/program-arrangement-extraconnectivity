#!/usr/bin/env python3
"""Find subsets where the compensated collision inequality is nearly tight."""

from __future__ import annotations

from itertools import combinations, permutations


def e_seq(size: int) -> int:
    """Return the cumulative popcount on ``0`` through ``size - 1``."""
    return sum(value.bit_count() for value in range(size))


def c_constant(size: int) -> int:
    """Return the collision constant used by the conjectured bound."""
    return (
        (size - 1) + sum(value.bit_length() for value in range(1, size)) - e_seq(size)
    )


def neighbors(vertex: tuple[int, ...], alphabet_size: int) -> set[tuple[int, ...]]:
    """Return the arrangement-graph neighbors of ``vertex``."""
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
    """Return whether the induced subgraph on ``subset`` is connected."""
    if len(subset) < 2:
        return True
    target = set(subset)
    seen = {subset[0]}
    frontier = [subset[0]]
    for vertex in tuple(frontier):
        for neighbor in adjacency[vertex] & target:
            if neighbor not in seen:
                seen.add(neighbor)
                frontier.append(neighbor)
    return len(seen) == len(target)


def boundary_metrics(
    subset: tuple[tuple[int, ...], ...],
    fibers: list[dict[tuple[int, ...], set[tuple[int, ...]]]],
) -> tuple[int, int, int]:
    """Return boundary, defect, and collision metrics for ``subset``."""
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


def find_near_tight(n: int, k: int, size: int, max_slack: int = 2) -> None:
    """Find subsets whose boundary is close to the conjectured bound."""
    vertices = list(permutations(range(n), k))
    adjacency = {vertex: neighbors(vertex, n) for vertex in vertices}
    fibers: list[dict[tuple[int, ...], set[tuple[int, ...]]]] = [{} for _ in range(k)]
    for vertex in vertices:
        for position in range(k):
            root = vertex[:position] + vertex[position + 1 :]
            fibers[position].setdefault(root, set()).add(vertex)

    m = n - k
    es = e_seq(size)
    cc = c_constant(size)
    compensated_rhs = cc - es + (m + 1) * (es)

    hits = []
    for subset in combinations(vertices, size):
        boundary, defect, collisions = boundary_metrics(subset, fibers)
        compensated_slack = compensated_rhs - collisions - (m + 1) * defect
        if 0 <= compensated_slack <= max_slack:
            connected = is_connected(subset, adjacency)
            hits.append(
                (compensated_slack, boundary, defect, collisions, connected, subset)
            )

    hits.sort()
    print(f"\nA({n},{k}) R={size}: near-tight subsets (slack ≤ {max_slack}):")
    for slack, boundary, defect, collisions, connected, subset in hits[:20]:
        conn_str = "connected" if connected else "DISCONNECTED"
        print(
            f"  slack={slack} boundary={boundary} D={defect} X={collisions} "
            f"{conn_str} V'={subset}"
        )
    if not hits:
        print("  (none found)")


def main() -> None:
    """Parse arguments and run the near-tight search."""
    # Cases with small compensated slack
    find_near_tight(5, 2, 5, max_slack=2)
    find_near_tight(5, 2, 6, max_slack=3)
    find_near_tight(6, 2, 5, max_slack=3)
    find_near_tight(6, 2, 6, max_slack=3)
    find_near_tight(4, 3, 5, max_slack=3)
    find_near_tight(5, 4, 3, max_slack=2)
    find_near_tight(5, 3, 4, max_slack=3)
    find_near_tight(6, 5, 2, max_slack=2)


if __name__ == "__main__":
    main()
