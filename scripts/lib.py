"""Shared helpers for the finite arrangement-graph experiments."""

from __future__ import annotations

import itertools


def e_seq(size: int) -> int:
    """Return the cumulative popcount below ``size``."""
    return sum(value.bit_count() for value in range(size))


def sbl(size: int) -> int:
    """Return the cumulative bit-length sum below ``size``."""
    return sum(value.bit_length() for value in range(1, size))


def c_constant(size: int) -> int:
    """Return the collision constant for a set of ``size`` vertices."""
    return 0 if size == 0 else size - 1 + sbl(size) - e_seq(size)


def neighbors(vertex: tuple[int, ...], alphabet_size: int) -> set[tuple[int, ...]]:
    """Return all one-coordinate substitutions of an injective tuple."""
    used = set(vertex)
    return {
        vertex[:position] + (symbol,) + vertex[position + 1 :]
        for position in range(len(vertex))
        for symbol in range(alphabet_size)
        if symbol not in used
    }


def coordinate_neighbors(
    vertex: tuple[int, ...], position: int, alphabet_size: int
) -> set[tuple[int, ...]]:
    """Return substitutions of ``vertex`` at one coordinate."""
    used = set(vertex)
    return {
        vertex[:position] + (symbol,) + vertex[position + 1 :]
        for symbol in range(alphabet_size)
        if symbol not in used
    }


def cross_collisions(
    vertices: tuple[tuple[int, ...], ...] | list[tuple[int, ...]],
    alphabet_size: int,
    dimension: int,
) -> int:
    """Return coordinate-boundary overlap excess for ``vertices``."""
    members = set(vertices)
    directional_total = 0
    external: set[tuple[int, ...]] = set()
    for position in range(dimension):
        directional: set[tuple[int, ...]] = set()
        for vertex in vertices:
            directional.update(coordinate_neighbors(vertex, position, alphabet_size))
        directional -= members
        directional_total += len(directional)
        external |= directional
    return directional_total - len(external)


def defect(
    vertices: tuple[tuple[int, ...], ...] | list[tuple[int, ...]], dimension: int
) -> int:
    """Return the coordinate-fiber defect of ``vertices``."""
    # Roots are counted separately for each coordinate: the same tuple can be
    # a root in different positions, and those occurrences are distinct.
    roots_by_position = sum(
        len({vertex[:position] + vertex[position + 1 :] for vertex in vertices})
        for position in range(dimension)
    )
    return len(vertices) * dimension - roots_by_position


def star_graph(alphabet_size: int, dimension: int, size: int):
    """Return a radius-one star truncated to ``size`` vertices."""
    center = tuple(range(dimension))
    return [center] + list(neighbors(center, alphabet_size))[: size - 1]


def build_fibers(vertices, dimension: int):
    """Build coordinate-root fiber lookup tables."""
    fibers = [{} for _ in range(dimension)]
    for vertex in vertices:
        for position in range(dimension):
            root = vertex[:position] + vertex[position + 1 :]
            fibers[position].setdefault(root, set()).add(vertex)
    return fibers


def boundary_metrics(subset, fibers):
    """Return external boundary, coordinate defect, and collision excess."""
    members = set(subset)
    coordinate_boundaries = []
    unique_roots = 0
    for position, position_fibers in enumerate(fibers):
        roots = {vertex[:position] + vertex[position + 1 :] for vertex in subset}
        unique_roots += len(roots)
        coordinate_boundaries.append(
            set().union(*(position_fibers[root] for root in roots)) - members
        )
    total = sum(map(len, coordinate_boundaries))
    boundary = len(set().union(*coordinate_boundaries))
    defect_value = len(subset) * len(fibers) - unique_roots
    return boundary, defect_value, total - boundary


def is_connected(subset, adjacency) -> bool:
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


def arrangement_vertices(alphabet_size: int, dimension: int):
    """Return all injective ``dimension``-tuples over the alphabet."""
    return list(itertools.permutations(range(alphabet_size), dimension))
