#!/usr/bin/env python3
"""Find subsets where the compensated collision inequality is nearly tight."""

from __future__ import annotations

from itertools import combinations, permutations

from lib import (
    boundary_metrics,
    build_fibers,
    c_constant,
    e_seq,
    is_connected,
    neighbors,
)


def find_near_tight(n: int, k: int, size: int, max_slack: int = 2) -> None:
    """Find subsets whose boundary is close to the conjectured bound."""
    vertices = list(permutations(range(n), k))
    adjacency = {vertex: neighbors(vertex, n) for vertex in vertices}
    fibers = build_fibers(vertices, k)

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
