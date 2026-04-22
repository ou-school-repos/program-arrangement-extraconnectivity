# Hypercube Edge Isoperimetry in Arrangement Graphs

## The Pattern Break at R=8

For R=2..7, the minimum (R-1)-extraconnectivity follows a linear progression
in its `(n-k)` coefficient. The coefficient `Rk - E` decreases by incrementing
E by 1 for each additional R. But at R=8, the coefficient jumps:

| R   | nk1 (= E, internal edges) | Expected (tree) | Actual |
| --- | ------------------------- | --------------- | ------ |
| 2   | 1                         | 1               | 1      |
| 3   | 2                         | 2               | 2      |
| 4   | 4                         | 3               | **4**  |
| 5   | 5                         | 4               | 5      |
| 6   | 7                         | 5               | 7      |
| 7   | 9                         | 6               | 9      |
| 8   | **12**                    | 7               | **12** |
| 9   | **13**                    | 8               | **13** |

## Why: Harper's Edge Isoperimetric Theorem

The `nk1` value in our search output equals E(R) — the maximum number of
internal edges achievable among R vertices in the Arrangement Graph A(n,k).

A(n,k) is a subgraph of the Hamming graph K_n^k. By Harper's Edge
Isoperimetric Theorem, the subgraph that maximizes internal edges (and thus
minimizes the external boundary) for R vertices is the **Hamming Ball** —
vertices ordered in binary lexicographic order.

For R = 2^d, this forms a perfect d-dimensional hypercube:

- 1-cube (R=2): 1 edge
- 2-cube (R=4): 4 edges
- 3-cube (R=8): **12 edges** (not 7 as a tree would give)
- 4-cube (R=16): 32 edges

## The E(R) sequence = OEIS A000788

The maximum edge count E(R) is exactly the **cumulative binary weight** —
the total number of 1-bits in the binary representations of 0, 1, ..., R-1:

```
E(R) = sum_{i=0}^{R-1} popcount(i)
```

This is [OEIS A000788](https://oeis.org/A000788):

```
R:    1  2  3  4  5  6  7  8  9  10  11  12  ...
E(R): 0  1  2  4  5  7  9  12 13  15  18  21 ...
```

This lets us **predict** the minimum-cut coefficient without running the search:

- R=10: E(10) = 15, so the coefficient is `(10k - 15)(n-k) - C`
- R=11: E(11) = 18, so the coefficient is `(11k - 18)(n-k) - C`
- R=16: E(16) = 32, so the coefficient is `(16k - 32)(n-k) - C`

(The constant C must still be computed by the search.)

## Closed form for R = 2^d

When R is a power of 2, the edge count has a clean closed form:

```
E(2^d) = d · 2^{d-1}
```

This follows from the recursive structure of hypercubes: a (d+1)-cube consists
of two d-cubes (2 × E(d) edges) plus 2^d new edges connecting them:

```
E(0) = 0
E(d+1) = 2 · E(d) + 2^d
```

Solving: `E(d) · 2 = d · 2^d`, hence `E(d) = d · 2^{d-1}`.

For d=3 (R=8): E(3) = 3 · 4 = 12. ✓

## Implication for the paper

The Cheng et al. (2022) theorems for R=5..7 assume the optimal vertex cuts
grow like "stars" or "trees" with R-1 internal edges. At R=8, the vertices
lock into a 3D hypercube with 12 edges — a **60% increase** over the tree
bound. This causes the external boundary coefficient to drop from the
predicted (8k-7) to the actual **(8k-12)**, which our brute-force
verification confirms.
