#!/usr/bin/env python3
"""Compare exact boundaries of a binary Hamming Ball and full Star."""

import argparse


def external_boundary(vertices, n, k):
    """Return the external neighbor set in A(n,k)."""
    members = set(vertices)
    boundary = set()
    for vertex in vertices:
        used = set(vertex)
        for position in range(k):
            for symbol in range(n):
                if symbol in used:
                    continue
                neighbor = vertex[:position] + (symbol,) + vertex[position + 1 :]
                if neighbor not in members:
                    boundary.add(neighbor)
    return boundary


def hamming_ball(k, size):
    """Return the binary Hamming embedding of indices 0 through size - 1."""
    if size > 1 << k:
        raise ValueError("binary Hamming Ball requires size <= 2**k")
    return tuple(
        tuple(
            k + position if (index >> position) & 1 else position
            for position in range(k)
        )
        for index in range(size)
    )


def full_star(n, k):
    """Return the center and every radius-one leaf of the arrangement graph."""
    center = tuple(range(k))
    vertices = [center]
    for position in range(k):
        for symbol in range(k, n):
            vertices.append(center[:position] + (symbol,) + center[position + 1 :])
    return tuple(vertices)


def main():
    """Print exact boundary sizes for the two candidate configurations."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, default=10)
    parser.add_argument("--k", type=int, default=5)
    parser.add_argument("--size", type=int, default=26)
    args = parser.parse_args()
    if not 1 <= args.k <= args.n or args.size < 1:
        parser.error("require n >= k >= 1 and size >= 1")
    candidates = {
        "binary Hamming ball": hamming_ball(args.k, args.size),
        "full Star": full_star(args.n, args.k),
    }
    for name, vertices in candidates.items():
        boundary = external_boundary(vertices, args.n, args.k)
        print(f"{name}: |V|={len(vertices)} |boundary|={len(boundary)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
