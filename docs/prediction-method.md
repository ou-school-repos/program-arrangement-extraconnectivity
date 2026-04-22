# Prediction Method: Hamming Ball Construction

## Overview

For large R, the exhaustive search becomes impractical (~8 min for R=9, ~6h estimated
for R=10). The **predictor** (`predict.cpp`) bypasses the search entirely by constructing
the known-optimal vertex set directly.

## Key Insight: A000788 and the Hamming Ball

The minimum (R-1)-extraconnectivity formula has the form:

```
(Rk − E(R)) · (n−k) − C(R)
```

where:

- **E(R) = A000788(R)** = cumulative popcount = Σ popcount(0..R-1)
- **C(R)** = a constant depending on the vertex set structure

The coefficient `Rk − E(R)` is determined by **OEIS A000788**, the cumulative binary
weight sequence. This sequence counts internal "edges" (shared position-stems) in the
optimal R-vertex subgraph.

## Why the Hamming Ball?

The optimal R-vertex subgraph (maximizing internal sharing, thus minimizing external
boundary) is the **Hamming ball** — the first R vertices in binary-lexicographic order
within the hypercube structure of the arrangement graph.

Construction for R vertices:

1. Start with the identity permutation v₀ = (0, 1, 2, ..., k-1)
2. Determine d = ⌈log₂ R⌉ dimensions
3. For each i from 1 to R-1, vertex vᵢ is v₀ with positions corresponding
   to set bits of i changed to fresh symbols

Example for R=8 (perfect 3-cube, d=3):

```
v₀ = ABCDEFGH  (binary 000)
v₁ = IBCDEFGH  (binary 001 — position 0 flipped)
v₂ = AJCDEFGH  (binary 010 — position 1 flipped)
v₃ = IJCDEFGH  (binary 011 — positions 0,1 flipped)
v₄ = ABKDEFGH  (binary 100 — position 2 flipped)
v₅ = IBKDEFGH  (binary 101 — positions 0,2 flipped)
v₆ = AJKDEFGH  (binary 110 — positions 1,2 flipped)
v₇ = IJKDEFGH  (binary 111 — positions 0,1,2 flipped)
```

## Validation

The predictor is validated against the exhaustive search for R=2..9:

| R  | Search nk1 | Predicted nk1 | A000788(R) | Match |
|----|-----------|---------------|------------|-------|
| 2  | 1         | 1             | 1          | ✓     |
| 3  | 2         | 2             | 2          | ✓     |
| 4  | 4         | 4             | 4          | ✓     |
| 5  | 5         | 5             | 5          | ✓     |
| 6  | 7         | 7             | 7          | ✓     |
| 7  | 9         | 9             | 9          | ✓     |
| 8  | 12        | 12            | 12         | ✓     |
| 9  | 13        | 13            | 13         | ✓     |
| 10 | —         | **15**        | 15         | ✓     |

Both the formula coefficients AND the constants match exactly for all
searched values (R=2..9).

## What Remains Empirical

The Hamming ball optimality is confirmed by exhaustive search for R=2..9
but not formally proven for arrangement graphs. Harper's edge isoperimetric
inequality applies to Hamming graphs, and arrangement graphs are subgraphs
of Hamming graphs, but the full proof that the Hamming ball remains optimal
in this subgraph setting is an open formalization problem.

## Lean Proofs

The file `proofs/HypercubeEdges.lean` provides machine-verified proofs for:

- **E(d) = d · 2^(d-1)**: closed-form edge count for d-dimensional hypercubes
- **A000788 values**: computed and verified for R ≤ 20
- **A000788_fast**: efficient O(log n) halving recurrence, proven equal to naive definition
- **A000788(2^d) = E(d)**: connecting cumulative popcount to hypercube edges at powers of 2

## Efficient Computation of A000788

The halving recurrence computes A000788(n) in O(log n) recursive calls:

```
A000788(0) = 0
A000788(2m) = 2 · A000788(m) + m
A000788(2m+1) = 2 · A000788(m) + m + popcount(m)
```

This works by splitting {0..2m-1} into even and odd subsets: popcount(2k) = popcount(k)
and popcount(2k+1) = popcount(k) + 1, so the even half contributes A000788(m) and the
odd half contributes A000788(m) + m.
