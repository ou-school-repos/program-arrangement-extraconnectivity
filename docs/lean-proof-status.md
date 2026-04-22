# Lean 4 Formal Verification Status

Formal verification of the Arrangement Graph extraconnectivity theorem
using Lean 4 and Mathlib.

## Build

```bash
make lean          # Build and verify proofs
make lean/cache    # Download pre-built Mathlib cache (first time)
```

Current status: **916 jobs, 0 errors, 3 sorries**.

## Architecture

| Layer                  | Theorem / Definition                                   | Status   |
| ---------------------- | ------------------------------------------------------ | -------- |
| 1 - Combinatorics      | `E_add_min_le`: E(x)+E(y)+min(x,y) <= E(x+y)           | PROVEN   |
| 1.5 - Algebraic Engine | `E_seq_list_sum_le`: generalized partition subaddivity | PROVEN   |
| 2 - Harper's Theorem   | `harpers_edge_isoperimetry`: cubeEdges(S) <= E(\|S\|)  | PROVEN   |
| 3 - Embedding          | `embed_cube`, `embedding_is_injective`                 | PROVEN   |
| 3 - Infrastructure     | `ArrVertex`, `Fintype`, `DecidableEq`, `arr_adjacent`  | PROVEN   |
| 3 - External Neighbors | `external_neighbors` (computable definition)           | PROVEN   |
| 3.5 - Bridge Lemmas    | `sum_unique_roots_lower_bound` (induction + engine)    | PROVEN\* |
| 3.5 - Bridge Lemmas    | `lower_bound_all_embeddings` (arithmetic composition)  | PROVEN   |
| Capstone               | `arrangement_extraconnectivity_minimum` (composition)  | PROVEN   |

\*Proven modulo `defect_fiber_bound` helper (see Remaining Sorries below).

## Remaining Sorries (3)

### Sorry 1: `defect_fiber_bound` -- Root Disjointness (L394, 1 sub-sorry)

**What it says**: D(V') <= sum D(F_s) + R - y, where F_s are fibers at a
disagreement coordinate p and y = unique_roots p V'.

**Current state**: The lemma is 95% proven. All 4 subgoals of the `refine`
are closed EXCEPT for `h_decomp` — the decomposition inequality that requires:

1. **Root disjointness at q != p**: For positions q != p, `drop_pos` at q
   retains coordinate p. If two vertices are in different fibers (different
   symbol at p), their roots at q are automatically distinct. This gives:
   `unique_roots q V' = sum_s unique_roots q F_s` for q != p.

2. **Injectivity at p**: `unique_roots p F_s = c_s` (drop_pos injective on
   each fiber). Already proven in Subgoal 2.

3. **Algebraic composition**: Combine (1) and (2) to get:
   `sum_unique_roots V' = y + sum_s sum_unique_roots F_s - R`
   Then: `D(V') = R*k - sum_unique_roots V' = sum D(F_s) + R - y`.

**What's proven in defect_fiber_bound**:

- ✅ Subgoal 1: `l.sum = V'.card` (fiberwise partition via `card_eq_sum_card_fiberwise`)
- ✅ Subgoal 2: `max(l) <= y` (drop_pos injective on fibers via `card_le_card_of_injOn`)
- ✅ Subgoal 3: `forall c in l, c < V'.card` (strict decrease via `card_lt_card`)
- ✅ Step A: IH application to each fiber (via `ih`)
- ✅ `h_sum_ih`: Pointwise IH sum (via list induction)
- ✅ Final chain: `h_decomp + h_sum_ih -> goal` (via `List.map_map` + `omega`)
- ⬜ `h_decomp`: Root disjointness identity (THE final piece)

**Estimated effort**: ~20-30 lines for root disjointness.

**Question for advisor**: The disjointness proof needs
`Finset.disjoint_left` or `Finset.disjoint_filter` to show that at position
q != p, images of different fibers under `drop_pos . q` are disjoint (because
they differ at coordinate p, which is retained by `drop_pos . q`). Then we
need to sum unique_roots over fibers and relate to unique_roots of V' via
`Finset.card_biUnion`. Is there a single Mathlib lemma that gives
`|union_s F_s.image f| = sum_s |F_s.image f|` given pairwise disjointness?

### Sorry 2: `external_neighbors_bound` (L602) -- Collision Formula

**What it says**: `|N(V')| >= sum_unique_roots * (n-k) - C_constant(R)`.

**What it needs**: Each unique root at position p can be extended by (n-k)
fresh symbols to form distinct external neighbors. The collision constant
C_constant(R) bounds the maximum overlaps from reused "named" symbols.

**Estimated effort**: ~100-150 lines. This is the deepest counting argument.

### Sorry 3: `exists_optimal_embedding` (L611) -- Constructive Upper Bound

**What it says**: There exists a Finset of size R achieving the formula exactly.

**What it needs**: Construct the Hamming ball as a Finset of arrangement
vertices using `Nat.testBit` and compute external_neighbors exactly on it.

**Estimated effort**: ~80-100 lines.

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

| File                                       | Contents                             |
| ------------------------------------------ | ------------------------------------ |
| `proofs/ArrangementExtraconnectivity.lean` | Main proof: Layers 1-3 + capstone    |
| `proofs/HypercubeEdges.lean`               | Supporting popcount/A000788 lemmas   |
| `proofs/PredictorComplexity.lean`          | Complexity analysis of the predictor |
| `proofs/lakefile.lean`                     | Lake build configuration             |

## Dependencies

- **Lean**: v4.30.0-rc2
- **Mathlib**: Current master (pinned in `lake-manifest.json`)
- Key imports: `Mathlib.Data.Fintype.Pi`, `Mathlib.Data.Fintype.Basic`,
  `Mathlib.Data.Finset.Card`, `Mathlib.Algebra.BigOperators.Group.Finset.Basic`
