#!/usr/bin/env python3
"""Compute exact coordinate-split data for full-Star spines.

Let ``S_j`` be the center of ``A(k+m,k)`` together with every radius-one
leaf in its first ``j`` coordinates.  Thus ``|S_j| = 1 + j*m``.  Splitting
an active coordinate produces ``S_(j-1)`` and ``m`` singleton fibers.

For this family the two geometric quantities have closed forms:

``D(S_j) = j*m`` and ``X(S_j) = m*(m - 1)*binom(j, 2)``.

The script checks both identities and prints the raw induction gap at every
spine edge.  It is an exact calculation for the maximally symmetric Star
family, not a universal proof for arbitrary subsets.
"""

from lib import cross_collisions, defect, e_seq, sbl


def potential(size, overhang):
    """Return P(size) = C(size) + overhang * E(size)."""
    if size == 0:
        return 0
    return size - 1 + sbl(size) + (overhang - 1) * e_seq(size)


def full_star(n, k, active_coordinates):
    """Return S_j with leaves in positions 0 through active_coordinates - 1."""
    center = tuple(range(k))
    vertices = [center]
    for position in range(active_coordinates):
        for symbol in range(k, n):
            vertices.append(center[:position] + (symbol,) + center[position + 1 :])
    return vertices


def print_spine(k, overhang):
    """Print the full-Star spine for A(k + overhang, k)."""
    n = k + overhang
    stars = [full_star(n, k, active) for active in range(k + 1)]
    print(f"\n=== full Star spine: A({n},{k}), m={overhang} ===")
    print(" j   R    X     D | dX  dD | P-surp   gap")
    print("-" * 50)
    for active in range(1, k + 1):
        parent = stars[active]
        child = stars[active - 1]
        parent_x = cross_collisions(parent, n, k)
        child_x = cross_collisions(child, n, k)
        parent_d = defect(parent, k)
        child_d = defect(child, k)
        expected_x = overhang * (overhang - 1) * active * (active - 1) // 2
        assert parent_d == active * overhang
        assert child_d == (active - 1) * overhang
        assert parent_x == expected_x
        delta_x = parent_x - child_x
        delta_d = parent_d - child_d
        surplus = potential(len(parent), overhang) - potential(len(child), overhang)
        overhead = delta_x + (overhang + 1) * delta_d
        assert delta_x == overhang * (overhang - 1) * (active - 1)
        assert delta_d == overhang
        print(
            f"{active:2d} {len(parent):3d} {parent_x:4d} {parent_d:5d} |"
            f" {delta_x:2d} {delta_d:3d} | {surplus:6d} {surplus - overhead:5d}"
        )


def main():
    """Print the k=5 family through m=8."""
    for overhang in range(1, 9):
        print_spine(5, overhang)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
