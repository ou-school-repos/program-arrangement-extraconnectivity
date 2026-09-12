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

| Section                  | Theorem / Definition                                                 | Status     |
| ------------------------ | -------------------------------------------------------------------- | ---------- |
| Subadditivity of A000788 | `E_add_min_le`: E(x)+E(y)+min(x,y) <= E(x+y)                         | PROVEN     |
| Defect Bound             | `E_seq_list_sum_le`: generalized partition subadditivity             | PROVEN     |
| Hypercube Embedding      | `Cube`, `embed_cube`, `embedding_is_injective`                       | PROVEN     |
| Harper's Theorem         | `harpers_edge_isoperimetry`: cubeEdges(S) <= E(\|S\|)                | PROVEN\*   |
| Graph Definition         | `ArrVertex`, `Fintype`, `DecidableEq`, `arr_adjacent`                | PROVEN     |
| External Neighbors       | `external_neighbors` (computable definition)                         | PROVEN     |
| Embedding Condition      | `can_embed_hypercube` (dual: `k+d ≤ n ∧ d ≤ k`)                      | PROVEN     |
| Defect Bound             | `sum_unique_roots_lower_bound`                                       | PROVEN     |
| Fiber Identity           | `total_coord_edges_eq` (fiber counting)                              | PROVEN     |
| Bitwise Arithmetic       | `nat_popcount_eq_card_filter`                                        | PROVEN     |
| Construction             | `hamming_ball_subset` (named, explicit)                              | PROVEN     |
| Evaluation               | `hamming_ball_eval` (boundary count)                                 | PROVEN\*   |
| Evaluation Hypothesis    | `HBCrossCollisions` (KK shadow, existential)                         | HYPOTHESIS |
| Cardinality              | `le_pow_bit_length`, `embed_vertex_injective_cube`                   | PROVEN     |
| Lower Bound              | `UniversalLowerBound` (universal bound)                              | HYPOTHESIS |
| Exact Penalty Identity   | `boundary_identity`, `penalty_exact`, `penalty_defect`, `penalty_ge` | PROVEN     |
| Capstone                 | `arrangement_extraconnectivity_minimum` (composition)                | PROVEN\*   |

\*Proven but **conditional on outstanding hypotheses** (see below).
Harper's Theorem is proven but **not in the dependency chain** of the
capstone theorem. The defect-based proof bypasses it entirely via algebraic
subadditivity of E_seq.

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
  │         └─ HBCrossCollisions [HYPOTHESIS]
  └─ UniversalLowerBound [HYPOTHESIS] (universal lower bound)
```

## Remaining Hypothesis Interfaces

The remaining mathematical gaps are isolated as explicit theorem parameters in
`arrangement_extraconnectivity_minimum`, not as raw `axiom` commands.

### 1. Universal Boundary Inequality (`UniversalLowerBound`)

**What it says**: `external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R`.

**Justification**:

- Conceptually justified by Section 6's Tug-of-War scaling logic: any sub-optimal defect is penalized by at least (n-k) boundary nodes, which eventually eclipses any cross-collision differences.
- Computationally verified via `predict --verify R` (predict.cpp) for all $R \le 260$ (the tool's enforced ceiling — brute-force verification memory is Theta(R^3 \cdot K); see docs/verifications.csv) and exhaustively for $R \le 10$ using `arrangement`.
  TODO(review): this section still reads as if the universal lower bound were
  fully validated; keep the hypothesis framing explicit until the Lean proof is
  actually closed.

### 2. Existential Shadow Bound (`HBCrossCollisions`)

**What it says**: The cross collisions of the Hamming Ball is exactly `C_constant R - E_seq R`.

**Justification**:

- Computationally verified alongside Axiom 1.
- Evaluates the 4-cycle count for the explicitly constructed Hamming Ball.
- **Active route, NOT complete: `Arrangement/CrossTop.lean`**. This route proves the unconditional closed form
  `cross_collisions(HB(2^{d-1}+m)) + 2·E_seq(m) = m·(d-1)` and derives `HBCrossCollisions` from it by pure
  arithmetic — no recursion. The Finset bookkeeping in `cross_collisions_eq_cube_sum` (`h_U_embed_sub`,
  `h_U_diff`, `h_sum_U`, `h_sum_U_embed`, `h_sum_Ico`, `h_card_U`, `h_card_embed`) is genuinely proven. But
  as of 2026-08-06 the file still has **7 unresolved holes**, using the `admit` tactic (a deprecated
  `sorry` alias — it still emits `sorryAx`, so counting literal `sorry` occurrences understates this): in
  `embed_mem_coord_boundary_iff` (4), `mem_two_boundaries_is_cube` (2, one of which — line ~669 — is the
  entire lemma body), and `cross_top` (1, a mechanical `2^(d-1)+2^(d-1)=2^d` endpoint rewrite). The
  combinatorial core connecting `cross_collisions` to hypercube counting (`embed_mem_coord_boundary_iff`,
  `mem_two_boundaries_is_cube`) is the real remaining work. The file was also not in `lakefile.lean`'s
  `roots` until this correction, so `lake build` never actually checked it — treat any prior "compiles"
  claim about it as unverified. Status should be gated on `#print axioms hb_cross_collisions_closed`
  showing no `sorryAx`, not on a `grep sorry` count.
- **Superseded route (`CrossCollisionsResearch.lean`)**: Previously attempted to prove this via strong induction on the recurrence (`CrossRecurrence`), but the recursion was shown to be structurally circular with respect to the goal. This route is now officially abandoned and replaced by `CrossTop.lean`.

### Superseded: `CollisionAdjustedBound` / `sub_optimal_penalty`

An earlier draft of the Asymptotic Penalty argument depended on a hypothesis
interface `CollisionAdjustedBound` (and a companion `sub_optimal_penalty`).
That hypothesis is **provably false**: the exact identity
`external_neighbors + cross_collisions + R*k = U(n-k+1)` (from
`total_coord_edges_eq`) makes it equivalent to `X + D ≤ C(R)`, which the Star
Graph refutes directly. It has been replaced by the unconditional identities
in `Arrangement/PenaltyExact.lean` (`boundary_identity`, `penalty_exact`,
`penalty_defect`, `penalty_ge`), which require no hypothesis beyond equal
cardinality and `k ≤ n`. Do not reintroduce `CollisionAdjustedBound` as a live
hypothesis interface in future status writeups.

## Embedding Condition

```lean
can_embed_hypercube (R n k : ℕ) : Prop :=
  k + bit_length (R - 1) ≤ n ∧ bit_length (R - 1) ≤ k
```

Dual constraint on the hypercube dimension `d = bit_length(R-1) = Nat.size(R-1)`:

- **`k + d ≤ n`**: need d fresh symbols beyond the k base positions.
- **`d ≤ k`**: can only flip coordinates that exist in the k-length sequence.

## What IS Fully Proven (No Hypotheses)

The core algebra, bijections, and isoperimetric defect inequalities of the **Algebraic Defect Framework** are 100% mechanized with zero remaining hypotheses:

- **E_seq subadditivity** (`E_add_min_le`): The core isoperimetric inequality on A000788.
- **Generalized partition bound** (`E_seq_list_sum_le`): Extension from binary splits to arbitrary partitions.
- **Defect fiber bound** (`defect_fiber_bound`): The topological decomposition showing D(V') ≤ Σ D(Fₛ) + R - y.
- **Universal lower bound** (`sum_unique_roots_lower_bound`): The defect bound D(V') ≤ E_seq(R) for ALL R-element subsets.
- **Total Coordinate Edges** (`total_coord_edges_eq`): Mechanically double-counting the available $(n-k+1)$ extensions for each unique root via pure Finset bijections.
- **Bitwise Arithmetic** (`nat_popcount_eq_card_filter`): Mechanically verifying the exact Finset bijection between `Nat.testBit` filters and the recursive `popcount` weight, by induction on the bit width with a partition-and-shift decomposition.
- **Hamming Ball construction** (`hamming_ball_subset`): Explicit construction with proven cardinality.
- **Exact Penalty Identity** (`boundary_identity`, `penalty_exact`, `penalty_defect`, `penalty_ge` in `Arrangement/PenaltyExact.lean`): Unconditional boundary identities and comparative penalty formulas derived directly from `total_coord_edges_eq`.

The following high-level results are **mechanically proven inside Lean**, but remain conditional on the remaining extremal-combinatorics hypotheses above:

- **Existence of Optimal Embedding** (`exists_optimal_embedding`): Proven constructor showing that the Hamming Ball achieves the exact optimal boundary (conditional on `HBCrossCollisions`).
- **Extraconnectivity Capstone** (`arrangement_extraconnectivity_minimum`): Combines existence and lower bound to squeeze the exact minimum cut (conditional on `HBCrossCollisions` and `UniversalLowerBound`).

## Novel Contributions

- **A000788 Discovery**: The maximum internal edges for R vertices in A(n,k) equals the cumulative popcount sequence (OEIS A000788).
- **Pareto Spectrum**: The full topology-boundary tradeoff between the Star graph and the Hamming Ball.
- **Compression No-Go Theorem**: The standard Kruskal-Katona/Harper compression technique provably FAILS for arrangement graphs due to "coordinate tangling". Documented in `IsoperimetricPartialPermutation.lean`.
- **Exact Penalty Framework**: `Arrangement/PenaltyExact.lean` proves the unconditional boundary identity and comparative penalty formulas used by the paper's asymptotic-penalty section.
- **Sandwich Conjecture & Hypercube Fracture Gap**: Formalized topological phase transitions and bounds.

## Open Conjectures

We have formally stated the remaining extremal bounds as `Prop`s to establish a rigorous bounty board for future Lean 4 contributors:

- `uniqueness_conjecture`
- `sandwich_upper_bound_conjecture`
- `hypercube_fracture_gap_conjecture`
