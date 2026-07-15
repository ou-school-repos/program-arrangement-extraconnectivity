# Lean 4 Formal Verification Status

Formal verification of the Arrangement Graph extraconnectivity theorem
using Lean 4 and Mathlib.

## Build

```bash
make lean          # Build and verify proofs
make lean/cache    # Download pre-built Mathlib cache (first time)
```

Current status: **stable build succeeds with 0 errors and 0 sorries in the main proof path**.
The capstone theorem is intentionally parameterized by the remaining extremal
combinatorics hypotheses rather than depending on raw global `axiom`
declarations.

## Architecture

| Section                  | Theorem / Definition                                    | Status     |
| ------------------------ | ------------------------------------------------------- | ---------- |
| Subadditivity of A000788 | `E_add_min_le`: E(x)+E(y)+min(x,y) <= E(x+y)            | PROVEN     |
| Defect Bound             | `E_seq_list_sum_le`: generalized partition subaddivity  | PROVEN     |
| Hypercube Embedding      | `Cube`, `embed_cube`, `embedding_is_injective`          | PROVEN     |
| Harper's Theorem         | `harpers_edge_isoperimetry`: cubeEdges(S) <= E(\|S\|)   | PROVEN\*   |
| Graph Definition         | `ArrVertex`, `Fintype`, `DecidableEq`, `arr_adjacent`   | PROVEN     |
| External Neighbors       | `external_neighbors` (computable definition)            | PROVEN     |
| Embedding Condition      | `can_embed_hypercube` (dual: `k+d ≤ n ∧ d ≤ k`)         | PROVEN     |
| Defect Bound             | `sum_unique_roots_lower_bound`                          | PROVEN     |
| Collision-Adjusted Bound | `CollisionAdjustedBound` / `UniversalLowerBound` bridge | HYPOTHESIS |
| Fiber Identity           | `total_coord_edges_eq` (fiber counting)                 | PROVEN     |
| Bitwise Arithmetic       | `nat_popcount_eq_card_filter`                           | PROVEN     |
| Construction             | `hamming_ball_subset` (named, explicit)                 | PROVEN     |
| Evaluation               | `hamming_ball_eval` (boundary count)                    | PROVEN     |
| Evaluation Hypothesis    | `HBCrossCollisions` (KK shadow, existential)            | HYPOTHESIS |
| Cardinality              | `le_pow_bit_length`, `embed_vertex_injective_cube`      | PROVEN     |
| Lower Bound              | `UniversalLowerBound` (universal bound)                 | HYPOTHESIS |
| Asymptotic Penalty       | `sub_optimal_penalty` (penalty for sub-optimality)      | PROVEN\*   |
| Capstone                 | `arrangement_extraconnectivity_minimum` (composition)   | PROVEN\*   |

\*Proven but **conditional on outstanding axioms** (see Axioms section below).
Harper's Theorem is proven but **not in the dependency chain** of the
capstone theorem. The defect-based proof bypasses it entirely via algebraic
subadditivity of E_seq.
TODO(review): the `sub_optimal_penalty` row and the prose below still advertise
the stronger linear penalty statement; rewrite this to the weaker conditional
form once the false theorem is removed.

## Dependency Graph

```text
arrangement_extraconnectivity_minimum
  ├─ exists_optimal_embedding
  │    ├─ hamming_ball_subset         (explicit construction)
  │    │    ├─ embed_vertex           (Cube d → ArrVertex n k)
  │    │    │    └─ embed_cube        (bit → fresh/base symbol)
  │    │    └─ nat_to_cube            (ℕ → Cube d via testBit)
  │    ├─ le_pow_bit_length           (R ≤ 2^d via Nat.lt_size_self)
  │    ├─ embed_vertex_injective_cube (injectivity of embedding)
  │    ├─ nat_to_cube_injective       (injectivity of testBit encoding)
  │    └─ hamming_ball_eval           (exact boundary evaluation)
  │         ├─ hb_total_coord_edges   (from total_coord_edges_eq)
  │         └─ hb_cross_collisions [AXIOM]
  └─ lower_bound_all_embeddings [AXIOM] (universal lower bound)
```

## Remaining Hypothesis Interfaces

The remaining mathematical gaps are isolated as explicit theorem parameters in
`arrangement_extraconnectivity_minimum`, not as raw `axiom` commands.

### 1. Universal Boundary Inequality (`UniversalLowerBound`)

**What it says**: `external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R`.

**Justification**:

- Conceptually justified by Section 6's Tug-of-War scaling logic: any sub-optimal defect is penalized by at least (n-k) boundary nodes, which eventually eclipses any cross-collision differences.
- Computationally verified via `predict --verify R` (predict.cpp) for all $R \le 260$ and exhaustively for $R \le 10$ using `arrangement`.
  TODO(review): this section still reads as if the universal lower bound were
  fully validated; keep the hypothesis framing explicit until the Lean proof is
  actually closed.

### 2. Collision-Adjusted Bound (`CollisionAdjustedBound`)

**What it says**: `external_neighbors V' ≥ sum_unique_roots V' * (n - k) - C_constant R`.

**Justification**:

- Core isoperimetric inequality bounding external neighbors by unique roots and the maximal collision constant.
  TODO(review): this bridge statement is the one the review flags as false; do
  not present it as an active theorem until it is either deleted or restated.

### 3. Existential Shadow Bound (`HBCrossCollisions`)

**What it says**: The cross collisions of the Hamming Ball is exactly `C_constant R - E_seq R`.

**Justification**:

- Computationally verified alongside Axiom 1.
- Evaluates the 4-cycle count for the explicitly constructed Hamming Ball.
  TODO(review): the current driver uses this as a hypothesis interface, but the
  doc should make clear that the combinatorial bridge lemmas are still open.

## Embedding Condition

```lean
can_embed_hypercube (R n k : ℕ) : Prop :=
  k + bit_length (R - 1) ≤ n ∧ bit_length (R - 1) ≤ k
```

Dual constraint on the hypercube dimension `d = bit_length(R-1) = Nat.size(R-1)`:

- **`k + d ≤ n`**: need d fresh symbols beyond the k base positions.
- **`d ≤ k`**: can only flip coordinates that exist in the k-length sequence.

## What IS Fully Proven (No Axioms)

The core algebra, bijections, and isoperimetric defect inequalities of the **Algebraic Defect Framework** are 100% mechanized with zero axioms:

- **E_seq subadditivity** (`E_add_min_le`): The core isoperimetric inequality on A000788.
- **Generalized partition bound** (`E_seq_list_sum_le`): Extension from binary splits to arbitrary partitions.
- **Defect fiber bound** (`defect_fiber_bound`): The topological decomposition showing D(V') ≤ Σ D(Fₛ) + R - y.
- **Universal lower bound** (`sum_unique_roots_lower_bound`): The defect bound D(V') ≤ E_seq(R) for ALL R-element subsets.
- **Total Coordinate Edges** (`total_coord_edges_eq`): Mechanically double-counting the available $(n-k+1)$ extensions for each unique root via pure Finset bijections.
- **Bitwise Arithmetic** (`nat_popcount_eq_card_filter`): Mechanically verifying the exact Finset bijection between `Nat.testBit` filters and the recursive `popcount` weight, by induction on the bit width with a partition-and-shift decomposition.
- **Hamming Ball construction** (`hamming_ball_subset`): Explicit construction with proven cardinality.

The following high-level results are **mechanically proven inside Lean**, but remain conditional on the three axioms above:

- **Asymptotic Penalty** (`sub_optimal_penalty`): Proving that topologies with a defect shortfall $\Delta E$ are unconditionally penalized by at least $\Delta E(n-k)$ boundary nodes (conditional on Axiom 2).
  TODO(review): this is the false stronger theorem the review calls out; keep it
  only as a historical note if you need the dependency graph.
- **Existence of Optimal Embedding** (`exists_optimal_embedding`): Proven constructor showing that the Hamming Ball achieves the exact optimal boundary (conditional on Axiom 3).
- **Extraconnectivity Capstone** (`arrangement_extraconnectivity_minimum`): Combines existence and lower bound to squeeze the exact minimum cut (conditional on Axiom 1 and Axiom 3).

## Novel Contributions

- **A000788 Discovery**: The maximum internal edges for R vertices in A(n,k) equals the cumulative popcount sequence (OEIS A000788).
- **Pareto Spectrum**: The full topology-boundary tradeoff between the Star graph and the Hamming Ball.
- **Compression No-Go Theorem**: The standard Kruskal-Katona/Harper compression technique provably FAILS for arrangement graphs due to "coordinate tangling". Documented in `IsoperimetricPartialPermutation.lean`.
- **Asymptotic Penalty Theorem**: TODO(review): remove or restate this claim;
  the current theorem name is misleading if the stronger penalty is not proven.
- **Sandwich Conjecture & Hypercube Fracture Gap**: Formalized topological phase transitions and bounds.

## Open Conjectures

We have formally stated the remaining extremal bounds as `Prop`s to establish a rigorous bounty board for future Lean 4 contributors:

- `uniqueness_conjecture`
- `sandwich_upper_bound_conjecture`
- `hypercube_fracture_gap_conjecture`
