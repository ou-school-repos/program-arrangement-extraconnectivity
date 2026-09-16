#!/usr/bin/env python3
"""Maximize distance-2 shared-boundary targets between two tight fibers.

This is an exact CP-SAT optimization model for one finite A(n,k) cell.  It
does not prove a symbolic statement: an OPTIMAL result certifies the maximum
for precisely the encoded finite instance.

The definition of ``ghost`` agrees with compute_T in
src/check_amortized_slack.cpp: a vertex in both external boundaries for which
no adjacent cross-fiber pair explains the shared target.

Usage: python3 search_ghosts.py n k c_a c_b [--time-limit SECONDS]
"""

import argparse
import itertools

# OR-Tools is an optional dependency used only by this solver script.
from ortools.sat.python import cp_model  # pylint: disable=import-error


def e_seq(size):
    """Return the cumulative popcount below ``size``."""
    return sum(value.bit_count() for value in range(size))


def c_constant(size):
    """The C(R) used throughout the paper and the existing search tools."""
    if size == 0:
        return 0
    return (
        (size - 1) + sum(value.bit_length() for value in range(1, size)) - e_seq(size)
    )


def rhs(n, k, size):
    """Return the conjectured minimum boundary for an ``size``-set."""
    return (size * k - e_seq(size)) * (n - k) - c_constant(size)


def iff_any(model, literals, name):
    """Return b with b iff at least one literal in literals is true."""
    result = model.NewBoolVar(name)
    if literals:
        model.AddBoolOr(literals).OnlyEnforceIf(result)
        model.AddBoolAnd([literal.Not() for literal in literals]).OnlyEnforceIf(
            result.Not()
        )
    else:
        model.Add(result == 0)
    return result


def iff_all(model, literals, name):
    """Return b with b iff every literal in literals is true."""
    result = model.NewBoolVar(name)
    model.AddBoolAnd(literals).OnlyEnforceIf(result)
    model.AddBoolOr([literal.Not() for literal in literals]).OnlyEnforceIf(result.Not())
    return result


def main():
    """Build and solve the finite CP-SAT ghost-search model."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("n", type=int)
    parser.add_argument("k", type=int)
    parser.add_argument("ca", type=int)
    parser.add_argument("cb", type=int)
    parser.add_argument("--time-limit", type=float, default=None)
    args = parser.parse_args()
    n, k, ca, cb = args.n, args.k, args.ca, args.cb

    if not (1 <= k <= n and ca > 0 and cb > 0):
        parser.error("require 1 <= k <= n and positive c_a, c_b")

    vertices = list(itertools.permutations(range(n), k))
    index = {vertex: vertex_id for vertex_id, vertex in enumerate(vertices)}
    adjacency = [[] for _ in vertices]
    for vertex_id, vertex in enumerate(vertices):
        used = set(vertex)
        for position in range(k):
            for symbol in range(n):
                if symbol not in used:
                    neighbor = list(vertex)
                    neighbor[position] = symbol
                    adjacency[vertex_id].append(index[tuple(neighbor)])
    adjacent = [set(neighbors) for neighbors in adjacency]

    target_a, target_b = rhs(n, k, ca), rhs(n, k, cb)
    print(
        f"Building CP-SAT model for A({n},{k}), c_a={ca}, c_b={cb} "
        f"(N={len(vertices)}; tight targets {target_a}, {target_b})..."
    )
    model = cp_model.CpModel()
    in_a = [model.NewBoolVar(f"in_a_{vertex_id}") for vertex_id in range(len(vertices))]
    in_b = [model.NewBoolVar(f"in_b_{vertex_id}") for vertex_id in range(len(vertices))]
    model.Add(sum(in_a) == ca)
    model.Add(sum(in_b) == cb)
    for vertex_id in range(len(vertices)):
        model.AddAtMostOne(in_a[vertex_id], in_b[vertex_id])

    ext_a, ext_b = [], []
    for vertex_id, neighbors in enumerate(adjacency):
        adjacent_a = iff_any(
            model, [in_a[neighbor] for neighbor in neighbors], f"adj_a_{vertex_id}"
        )
        adjacent_b = iff_any(
            model, [in_b[neighbor] for neighbor in neighbors], f"adj_b_{vertex_id}"
        )
        ext_a.append(
            iff_all(model, [adjacent_a, in_a[vertex_id].Not()], f"ext_a_{vertex_id}")
        )
        ext_b.append(
            iff_all(model, [adjacent_b, in_b[vertex_id].Not()], f"ext_b_{vertex_id}")
        )
    model.Add(sum(ext_a) == target_a)
    model.Add(sum(ext_b) == target_b)

    ghosts = []
    for target, neighbors in enumerate(adjacency):
        # Ordered pairs retain the orientation: u belongs to F_a and v to F_b.
        explainers = [
            iff_all(model, [in_a[u], in_b[v]], f"explainer_{target}_{u}_{v}")
            for u in neighbors
            for v in neighbors
            if u != v and v in adjacent[u]
        ]
        distance_one = iff_any(model, explainers, f"distance_one_{target}")
        ghosts.append(
            iff_all(
                model,
                [ext_a[target], ext_b[target], distance_one.Not()],
                f"ghost_{target}",
            )
        )

    model.Maximize(sum(ghosts))
    solver = cp_model.CpSolver()
    solver.parameters.num_search_workers = 8
    if args.time_limit is not None:
        solver.parameters.max_time_in_seconds = args.time_limit
    print("Searching for the maximum ghost count T...")
    status = solver.Solve(model)
    print(f"status: {solver.StatusName(status)}")
    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return

    print(f"T = {int(solver.ObjectiveValue())}")
    for label, variables in (("F_a", in_a), ("F_b", in_b), ("ghosts", ghosts)):
        chosen = [
            "".join(map(str, vertices[i]))
            for i, var in enumerate(variables)
            if solver.Value(var)
        ]
        print(f"{label} = {chosen}")
    if status != cp_model.OPTIMAL:
        print(
            "WARNING: this is an incumbent, not a certified maximum;",
            "increase --time-limit.",
        )


if __name__ == "__main__":
    main()
