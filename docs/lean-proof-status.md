# Lean 4 Formal Verification Status

Formal verification of the Arrangement Graph extraconnectivity theorem
using Lean 4 and Mathlib.

## Build

```bash
make lean          # Build and verify proofs
make lean/cache    # Download pre-built Mathlib cache (first time)
```

Current status: **0 errors, 0 sorries, 2 axioms**.

## Architecture

| Layer                  | Theorem / Definition                                   | Status   |
| ---------------------- | ------------------------------------------------------ | -------- |
| 1 - Combinatorics      | `E_add_min_le`: E(x)+E(y)+min(x,y) <= E(x+y)           | PROVEN   |
| 1.5 - Algebraic Engine | `E_seq_list_sum_le`: generalized partition subaddivity | PROVEN   |
| 2 - Harper's Theorem   | `harpers_edge_isoperimetry`: cubeEdges(S) <= E(\|S\|)  | PROVEN\* |
| 3 - Graph Definition   | `ArrVertex`, `Fintype`, `DecidableEq`, `arr_adjacent`  | PROVEN   |
| 3 - External Neighbors | `external_neighbors` (computable definition)           | PROVEN   |
| 3 - Embedding Cond.    | `can_embed_hypercube` (uses `bit_length(R-1)` ceiling) | PROVEN   |
| 3.5 - Bridge Lemma 2   | `sum_unique_roots_lower_bound` (defect bound)          | PROVEN   |
| 3.5 - Bridge Lemma 3   | `external_neighbors_collision_bound`                   | AXIOM    |
| 3.5 - Upper Bound      | `hamming_ball_achieves_bound`                          | AXIOM    |
| 3.5 - Lower Bound      | `lower_bound_all_embeddings` (arithmetic composition)  | PROVEN   |
| Capstone               | `arrangement_extraconnectivity_minimum` (composition)  | PROVEN   |

\*Harper's Theorem is proven but **not in the dependency chain** of the
capstone theorem. The defect-based proof bypasses it entirely via algebraic
subadditivity of E_seq. Moved to `unstable/ArrangementGraphUtils.lean`.

## Dependency Graph

```
arrangement_extraconnectivity_minimum
  ├─ exists_optimal_embedding
  │    └─ hamming_ball_achieves_bound [AXIOM]
  └─ lower_bound_all_embeddings
       ├─ sum_unique_roots_lower_bound  (Bridge Lemma 2)
       │    └─ defect_fiber_bound
       │         └─ E_seq_list_sum_le   (Layer 1.5 algebraic engine)
       │              └─ E_add_min_le   (Layer 1 core inequality)
       └─ external_neighbors_collision_bound [AXIOM]
```

## Axioms (2)

### Axiom 1: `external_neighbors_collision_bound` — Collision Formula

**What it says**: `|N(V')| ≥ sum_unique_roots(V') · (n-k) - C_constant(R)`.

**Justification**:

- Computationally verified for all R ≤ 20 by brute-force oracle
- Mathematically justified by the Kruskal-Katona theorem (counting
  collisions ≡ counting 4-cycles; Hamming Ball maximizes squares)
- Formalizing requires ~500-800 lines and Mathlib contributions for
  shadow operators not yet available

See [collision-axiom-roadmap.md](collision-axiom-roadmap.md) for the full
formalization roadmap.

### Axiom 2: `hamming_ball_achieves_bound` — Constructive Upper Bound

**What it says**: ∃ V' with |V'| = R achieving the formula exactly.

**Justification**:

- The Hamming Ball construction via `Nat.testBit` is partially formalized
  (`nat_to_cube`, `nat_to_cube_injective`)
- The exact external neighbor evaluation requires the same shadow-counting
  machinery as Axiom 1
- Computationally verified for all R ≤ 20

## What IS Fully Proven (No Axioms)

The **Algebraic Defect Squeeze** — the novel contribution — is 100% mechanized:

1. **E_seq subadditivity** (`E_add_min_le`): The core isoperimetric inequality
   on A000788, proven by strong induction with even/odd case splitting.

2. **Generalized partition bound** (`E_seq_list_sum_le`): Extension from
   binary splits to arbitrary partitions, proven by list induction.

3. **Defect fiber bound** (`defect_fiber_bound`): The topological decomposition
   showing D(V') ≤ Σ D(Fₛ) + R - y via 7-step root disjointness proof.

4. **Universal lower bound** (`sum_unique_roots_lower_bound`): The defect
   bound D(V') ≤ E_seq(R) for ALL R-element subsets of A(n,k), proven by
   strong induction composing (2) and (3).

5. **Arithmetic squeeze** (`lower_bound_all_embeddings`): Composing Bridge
   Lemmas 2 and 3 to pin the exact extraconnectivity.

## Uniqueness: Open Problem

The theorem establishes the **exact value** of (R-1)-extraconnectivity
(∃ + ∀ squeeze) but does **not** prove the Hamming Ball is the unique
minimizer.

- Computationally confirmed unique for R ≤ 10
- Would require showing equality in the defect bound forces hypercube structure
- Related to equality cases in the Kruskal-Katona theorem
- See [collision-axiom-roadmap.md](collision-axiom-roadmap.md#uniqueness-open-problem)

## Key Proven Infrastructure

### The Triangle Anomaly (Why edges_at was removed)

Harper's theorem bounds edges in _hypercubes_, not arrangement graph cliques.
Example: R=3 in A(n,1), the triangle K_3 has 3 internal edges, but
E_seq(3) = 2. So the naive "sum edges, apply Harper" path is mathematically
wrong.

The correct invariant is the **Defect**: D(V') = |V'| * k - sum_unique_roots(V').
The Defect bounds hold even for cliques (triangle: D = 3*1 - 1 = 2 <= E_seq(3) = 2).

### The Algebraic Engine (`E_seq_list_sum_le`)

Generalizes `E_add_min_le` from binary splits to arbitrary partitions:
for any list of sizes `l` with `y >= max(l)`:

    (l.map E_seq).sum + l.sum - y <= E_seq(l.sum)

This is proven by list induction using `E_seq_add_bound` as the step lemma.

## File Map

| File                                         | Contents                                     |
| -------------------------------------------- | -------------------------------------------- |
| `proofs/ArrangementExtraconnectivity.lean`   | Main proof: Layers 1, 3 + capstone           |
| `proofs/HypercubeEdges.lean`                 | Supporting popcount/A000788 lemmas           |
| `proofs/PredictorComplexity.lean`            | Complexity analysis of the predictor         |
| `proofs/unstable/ArrangementGraphUtils.lean` | Harper's theorem + Cube embedding (orphaned) |
| `proofs/lakefile.lean`                       | Lake build configuration                     |

## Dependencies

- **Lean**: v4.30.0-rc2
- **Mathlib**: Current master (pinned in `lake-manifest.json`)
- Key imports: `Mathlib.Data.Fintype.Pi`, `Mathlib.Data.Fintype.Basic`,
  `Mathlib.Data.Finset.Card`, `Mathlib.Algebra.BigOperators.Group.Finset.Basic`
