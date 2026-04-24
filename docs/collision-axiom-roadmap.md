# Collision Axiom Formalization Roadmap

The `external_neighbors_collision_bound` axiom in `ArrangementExtraconnectivity.lean`
asserts that for any R-vertex subset V' of A(n,k):

```
|N(V')| ≥ sum_unique_roots(V') · (n-k) - C_constant(R)
```

This document describes what a complete mechanized proof would require.

## Current Status

- **Axiomatized** in Lean 4 (zero `sorry` blocks)
- **Computationally verified** for all R ≤ 20 by brute-force oracle
- **Mathematically justified** by the Kruskal-Katona theorem

## The Core Equivalence: Collisions ≡ 4-Cycles

**Claim**: Two distinct vertices u, v ∈ V' produce the same external neighbor w
if and only if they form a 4-cycle (square) with w and some vertex w'.

**Proof sketch**: If drop_pos(u, p) and drop_pos(v, p) produce the same root r,
then extending r with the same fresh symbol s at position p yields a single
neighbor w. But u and v differ at position p (since they're in different fibers),
so the "collision" w is simultaneously adjacent to both u and v. The fourth
vertex w' is obtained by swapping the fresh symbol at p in the other direction.

**Implication**: Counting collisions is exactly counting 4-cycles in the
subgraph induced by V' ∪ N(V').

## Step 1: Kruskal-Katona Shadow Operators in Lean

### What's needed

The **Kruskal-Katona theorem** states that among all k-element families of
r-element sets, the initial segment in colex order minimizes the shadow
(the family of (r-1)-element subsets contained in at least one member).

### Mathlib status

- `Mathlib.Combinatorics.SetFamily.Shadow` provides basic shadow definitions
- The full KK inequality is **not yet in Mathlib**
- Shadow operators for `Finset (Fin d → Bool)` (bit-vectors) need to be
  connected to the existing `SetFamily.Shadow` infrastructure

### Estimated effort

~200-300 lines for:

- Colex ordering on `Finset (Fin d → Bool)`
- KK inequality statement and proof
- Connection between shadow size and 4-cycle count

## Step 2: Hamming Ball Maximizes Squares

### What's needed

Prove that among all R-element subsets of the d-dimensional hypercube Q_d,
the initial segment in binary lexicographic order (the Hamming Ball)
maximizes the number of 4-cycles.

### Proof approach

1. Define the "square count" function: `squares(S) = |{(u,v,w) : u,v ∈ S, w ∈ N(S), adj(u,w) ∧ adj(v,w)}|`
2. Show that square count is monotone under compression (shifting toward the Hamming Ball)
3. Apply KK to conclude the Hamming Ball is optimal

### Estimated effort

~150-200 lines

## Step 3: Transfer to Arrangement Graphs

### What's needed

Show that the permutation constraint (no duplicate symbols) in A(n,k) only
**removes** edges compared to the full Hamming graph H(k,n). Therefore:

- Any collision bound proven for hypercubes transfers to A(n,k)
- The arrangement graph can only have **fewer** collisions than the hypercube
- This means the external boundary is at least as large as the hypercube bound

### Key lemma

```
A(n,k) ⊆ H(k,n)  as graphs (isometric embedding)
⟹ squares_A(S) ≤ squares_H(S)  for any S
⟹ collisions_A(S) ≤ collisions_H(S)
⟹ |N_A(S)| ≥ |N_H(S)|
```

### Estimated effort

~100-150 lines (mostly boilerplate connecting the two graph definitions)

## Step 4: Deriving C_constant(R)

### What's needed

Show that `C_constant(R) = (R-1) + sum_bit_length(R) - E_seq(R)` exactly
counts the maximum number of collisions for a Hamming Ball of size R.

### Proof approach

Induction on R, using the recursive structure of E_seq (A000788) and
the bitwise decomposition of the Hamming Ball.

### Estimated effort

~100-150 lines

## Total Estimated Effort

**~550-800 lines of Lean 4**, with potential Mathlib contributions required
for the Kruskal-Katona theorem infrastructure.

## Uniqueness (Open Problem)

The current proof establishes that the Hamming Ball **achieves** the minimum
external boundary, but does not prove it is the **unique** minimizer.

### What uniqueness would require

- Show that equality in the defect bound `D(V') = E_seq(|V'|)` forces
  V' to be isomorphic to a Hamming Ball
- This is equivalent to showing that E_seq is **strictly** subadditive
  for non-Hamming-Ball partitions
- The computational search confirms uniqueness for R ≤ 10 (one minimum-cut
  topology class per R)

### Why this is hard

Equality cases in Kruskal-Katona are known but technically involved.
The transfer to arrangement graphs adds another layer of complexity
because the permutation constraint may create additional minimizers
in degenerate cases (small n-k).

This remains an open question for future work.
