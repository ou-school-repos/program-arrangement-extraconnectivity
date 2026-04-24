# Collision Axiom Formalization Roadmap

The `external_neighbors_collision_bound` axiom in `ArrangementExtraconnectivity.lean`
asserts that for any R-vertex subset V' of A(n,k):

```
|N(V')| ≥ sum_unique_roots(V') · (n-k) - C_constant(R)
```

This document describes what a complete mechanized proof would require.

## Current Status

- **Axiomatized** in Lean 4 (zero `sorry` blocks)
- **Formula values verified** for R ≤ 20 by predictor oracle (`predict.cpp`)
- **Exhaustive topology search** confirms uniqueness for R ≤ 10
  (`arrangementoptimized.cpp`)
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
  topology class per R); formula values verified for R ≤ 20
- Formalized as `uniqueness_conjecture` using the full automorphism group
  S_n × S_k (symbol permutation σ + coordinate permutation τ)

### Why this is hard

Equality cases in Kruskal-Katona are known but technically involved.
The transfer to arrangement graphs adds another layer of complexity
because the permutation constraint may create additional minimizers
in degenerate cases (small n-k).

This remains an open question for future work.

## Future Work: Custom Sequence Compressions

### Why Mathlib's Kruskal-Katona Doesn't Apply Directly

Mathlib provides `Finset.kruskal_katona` in
`Mathlib.Combinatorics.SetFamily.KruskalKatona`, along with shadow
operators, colex ordering, and UV-compression. However, these operate
on **unordered subsets** (`Finset (Finset α)`), not injective sequences.

The arrangement graph A(n,k) has vertices that are **ordered, injective
sequences** (`Fin k → Fin n`). The key incompatibilities:

- **Injectivity blindspot**: Mathlib's `uv.compress` has no concept of
  the injectivity constraint and will "compress" into invalid states
  where two positions share the same symbol.
- **Different shadow definitions**: Mathlib's shadow removes an unordered
  element; our `unique_roots` drops a specific coordinate position,
  producing a (k-1)-sequence. This coordinate-aware projection does not
  commute with unordered set compressions.
- **Factorial trap**: Multiplying by k! to account for ordering destroys
  the local topological nuance of asymmetric concentration that makes
  the Hamming Ball extremal.

### The Sequence Compression Operator

To remove the axioms, define a custom compression for A(n,k):

```
compress(V', a, b) where a < b :
  for each v ∈ V':
    if v uses symbol b at some position p AND v doesn't use symbol a:
      let v' = v with b replaced by a at position p
      if v' ∉ V':        -- CRITICAL: set-wise injectivity guard
        map v → v'
      else:
        leave v unchanged  (target space occupied)
    else:
      leave v unchanged
```

**Set-wise injectivity trap**: Without the `v' ∉ V'` guard, two distinct
vertices can collapse to the same target:

- v1 = (b, x) compresses to (a, x)
- v2 = (a, x) stays as (a, x)
- Both map to (a, x), destroying cardinality

The conditional guard (matching Mathlib's `uv.compress` pattern) ensures
the operator is a bijection on V'.

### Proof Obligations (~500-800 lines)

1. **Compression preserves injectivity** (~50 lines): Swapping one symbol
   in an injective sequence produces another injective sequence.

2. **Compression preserves cardinality** (~100 lines): The conditional
   set-wise operator is a bijection on V' (the hard direction: showing
   the guard never creates orphaned vertices).

3. **Compression does not increase boundary** (~200 lines): The core
   extremal lemma. Compressing two vertices toward shared symbols
   increases root collisions, which can only decrease external neighbors.

4. **Colex ordering for sequences** (~50 lines): Define a total order on
   `ArrVertex n k` matching the Hamming Ball construction step-by-step.

5. **Convergence** (~100 lines): Repeated compression terminates at the
   Hamming Ball initial segment (the colex minimum).

### Architectural Recommendation

While the theorems cannot be imported from Mathlib, the **design patterns**
can be copied: study `Mathlib.Combinatorics.SetFamily.Compression.UV` for
the conditional compression architecture, and
`Mathlib.Combinatorics.Colex` for the ordering machinery. Adapting these
patterns to injective sequences is the most efficient path.
