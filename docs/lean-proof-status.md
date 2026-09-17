# Lean 4 Formal Verification Status

Formal verification of the Arrangement Graph extraconnectivity theorem using
Lean 4 and Mathlib.

## Build

**Mathematical status update (2026-09-16):** The unrestricted
UniversalLowerBound hypothesis is false (full Star in A(10,8), R=17, boundary
168 < 169). The capstone now uses `RestrictedLowerBound`, gated by the hypercube
embedding conditions. The embedding condition is not itself a proof of
optimality: Star counterexamples exist inside R ≤ 2^m once m ≥ 5. The restricted
boundary proof remains open; see `proof-sketch-weighted-potential.md` for
details.

```bash
make lean          # Build and verify proofs
make lean/cache    # Download pre-built Mathlib cache (first time)
```

Current status: **stable build succeeds with 0 errors and 0 sorries in the main
proof path**. The capstone theorem is intentionally parameterized by the
remaining extremal combinatorics hypotheses rather than depending on raw global
`axiom` declarations.

## Architecture

<!-- markdownlint-disable MD013 -->

| Section                  | Theorem / Definition                                                 | Status        |
| ------------------------ | -------------------------------------------------------------------- | ------------- |
| Subadditivity of A000788 | `E_add_min_le`: E(x)+E(y)+min(x,y) <= E(x+y)                         | PROVEN        |
| Defect Bound             | `E_seq_list_sum_le`: generalized partition subadditivity             | PROVEN        |
| Hypercube Embedding      | `Cube`, `embed_cube`, `embedding_is_injective`                       | PROVEN        |
| Harper's Theorem         | `harpers_edge_isoperimetry`: cubeEdges(S) <= E(\|S\|)                | PROVEN\*      |
| Graph Definition         | `ArrVertex`, `Fintype`, `DecidableEq`, `arr_adjacent`                | PROVEN        |
| External Neighbors       | `external_neighbors` (computable definition)                         | PROVEN        |
| Embedding Condition      | `can_embed_hypercube` (dual: `k+d ≤ n ∧ d ≤ k`)                      | PROVEN        |
| Defect Bound             | `sum_unique_roots_lower_bound`                                       | PROVEN        |
| Fiber Identity           | `total_coord_edges_eq` (fiber counting)                              | PROVEN        |
| Bitwise Arithmetic       | `nat_popcount_eq_card_filter`                                        | PROVEN        |
| Construction             | `hamming_ball_subset` (named, explicit)                              | PROVEN        |
| Evaluation               | `hamming_ball_eval` (boundary count)                                 | PROVEN\*      |
| Hamming-ball evaluation  | `hb_cross_collisions_closed`                                         | PROVEN        |
| Cardinality              | `le_pow_bit_length`, `embed_vertex_injective_cube`                   | PROVEN        |
| Lower Bound              | `RestrictedLowerBound` (under embedding conditions)                  | HYPOTHESIS    |
| Exact Penalty Identity   | `boundary_identity`, `penalty_exact`, `penalty_defect`, `penalty_ge` | PROVEN        |
| Capstone                 | `arrangement_extraconnectivity_minimum` (composition)                | CONDITIONAL\* |

<!-- markdownlint-enable MD013 -->

\*Conditional on the per-instance `RestrictedLowerBound` hypothesis. The
Hamming-ball collision equality is proved directly by `CrossTop` and is used by
the wrapper theorem. Harper's Theorem is proven but **not in the dependency
chain** of the capstone theorem. The defect-based proof bypasses it entirely via
algebraic subadditivity of E_seq.

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
  │         └─ hb_cross_collisions_closed [PROVEN]
  └─ RestrictedLowerBound R n k [HYPOTHESIS]
```

## Remaining Hypothesis Interfaces

The remaining mathematical gaps are isolated as explicit theorem parameters in
`arrangement_extraconnectivity_minimum`, not as raw `axiom` commands.

### 1. Restricted Boundary Inequality (`RestrictedLowerBound`)

**What it says**:
`external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R`.

**Justification**:

- Conceptually justified by Section 6's Tug-of-War scaling logic: any
  sub-optimal defect is penalized by at least (n-k) boundary nodes, which
  eventually eclipses any cross-collision differences.
- Computationally confirmed via `predict --verify R` through $R \le 160$ and
  exhaustively for $R \le 10$ using `arrangement`. The configured predictor
  ceiling is 260, but the complete sweep through that ceiling is still pending.
- The recently tested edge-gradient, single-vertex hole-filling, guarded
  dual-compression, and additive fiber-size Lyapunov candidates are finite
  counterexamples to proposed proof mechanisms, not changes to this statement.
  Consequently no Lean theorem or axiom is removed or added:
  `RestrictedLowerBound` remains the active capstone hypothesis, gated by the
  hypercube embedding conditions. Those conditions do not establish the
  inequality: A(11,6), R=31≤2^5 is already a Star counterexample: the Star has
  boundary 450 while the proposed bound is 476. Thus
  `¬ ∀ R n k, RestrictedLowerBound R n k`; the public theorem now takes the
  hypothesis per instance. (`scripts/lyapunov_profile_check.py`) refutes the
  additive convex potential Ψ_f(V') = Σ_r f(|V' ∩ F_r|) even with Hamming-ball
  normalization: the embedded-cube equalities alone are algebraically
  inconsistent for A(5,3) and A(6,3), and A(4,3) admits equality weights that
  violate the corpus lower bound on an explicit witness.

### 2. Hamming-ball collision evaluation (`HBCrossCollisions`)

`CrossTop.lean` proves `hb_cross_collisions_closed` for nonempty Hamming balls,
and the `arrangement_boundary_minimum` wrapper consumes it directly. The lower
bound remains an explicit per-instance `RestrictedLowerBound` hypothesis.

### Superseded: `CollisionAdjustedBound` / `sub_optimal_penalty`

An earlier draft of the Asymptotic Penalty argument depended on a hypothesis
interface `CollisionAdjustedBound` (and a companion `sub_optimal_penalty`). That
hypothesis is **provably false**: the exact identity
`external_neighbors + cross_collisions + R*k = U(n-k+1)` (from
`total_coord_edges_eq`) makes it equivalent to `X + D ≤ C(R)`, which the Star
Graph refutes directly. It has been replaced by the unconditional identities in
`Arrangement/PenaltyExact.lean` (`boundary_identity`, `penalty_exact`,
`penalty_defect`, `penalty_ge`), which require no hypothesis beyond equal
cardinality and `k ≤ n`. Do not reintroduce `CollisionAdjustedBound` as a live
hypothesis interface in future status writeups.

## Embedding Condition

```lean
can_embed_hypercube (R n k : ℕ) : Prop :=
  k + bit_length (R - 1) ≤ n ∧ bit_length (R - 1) ≤ k
```

Dual constraint on the hypercube dimension
`d = bit_length(R-1) = Nat.size(R-1)`:

- **`k + d ≤ n`**: need d fresh symbols beyond the k base positions.
- **`d ≤ k`**: can only flip coordinates that exist in the k-length sequence.

## What IS Fully Proven (No Hypotheses)

The core algebra, bijections, and isoperimetric defect inequalities of the
**Algebraic Defect Framework** are 100% mechanized with zero remaining
hypotheses:

- **E_seq subadditivity** (`E_add_min_le`): The core isoperimetric inequality on
  A000788.
- **Generalized partition bound** (`E_seq_list_sum_le`): Extension from binary
  splits to arbitrary partitions.
- **Defect fiber bound** (`defect_fiber_bound`): The topological decomposition
  showing D(V') ≤ Σ D(Fₛ) + R - y.
- **Universal lower bound** (`sum_unique_roots_lower_bound`): The defect bound
  D(V') ≤ E_seq(R) for ALL R-element subsets.
- **Total Coordinate Edges** (`total_coord_edges_eq`): Mechanically
  double-counting the available $(n-k+1)$ extensions for each unique root via
  pure Finset bijections.
- **Bitwise Arithmetic** (`nat_popcount_eq_card_filter`): Mechanically verifying
  the exact Finset bijection between `Nat.testBit` filters and the recursive
  `popcount` weight, by induction on the bit width with a partition-and-shift
  decomposition.
- **Hamming Ball construction** (`hamming_ball_subset`): Explicit construction
  with proven cardinality.
- **Exact Penalty Identity** (`boundary_identity`, `penalty_exact`,
  `penalty_defect`, `penalty_ge` in `Arrangement/PenaltyExact.lean`):
  Unconditional boundary identities and comparative penalty formulas derived
  directly from `total_coord_edges_eq`.

The following high-level results are **mechanically proven inside Lean**; the
public capstone remains conditional on `RestrictedLowerBound`:

- **Existence of Optimal Embedding** (`exists_optimal_embedding`): Proven
  constructor, conditional on its explicit `HBCrossCollisions` argument.
- **Boundary capstone** (`arrangement_boundary_minimum`): Combines
  `RestrictedLowerBound` and the explicit Hamming-ball collision interface to
  squeeze the exact minimum cut.

## Novel Contributions

- **A000788 defect bound**: The universal bound applies to the defect, not to
  internal edge count; arrangement lines can contain triangles.
- **Pareto spectrum**: exploratory topology-boundary data, not a completed
  classification.
- **Compression diagnostic**: one standard Kruskal-Katona/Harper compression
  operator fails on tested arrangement configurations; this is not a general
  no-go theorem.
- **Exact Penalty Framework**: `Arrangement/PenaltyExact.lean` proves the
  unconditional boundary identity and comparative penalty formulas used by the
  paper's asymptotic-penalty section.
- **Sandwich conjecture & hypercube fracture gap**: retained as explicitly open
  interfaces/diagnostics; the proposed sandwich upper bound is refuted by the
  four-vertex path in A(6,3), and the fracture claims do not follow from Harper
  because arrangement lines are cliques.

## Open Conjectures

We have formally stated the remaining extremal bounds as `Prop`s to establish a
rigorous bounty board for future Lean 4 contributors:

- `uniqueness_conjecture`
- `sandwich_upper_bound_conjecture`
- `hypercube_fracture_gap_conjecture`

## Appendix: Finite Evidence Isolates the Remaining Induction Lemma

The proof of `UniversalLowerBound` via coordinate-split induction requires
bounding the geometric cross-collision overhead against the arithmetic surplus
of the potential $P(R) = C(R) + mE(R)$. While a zero-slack raw induction step
fails globally, finite computational certificates rigorously isolate the shape
of the required correction.

- **Zero-Correction in Favorable Regimes:** For all subsets in A(4,3) through
  R=8 (1,271,601 concrete sets), a zero-correction induction is universally
  feasible. For every set, there exists at least one coordinate split where the
  raw arithmetic surplus strictly covers the geometric overhead. The
  `exact-menu` CP-SAT solver with `--minimize-sum-g` certifies sum G = 0 across
  the entire universe.
- **Finite Profile-State Certificates:** Bounded `exact-menu` CP-SAT evaluations
  certify that a nonnegative, recursive profile-state potential
  $G(\text{state})$ successfully closes the induction for all subsets in A(4,2)
  (R≤5), A(5,2) (R≤6), and A(4,3) (R≤7) with small integer bounds (G≤4 before
  minimization; G=0 after).
- **The Zero-Correction Obstruction:** The uncorrected "best coordinate"
  selection rule is provably not universal. Targeted capacity-valid Star Graphs
  in larger cells generate strictly negative raw gaps across _every_ coordinate
  split:
  - A(7,4), R=11: best gap = −1
  - A(8,4), R=17: best gap = −2
  - A(8,5), R=15: best gap = −2
- **The Open Induction Lemma:** These finite tests isolate the remaining gap.
  The induction requires a recursive profile-state correction function
  $G(\text{state})$ that telescopes to absorb these local hub-vertex deficits:
  $G(\text{parent}) \le \text{gap}(V,p) + \sum G(\text{children})$. The exact
  closed-form equation for $G$ and its corresponding Lean 4 formalization remain
  open.
