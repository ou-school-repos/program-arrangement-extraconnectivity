#!/usr/bin/env python3
"""Sweep full-Star spines to map where the split identity first fails.

Reuses the exact closed forms from ``full_star_spine.py``:
``D(S_j) = j*m`` and ``X(S_j) = m*(m - 1)*binom(j, 2)`` for the full-Star
family in ``A(k+m,k)`` with ``k >= j``.

For each edge ``S_(j-1) -> S_j`` this reports:

- the local split gap ``surplus - overhead`` (per-edge accounting, as in
  ``full_star_spine.py``), and
- the cumulative slack ``P(S_j) - Phi(S_j)`` where ``Phi`` is the running
  sum of ``overhead`` from ``S_0``.

Both quantities can be negative independently; the script records the
smallest ``j`` at which each first goes negative, per ``m``. This is an
empirical sweep over the full-Star family only -- it does not touch partial
Stars or non-Star configurations, and a clean range here proves nothing
about ``m`` or ``j`` outside the values checked.
"""

import argparse


def sbl(size):
    """Return sum(bit_length(i) for 1 <= i < size)."""
    return sum(value.bit_length() for value in range(1, size))


def e_seq(size):
    """Return cumulative binary popcount through size - 1."""
    return sum(value.bit_count() for value in range(size))


def potential(size, overhang):
    """Return P(size) = C(size) + overhang * E(size)."""
    if size == 0:
        return 0
    return size - 1 + sbl(size) + (overhang - 1) * e_seq(size)


def sweep(m, j_max):
    """Print per-edge gaps and cumulative slack for j = 1..j_max at this m."""
    print(f"\n=== sweep: m={m}, j=1..{j_max} ===")
    print(" j     R      X      D |   dX   dD | local_gap  cum_slack")
    print("-" * 62)
    first_local_negative = None
    first_cum_negative = None
    phi = 0
    for j in range(1, j_max + 1):
        size = 1 + j * m
        child_size = 1 + (j - 1) * m
        x_j = m * (m - 1) * j * (j - 1) // 2
        x_child = m * (m - 1) * (j - 1) * (j - 2) // 2 if j >= 2 else 0
        d_j = j * m
        d_child = (j - 1) * m
        delta_x = x_j - x_child
        delta_d = d_j - d_child
        overhead = delta_x + (m + 1) * delta_d
        surplus = potential(size, m) - potential(child_size, m)
        local_gap = surplus - overhead
        phi += overhead
        cum_slack = potential(size, m) - phi
        if local_gap < 0 and first_local_negative is None:
            first_local_negative = j
        if cum_slack < 0 and first_cum_negative is None:
            first_cum_negative = j
        print(
            f"{j:2d} {size:5d} {x_j:6d} {d_j:5d} |"
            f" {delta_x:4d} {delta_d:3d} | {local_gap:9d}  {cum_slack:9d}"
        )
    print(
        f"first negative local_gap: j={first_local_negative}; "
        f"first negative cum_slack: j={first_cum_negative}"
    )
    return first_local_negative, first_cum_negative


def main():
    """Sweep the requested m values across the requested j range."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--m", type=int, nargs="+", required=True)
    parser.add_argument(
        "--j-range",
        type=int,
        nargs=2,
        metavar=("J_MIN", "J_MAX"),
        required=True,
    )
    args = parser.parse_args()
    j_min, j_max = args.j_range
    if j_min < 1 or j_max < j_min:
        parser.error("require 1 <= J_MIN <= J_MAX")
    summary = {}
    for m in args.m:
        if m < 1:
            parser.error("require m >= 1")
        first_local, first_cum = sweep(m, j_max)
        summary[m] = (first_local, first_cum)
    print("\n=== summary ===")
    for m, (first_local, first_cum) in summary.items():
        print(
            f"m={m}: first negative local_gap j={first_local}, cum_slack j={first_cum}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
