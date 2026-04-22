#!/usr/bin/env python3
"""
Independent verification of R=2..9 minimum-cut vertex sets.

These vertex sets were discovered by exhaustive search (arrangementoptimized.cpp).
This script verifies |N(V')| by brute-force neighbor counting — no external
dependencies, no trust in the search engine required.

Usage: python3 docs/verify-counterexample.py
"""


def count_neighbors(V: list[str]) -> int:
    """Count |N(V')| in the arrangement graph A(n,k) where n=2R, k=R."""
    R = len(V[0])
    n = 2 * R
    symbols = [chr(ord("A") + i) for i in range(n)]
    V_set = set(V)
    nbrs: set[str] = set()

    for v in V:
        for pos in range(R):
            for c in symbols:
                if c not in v:
                    rest = pos + 1
                    w = v[:pos] + c + v[rest:]
                    if w not in V_set:
                        nbrs.add(w)
    return len(nbrs)


def verify(R: int, vertices: list[str], expected_nk1: int, expected_const: int) -> None:
    """Verify a vertex set and compare A000788 vs Cheng predictions."""
    assert len(vertices) == R, f"Expected {R} vertices, got {len(vertices)}"
    assert all(len(v) == R for v in vertices), "All vertices must have length R"

    for v in vertices:
        assert len(set(v)) == R, f"Repeated symbol in vertex {v}"

    # Check connectivity via BFS
    def adjacent(a: str, b: str) -> bool:
        return sum(1 for x, y in zip(a, b) if x != y) == 1

    visited = {vertices[0]}
    queue = [vertices[0]]
    while queue:
        curr = queue.pop()
        for v in vertices:
            if v not in visited and adjacent(curr, v):
                visited.add(v)
                queue.append(v)
    assert visited == set(vertices), "Subgraph is NOT connected!"

    # Brute-force neighbor count
    nbr_count = count_neighbors(vertices)

    # A000788 prediction
    coeff = R * R - expected_nk1
    predicted = coeff * R - expected_const

    # Cheng's linear extrapolation: E(R) = 2R-5, constant = 2R-1
    cheng_nk1 = 2 * R - 5
    cheng_const = 2 * R - 1
    cheng_coeff = R * R - cheng_nk1
    cheng_predicted = cheng_coeff * R - cheng_const

    a_match = "MATCH" if nbr_count == predicted else "MISMATCH!"
    c_match = "MATCH" if nbr_count == cheng_predicted else "MISMATCH!"

    nk1_cmp = "=" if expected_nk1 == cheng_nk1 else "!="
    const_cmp = "=" if expected_const == cheng_const else "!="

    print(f"R={R}: |N(V')| = {nbr_count}")
    print(f"  nk1 coeff: A000788={expected_nk1}, Cheng={cheng_nk1} ({nk1_cmp})")
    print(f"  constant:  A000788={expected_const}, Cheng={cheng_const} ({const_cmp})")
    print(f"  A000788: ({coeff})({R}) - {expected_const}" f" = {predicted}  {a_match}")
    print(
        f"  Cheng:   ({cheng_coeff})({R}) - {cheng_const}"
        f" = {cheng_predicted}  {c_match}"
    )

    assert (
        nbr_count == predicted
    ), f"A000788 prediction failed: expected {predicted}, got {nbr_count}"


# All minimum-cut vertex sets from exhaustive search (R=2..9)
CASES = [
    (2, ["AB", "CB"], 1, 1),
    (3, ["ABC", "DBC", "AEC"], 2, 3),
    (4, ["ABCD", "EBCD", "AFCD", "EFCD"], 4, 4),
    (5, ["ABCDE", "FBCDE", "AGCDE", "ABHDE", "FGCDE"], 5, 7),
    (6, ["ABCDEF", "GBCDEF", "AHCDEF", "ABIDEF", "GHCDEF", "GBIDEF"], 7, 9),
    (
        7,
        ["ABCDEFG", "HBCDEFG", "AICDEFG", "ABJDEFG", "HICDEFG", "HBJDEFG", "AIJDEFG"],
        9,
        11,
    ),
    (
        8,
        [
            "ABCDEFGH",
            "IBCDEFGH",
            "AJCDEFGH",
            "ABKDEFGH",
            "IJCDEFGH",
            "IBKDEFGH",
            "AJKDEFGH",
            "IJKDEFGH",
        ],
        12,
        12,
    ),
    (
        9,
        [
            "ABCDEFGHI",
            "JBCDEFGHI",
            "AKCDEFGHI",
            "ABLDEFGHI",
            "ABCMEFGHI",
            "JKCDEFGHI",
            "JBLDEFGHI",
            "AKLDEFGHI",
            "JKLDEFGHI",
        ],
        13,
        16,
    ),
]

if __name__ == "__main__":
    for R, vertices, nk1, const in CASES:
        verify(R, vertices, nk1, const)
    print(f"\nAll {len(CASES)} cases verified (R=2..9).")
