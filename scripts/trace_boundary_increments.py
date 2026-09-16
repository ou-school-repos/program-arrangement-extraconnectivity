#!/usr/bin/env python3
"""Trace exact vertex-boundary increments for two A(n,k) configurations."""

from compare_star_hamming import external_boundary, full_star, hamming_ball


def trace(vertices, n, k):
    """Return (prefix boundary size, increment) pairs for a canonical order."""
    ordered = tuple(sorted(vertices))
    previous = set()
    rows = []
    for step, vertex in enumerate(ordered, start=1):
        current = set(ordered[:step])
        boundary = external_boundary(current, n, k)
        size = len(boundary)
        rows.append((step, vertex, size, size - len(previous)))
        previous = boundary
    return rows


def main():
    """Print exact prefix boundary sizes and increments."""
    n, k, size = 10, 5, 26
    candidates = {
        "binary Hamming ball": hamming_ball(k, size),
        "full Star": full_star(n, k),
    }
    for name, vertices in candidates.items():
        print(f"\n{name}")
        print("step vertex boundary increment")
        for step, vertex, boundary, increment in trace(vertices, n, k):
            print(
                f"{step:>4} {''.join(map(str, vertex)):>6} "
                f"{boundary:>8} {increment:>9}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
