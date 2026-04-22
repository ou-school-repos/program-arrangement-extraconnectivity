# Lean 4 Formal Verification Status

Formal verification of the Arrangement Graph extraconnectivity theorem
using Lean 4 and Mathlib.

## Build

```bash
make lean          # Build and verify proofs
make lean/cache    # Download pre-built Mathlib cache (first time)
```

Current status: **909 jobs, 0 errors, 4 sorries**.

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

## Remaining Sorries (4)

### Sorry 1: `defect_fiber_bound` (L393) -- Finset Partition Plumbing

**What it says**: For any V' with |V'| >= 2, there exists a list of fiber
sizes `l` and an overlap count `y` such that the Defect decomposes as
`D(V') <= sum E_seq(c_i) + R - y`.

**What it needs**: This is the core Finset wiring for the partition induction:

1. Find a coordinate p where vertices in V' disagree (exists since R >= 2)
2. Partition V' into fibers `V'.filter (fun v => v.val p = s)` for each symbol s
3. Prove `drop_pos` is injective on each fiber (same symbol at p => distinct
   roots must map distinctly)
4. Prove root disjointness at other positions q != p (key: `drop_pos v q`
   includes position p, so different symbols at p => different roots)
5. Compose the sum_unique_roots decomposition identity

**Estimated effort**: ~60-80 lines of Finset API (filter, image, card, disjoint).

**Question for advisor**: Is there a cleaner Mathlib API for "partition a
Finset by a function and sum over fibers"? Something like `Finset.disjiUnion`
or `Finset.sigma` might simplify the fiber card sum identity.

### Sorry 2: Singleton Base Case (L404)

**What it says**: For R=1, `sum_unique_roots V' >= k`.

**What it needs**: When V' is a singleton {v}, each `unique_roots p V'` = 1
(image of a singleton has card 1), and there are k coordinates. So the sum
is k. This requires:

- `Finset.image_singleton` or `Finset.card_image_of_injective` for singletons
- Connecting sum_unique_roots (defined via Multiset.map/sum) to k \* 1

**Estimated effort**: ~15-20 lines.

### Sorry 3: `external_neighbors_bound` (L443) -- Collision Formula

**What it says**: `|N(V')| >= sum_unique_roots * (n-k) - C_constant(R)`.

**What it needs**: Each unique root at position p can be extended by (n-k)
fresh symbols to form distinct external neighbors. The collision constant
C_constant(R) bounds the maximum overlaps from reused "named" symbols.

**Proof strategy** (from advisor):

- Define Candidates as (position, root, fresh_symbol) tuples
- Map candidates to external neighbor vertices
- Split fresh symbols into Anonymous (never collide across dimensions) and
  Named (collide, bounded by C_constant)
- Count via injection + collision bound

**Estimated effort**: ~100-150 lines. This is the deepest counting argument.

**Question for advisor**: Should we formalize the Named/Anonymous split as
two separate Finsets with a union bound, or use a single injection with a
collision correction term?

### Sorry 4: `exists_optimal_embedding` (L452) -- Constructive Upper Bound

**What it says**: There exists a Finset of size R achieving the formula exactly.

**What it needs**: Construct the Hamming ball as a Finset of arrangement
vertices and compute external_neighbors exactly on it.

**Critical issue** (from advisor): For arbitrary R (not just powers of 2),
we need the initial segment of lexicographic order in Q(d), not the full
cube. This requires:

- `nat_to_cube`: map integers 0..R-1 to binary representations in Cube d
- `hamming_ball_cube`: `(Finset.range R).image nat_to_cube`
- Prove card = R (injectivity of nat_to_cube for values < 2^d)
- Compute external_neighbors on this specific set to get exact equality

**Estimated effort**: ~80-100 lines.

**Question for advisor**: Since the `embed_vertex` injectivity is already
proven, the main work is the nat_to_cube cardinality. Should we use
`Nat.binaryRec` or a direct `fun i => (x / 2^i) % 2 == 1` definition?

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
  `Mathlib.Data.Finset.Card`
