#!/usr/bin/env python3
"""Track P - Phi one vertex at a time along explicit spines.

Two spines are checked, both for A(k+m,k):

1. Partial-Star: start from the full Star S_(b-1) (b-1 complete branches)
   and add the leaves of branch b one at a time, watching where the
   cumulative slack P(R) - Phi(R) first goes negative inside that branch.

2. Non-Star spread: at the same target size R, distribute leaves as evenly
   as possible across *more* branches (e.g. partial coverage of 5 or 6
   branches instead of 4 full branches) and compare P - Phi against the
   full-Star spine at that size.

Unlike ``sweep_boundary.py``, X and D here are computed exactly by
enumeration (``cross_collisions`` / ``defect`` from ``full_star_spine.py``),
not from the full-Star closed forms, so this also validates non-Star shapes.
This is a bounded, explicit-enumeration check -- not a proof for arbitrary m
or R outside the cases run.
"""

import argparse

from lib import cross_collisions, defect, e_seq, sbl


def potential(size, overhang):
    """Return P(size) = C(size) + overhang * E(size)."""
    if size == 0:
        return 0
    return size - 1 + sbl(size) + (overhang - 1) * e_seq(size)


def trace_spine(label, n, k, m, vertex_sequence):
    """Print P - Phi and local gap along an explicit vertex-addition order."""
    print(f"\n=== {label}: A({n},{k}), m={m} ===")
    print(" R      X      D | local_gap  cum_slack")
    print("-" * 46)
    center = tuple(range(k))
    vertices = [center]
    phi = 0
    prev_p = potential(1, m)
    prev_x = 0
    prev_d = 0
    first_cum_negative = None
    for vertex in vertex_sequence:
        vertices.append(vertex)
        size = len(vertices)
        x = cross_collisions(vertices, n, k)
        d = defect(vertices, k)
        delta_x = x - prev_x
        delta_d = d - prev_d
        overhead = delta_x + (m + 1) * delta_d
        p = potential(size, m)
        surplus = p - prev_p
        local_gap = surplus - overhead
        phi += overhead
        cum_slack = p - phi
        if cum_slack < 0 and first_cum_negative is None:
            first_cum_negative = size
        print(f"{size:3d} {x:6d} {d:5d} | {local_gap:9d}  {cum_slack:9d}")
        prev_p, prev_x, prev_d = p, x, d
    print(f"first negative cum_slack at R={first_cum_negative}")
    return first_cum_negative


def partial_star_sequence(n, k, full_branches, extra_branch):
    """Yield leaves: full_branches complete branches, then leaves of one more."""
    center = tuple(range(k))
    for position in range(full_branches):
        for symbol in range(k, n):
            yield center[:position] + (symbol,) + center[position + 1 :]
    for symbol in range(k, n):
        yield center[:extra_branch] + (symbol,) + center[extra_branch + 1 :]


def spread_sequence(k, branch_counts):
    """Yield leaves distributed across branches per branch_counts (round-robin)."""
    center = tuple(range(k))
    remaining = list(branch_counts)
    symbol_offsets = [0] * len(branch_counts)
    while any(remaining):
        for position, count in enumerate(remaining):
            if count <= 0:
                continue
            symbol = k + symbol_offsets[position]
            symbol_offsets[position] += 1
            remaining[position] -= 1
            yield center[:position] + (symbol,) + center[position + 1 :]


def main():
    """Run the partial-Star and non-Star spread checks for the requested m."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--m", type=int, required=True)
    parser.add_argument(
        "--full-branches",
        type=int,
        default=4,
        help="number of complete branches before the transition leaf-by-leaf",
    )
    args = parser.parse_args()
    m = args.m
    b = args.full_branches
    k = b + 2
    n = k + m

    seq = partial_star_sequence(n, k, b, b)
    trace_spine(
        f"partial-Star: {b} full branches + branch {b+1} leaf-by-leaf", n, k, m, seq
    )

    target_r = 1 + b * m
    print(f"\n--- non-Star spreads at target R={target_r} (full-Star S_{b}) ---")
    leftover = target_r - 1
    spread_branches = min(k, leftover) if leftover > 0 else 1
    base, extra = divmod(leftover, spread_branches)
    counts = [base + (1 if i < extra else 0) for i in range(spread_branches)]
    seq2 = spread_sequence(k, counts)
    trace_spine(
        f"non-Star spread across {spread_branches} branches (counts={counts})",
        n,
        k,
        m,
        seq2,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
