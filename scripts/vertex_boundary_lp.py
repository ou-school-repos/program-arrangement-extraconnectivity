#!/usr/bin/env python3
"""Fractional vertex-boundary LP for arrangement graphs.

Usage: ./scripts/vertex_boundary_lp.py [n] [k] [max_volume]

The LP uses x_v for selected mass and y_v for boundary mass:
    sum(x_v) = volume
    x_u + y_u <= 1
    x_v - y_u <= 0 for every edge (u, v)
    0 <= x_v, y_v <= 1
"""

from __future__ import annotations

import itertools
import sys


def arrangement_graph(
    n: int, k: int
) -> tuple[list[tuple[int, ...]], list[tuple[int, int]], list[list[int]]]:
    """Construct vertices, undirected edges, and coordinate fibers."""
    vertices = list(itertools.permutations(range(n), k))
    index = {vertex: vertex_id for vertex_id, vertex in enumerate(vertices)}
    edges: set[tuple[int, int]] = set()
    for vertex_id, vertex in enumerate(vertices):
        for position in range(k):
            for symbol in range(n):
                if symbol in vertex:
                    continue
                neighbor = list(vertex)
                neighbor[position] = symbol
                neighbor_id = index[tuple(neighbor)]
                edges.add(tuple(sorted((vertex_id, neighbor_id))))
    roots: list[list[int]] = []
    for position in range(k):
        by_root: dict[tuple[int, ...], list[int]] = {}
        for vertex_id, vertex in enumerate(vertices):
            root = vertex[:position] + vertex[position + 1 :]
            by_root.setdefault(root, []).append(vertex_id)
        roots.extend(by_root.values())
    return vertices, sorted(edges), roots


def solve_profile(n: int, k: int, max_volume: int) -> list[float]:
    """Solve the fractional vertex-boundary LP for each requested volume."""
    try:
        # Keep SciPy optional so the rest of the repository does not require it.
        # pylint: disable=import-outside-toplevel
        import numpy as np
        from scipy.optimize import linprog
        from scipy.sparse import lil_matrix
    except ImportError as exc:
        raise SystemExit("requires numpy and scipy") from exc

    vertices, edges, roots = arrangement_graph(n, k)
    vertex_count = len(vertices)
    root_count = len(roots)
    z_offset = 2 * vertex_count
    variable_count = z_offset + root_count
    rows = vertex_count + 4 * len(edges) + 2 * sum(len(root) for root in roots)
    matrix = lil_matrix((rows, variable_count), dtype=float)
    upper = np.zeros(rows)
    row = 0

    # A selected vertex cannot simultaneously be external boundary.
    for vertex_id in range(vertex_count):
        matrix[row, vertex_id] = 1.0
        matrix[row, vertex_count + vertex_id] = 1.0
        upper[row] = 1.0
        row += 1

    # Every selected neighbor forces boundary mass at this vertex.
    for selected, boundary in edges:
        matrix[row, selected] = 1.0
        matrix[row, vertex_count + boundary] = -1.0
        row += 1
        matrix[row, boundary] = 1.0
        matrix[row, vertex_count + selected] = -1.0
        row += 1

    # Fiber activation and the remaining-space boundary cut.  For integral
    # x, z is exactly the indicator that a root contains a selected vertex.
    m = n - k
    for root_id, root in enumerate(roots):
        z = z_offset + root_id
        for vertex_id in root:
            matrix[row, vertex_id] = 1.0
            matrix[row, z] = -1.0
            row += 1

        # z <= sum(x on root), preventing unused roots from activating.
        matrix[row, z] = 1.0
        for vertex_id in root:
            matrix[row, vertex_id] = -1.0
        upper[row] = 0.0
        row += 1

        # sum_R(x+y) >= (m+1)z.
        matrix[row, z] = m + 1
        for vertex_id in root:
            matrix[row, vertex_id] = -1.0
            matrix[row, vertex_count + vertex_id] = -1.0
        upper[row] = 0.0
        row += 1

    objective = np.zeros(variable_count)
    objective[vertex_count:z_offset] = 1.0
    equality = np.zeros((1, variable_count))
    equality[0, :vertex_count] = 1.0
    bounds = [(0.0, 1.0)] * variable_count
    profile = []
    for volume in range(max_volume + 1):
        equality_value = np.array([float(volume)])
        result = linprog(
            objective,
            A_ub=matrix.tocsr(),
            b_ub=upper,
            A_eq=equality,
            b_eq=equality_value,
            bounds=bounds,
            method="highs",
        )
        if not result.success:
            raise RuntimeError(f"volume {volume}: {result.message}")
        profile.append(float(result.fun))
    return profile


def main() -> int:
    """Parse command-line arguments and print the LP profile."""
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    k = int(sys.argv[2]) if len(sys.argv) > 2 else 3
    max_volume = int(sys.argv[3]) if len(sys.argv) > 3 else 6
    profile = solve_profile(n, k, max_volume)

    print(f"A({n},{k}) fractional vertex-boundary LP")
    print("profile=" + ",".join(f"{value:.9f}" for value in profile))
    minimum_delta = float("inf")
    witness = None
    for a in range(max_volume + 1):
        for b in range(max_volume + 1 - a):
            delta = profile[a] + profile[b] - profile[a + b]
            if delta < minimum_delta:
                minimum_delta = delta
                witness = (a, b)
    print(f"minimum-delta={minimum_delta:.9f} witness={witness}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
