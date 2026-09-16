#!/usr/bin/env python3
"""Finite checks for ``sbl`` recurrences and star-fiber surplus accounting.

For ``sbl(R) = sum(bit_length(i) for 1 <= i < R)``, verify:

* ``sbl(2m)   = 2*sbl(m) + 2m - 1``;
* ``sbl(2m+1) = sbl(m) + sbl(m+1) + 2m``.

For radius-one stars in selected arrangement graphs, decompose the exact
``P``-surplus at a coordinate split:

    P(R) - sum P(c_i)
      = (t - 1) + (sbl(R) - sum sbl(c_i))
        + (m - 1) * (E(R) - sum E(c_i)).

This is a finite arithmetic/geometry diagnostic.  It does not prove a
universal induction inequality.
"""

from lib import c_constant, e_seq, neighbors, sbl


def potential(size, overhang):
    """``P(R) = C(R) + m E(R)``."""
    return c_constant(size) + overhang * e_seq(size)


def star_graph(alphabet_size, dimension, size):
    """A radius-one star, or ``None`` when ``size`` exceeds its capacity."""
    center = tuple(range(dimension))
    available = 1 + dimension * (alphabet_size - dimension)
    if size > available:
        return None
    return [center, *sorted(neighbors(center, alphabet_size))[: size - 1]]


def cross_collisions(vertices, alphabet_size, dimension):
    """Root-incidence excess over the ordinary external vertex boundary."""
    vertex_set = set(vertices)
    coordinate_boundary_size = 0
    external = set()
    for position in range(dimension):
        directional = set()
        for vertex in vertices:
            used = set(vertex)
            for symbol in range(alphabet_size):
                if symbol not in used:
                    candidate = vertex[:position] + (symbol,) + vertex[position + 1 :]
                    if candidate not in vertex_set:
                        directional.add(candidate)
        coordinate_boundary_size += len(directional)
        external.update(directional)
    return coordinate_boundary_size - len(external)


def defect(vertices, dimension):
    """``R*k -`` the total number of occupied coordinate roots."""
    root_count = 0
    for position in range(dimension):
        root_count += len(
            {vertex[:position] + vertex[position + 1 :] for vertex in vertices}
        )
    return len(vertices) * dimension - root_count


def phi(vertices, alphabet_size, dimension):
    """Return ``X + (m+1)D`` for a vertex set in ``A(n,k)``."""
    overhang = alphabet_size - dimension
    return cross_collisions(vertices, alphabet_size, dimension) + (
        overhang + 1
    ) * defect(vertices, dimension)


def verify_recurrences(limit):
    """Return all failed even/odd ``sbl`` recurrence checks through ``limit``."""
    failures = []
    for half in range(1, limit + 1):
        if sbl(2 * half) != 2 * sbl(half) + 2 * half - 1:
            failures.append(("even", half))
        if sbl(2 * half + 1) != sbl(half) + sbl(half + 1) + 2 * half:
            failures.append(("odd", half))
    return failures


def split_gap(vertices, alphabet_size, dimension, position):
    """Return fiber sizes and raw ``P``-surplus minus ``Phi`` overhead."""
    fibers = {}
    for vertex in vertices:
        fibers.setdefault(vertex[position], []).append(vertex)
    if len(fibers) < 2:
        return (), None
    sizes = tuple(sorted(len(fiber) for fiber in fibers.values()))
    size = len(vertices)
    overhang = alphabet_size - dimension
    overhead = phi(vertices, alphabet_size, dimension) - sum(
        phi(fiber, alphabet_size, dimension) for fiber in fibers.values()
    )
    surplus = potential(size, overhang) - sum(
        potential(fiber_size, overhang) for fiber_size in sizes
    )
    return sizes, surplus - overhead


def main():
    """Run the finite recurrence and capacity-valid star diagnostics."""
    failures = verify_recurrences(1000)
    print("=== exact sbl recurrences ===")
    print(f"tests: 2000; failures: {len(failures)}")
    if failures:
        print(f"first failure: {failures[0]}")
        return 1

    print("\n=== star-fiber P-surplus decomposition ===")
    print(" n  k  R  t | overhead | sbl-surp (m-1)E-surp  t-1 | P-surp | gap")
    print("-" * 78)
    for alphabet_size, dimension in [(5, 3), (6, 3), (7, 4), (8, 4), (8, 5)]:
        overhang = alphabet_size - dimension
        capacity = 1 + dimension * overhang
        for requested_size in range(4, min(19, capacity) + 1):
            vertices = star_graph(alphabet_size, dimension, requested_size)
            assert vertices is not None
            size = len(vertices)

            fibers = {}
            for vertex in vertices:
                fibers.setdefault(vertex[0], []).append(vertex)
            if len(fibers) < 2:
                continue

            sizes = [len(fiber) for fiber in fibers.values()]
            fiber_phi = sum(
                phi(fiber, alphabet_size, dimension) for fiber in fibers.values()
            )
            overhead = phi(vertices, alphabet_size, dimension) - fiber_phi
            sbl_surplus = sbl(size) - sum(sbl(fiber_size) for fiber_size in sizes)
            e_surplus = e_seq(size) - sum(e_seq(fiber_size) for fiber_size in sizes)
            linear_surplus = len(sizes) - 1
            p_surplus = potential(size, overhang) - sum(
                potential(fiber_size, overhang) for fiber_size in sizes
            )
            assert p_surplus == (
                linear_surplus + sbl_surplus + (overhang - 1) * e_surplus
            )
            gap = p_surplus - overhead
            print(
                f"{alphabet_size:2d} {dimension:2d} {size:2d} {len(sizes):2d} |"
                f" {overhead:8d} | {sbl_surplus:8d}"
                f" {(overhang - 1) * e_surplus:11d} {linear_surplus:4d} |"
                f" {p_surplus:6d} | {gap:3d}"
            )

    print("\n=== all-coordinate check for negative Star splits ===")
    for alphabet_size, dimension in [(7, 4), (8, 4), (8, 5)]:
        overhang = alphabet_size - dimension
        capacity = 1 + dimension * overhang
        for size in range(4, min(19, capacity) + 1):
            vertices = star_graph(alphabet_size, dimension, size)
            assert vertices is not None
            coordinate_data = [
                split_gap(vertices, alphabet_size, dimension, position)
                for position in range(dimension)
            ]
            gaps = [gap for _sizes, gap in coordinate_data if gap is not None]
            if min(gaps) >= 0:
                continue
            valid_positions = [
                position
                for position, (_sizes, gap) in enumerate(coordinate_data)
                if gap is not None
            ]
            best_position = max(
                valid_positions,
                key=lambda position, data=coordinate_data: data[position][1],
            )
            print(
                f"A({alphabet_size},{dimension}) R={size}:"
                f" best p={best_position}, gap={coordinate_data[best_position][1]}"
            )
            for position, (sizes, gap) in enumerate(coordinate_data):
                print(f"  p={position}: sizes={sizes}, gap={gap}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
