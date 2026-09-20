#!/usr/bin/env python3
"""Finite LP diagnostic for Strategy 2's additive fiber-size ansatz.

For an A(n,k) set S, let N_s(S) count coordinate roots containing s members.
This checks whether convex weights w_1,...,w_(m+1), m=n-k, can give the
finite-corpus sandwich

  X(S) + (m+1)D(S) <= sum_s N_s(S) w_s <= C(R)+mE(R),  R=|S|,

while agreeing with the repository's embedded Hamming cubes whenever they
fit: Psi(H_R)=C(R)+mE(R).  Thus feasibility is only evidence for this
particular additive ansatz; infeasibility refutes it on the sampled corpus.

Run: python3 scripts/lyapunov_profile_check.py [--ambient=N,K]
Requires SciPy, already present in the development environment.
"""

import argparse
import itertools
from collections import defaultdict

import numpy as np
from coverage_avoidance import Cconst, Eseq, corpus
from scipy.optimize import linprog


def build_fibers(n, k):
    """Enumerate A(n,k) and index its coordinate-root cliques."""
    vertices = list(itertools.permutations(range(n), k))
    fibers = [defaultdict(set) for _ in range(k)]
    for vertex in vertices:
        for position in range(k):
            root = vertex[:position] + vertex[position + 1 :]
            fibers[position][root].add(vertex)
    return fibers


def profile_and_phi(subset, fibers, k, m):
    """Return the nonzero fiber profile and Phi=X+(m+1)D."""
    members = set(subset)
    profile = [0] * (m + 1)
    unique_roots = 0
    coordinate_boundaries = []
    for position, position_fibers in enumerate(fibers):
        occupied = {vertex[:position] + vertex[position + 1 :] for vertex in members}
        unique_roots += len(occupied)
        for root in occupied:
            occupancy = len(members & position_fibers[root])
            assert 1 <= occupancy <= m + 1
            profile[occupancy - 1] += 1
        coordinate_boundaries.append(
            set().union(*(position_fibers[root] for root in occupied)) - members
        )
    boundary = len(set().union(*coordinate_boundaries))
    root_boundary_sum = sum(len(boundary_set) for boundary_set in coordinate_boundaries)
    collisions = root_boundary_sum - boundary
    defect = len(members) * k - unique_roots
    return tuple(profile), collisions + (m + 1) * defect


def embedded_hamming_ball(n, k, size):
    """The `embed_vertex` cube construction used by CrossTop/ArrDefs."""
    dimension = (size - 1).bit_length()
    if dimension > min(k, n - k):
        return None
    return tuple(
        tuple(
            (
                (k + position if (index >> position) & 1 else position)
                if position < dimension
                else position
            )
            for position in range(k)
        )
        for index in range(size)
    )


def solve_group(n, k, records):
    """Build the normalized convex feasibility LP for A(n,k)."""
    m = n - k
    fibers = build_fibers(n, k)
    width = m + 1
    a_ub, b_ub = [], []
    for _tag, subset in records:
        profile, value = profile_and_phi(subset, fibers, k, m)
        target = Cconst(len(subset)) + m * Eseq(len(subset))
        # Phi <= Psi <= target.
        a_ub.extend(([-entry for entry in profile], list(profile)))
        b_ub.extend((-value, target))

    # Discrete convexity with w_0=0:
    # 2 w_1 <= w_2, then 2 w_s <= w_(s-1)+w_(s+1).
    for index in range(width - 1):
        row = [0] * width
        row[index] = 2
        if index:
            row[index - 1] -= 1
        row[index + 1] -= 1
        a_ub.append(row)
        b_ub.append(0)

    a_eq, b_eq, normalized_sizes = [], [], []
    for size in range(1, 1 + 2 ** min(k, m)):
        ball = embedded_hamming_ball(n, k, size)
        assert ball is not None
        profile, _value = profile_and_phi(ball, fibers, k, m)
        a_eq.append(profile)
        b_eq.append(Cconst(size) + m * Eseq(size))
        normalized_sizes.append(size)

    result = linprog(
        c=np.zeros(width),
        A_ub=np.asarray(a_ub, dtype=float),
        b_ub=np.asarray(b_ub, dtype=float),
        A_eq=np.asarray(a_eq, dtype=float),
        b_eq=np.asarray(b_eq, dtype=float),
        bounds=[(None, None)] * width,
        method="highs",
    )
    equality_only = linprog(
        c=np.zeros(width),
        A_eq=np.asarray(a_eq, dtype=float),
        b_eq=np.asarray(b_eq, dtype=float),
        bounds=[(None, None)] * width,
        method="highs",
    )
    return result, equality_only, normalized_sizes


def first_cap_violation(n, k, records, weights):
    """Return one corpus record violating the sandwich for fixed weights."""
    m = n - k
    fibers = build_fibers(n, k)
    for tag, subset in records:
        profile, value = profile_and_phi(subset, fibers, k, m)
        potential = float(np.dot(profile, weights))
        target = Cconst(len(subset)) + m * Eseq(len(subset))
        if value > potential + 1e-8 or potential > target + 1e-8:
            side = "lower" if value > potential + 1e-8 else "upper"
            return tag, len(subset), profile, value, potential, target, side
    return None


def main():
    """Parse arguments and evaluate the LP for specified A(n,k) graphs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ambient", metavar="N,K", help="for example: 5,3")
    args = parser.parse_args()
    requested = tuple(map(int, args.ambient.split(","))) if args.ambient else None
    if requested is not None and len(requested) != 2:
        parser.error("--ambient must have the form N,K")

    groups = defaultdict(list)
    for tag, ambient, subset in corpus():
        key = ambient.n, ambient.k
        if requested is None or key == requested:
            groups[key].append((tag, tuple(subset)))
    if not groups:
        parser.error("no corpus records selected")

    for (n, k), records in sorted(groups.items()):
        result, equality_only, normalized_sizes = solve_group(n, k, records)
        print(f"A({n},{k}): {len(records)} corpus sets")
        print(f"  H_R equality normalization for R={normalized_sizes}")
        if not equality_only.success:
            print("  H_R equalities alone: INFEASIBLE (additive profile mismatch)")
        else:
            weights = ", ".join(f"{weight:.8g}" for weight in equality_only.x)
            print(f"  H_R equalities alone: feasible; one solution = ({weights})")
        if result.success:
            weights = ", ".join(f"{weight:.8g}" for weight in result.x)
            print(f"  FEASIBLE convex weights (w_1,...,w_{n - k + 1}) = ({weights})")
        else:
            print(f"  INFEASIBLE: {result.message}")
            if equality_only.success:
                witness = first_cap_violation(n, k, records, equality_only.x)
                if witness is not None:
                    tag, size, profile, value, potential, target, side = witness
                    print(
                        f"  witness against equality weights ({side} cap): {tag}, "
                        f"R={size}, profile={profile}, Phi={value}, "
                        f"Psi={potential:g}, target={target}"
                    )


if __name__ == "__main__":
    main()
