# Collision Axiom Formalization Roadmap

The remaining extremal-combinatorics gaps in `ArrangementExtraconnectivity.lean`
are isolated as explicit hypothesis interfaces (Lean `def ... : Prop`
parameters to the capstone theorem), not raw global axioms:

- **`UniversalLowerBound (R n k : ℕ) : Prop`** — the universal boundary
  inequality `external_neighbors V' ≥ (R*k − E_seq R)*(n-k) − C_constant R`
  for every `R`-element subset `V'`.
- **`HBCrossCollisions (R n k d : ℕ) ... : Prop`** — the exact evaluation
  `cross_collisions(HB_R) = C_constant(R) − E_seq(R)` for the explicitly
  constructed Hamming Ball.

This document describes what a complete mechanized proof of each would
require.

## Current Status

- **`HBCrossCollisions` is not yet on the stable capstone path.**
  `CrossTop.lean` contains a direct proof candidate
  (`hb_cross_collisions_closed`) for every `R ≥ 1`, but the public capstone
  still accepts `HBCrossCollisions` as an explicit hypothesis.  It must remain
  an interface until the supporting proof work is reconciled, including the
  `CrossRecurrence` route and the remaining unstable-scaffold obligations.
  Only then should the direct route be promoted, its hypothesis removed from
  `arrangement_extraconnectivity_minimum`, and the status changed to proven.
- **Why the driver route remains research scaffolding.** The strong
  induction driver `hb_cross_collisions_of_recurrence` in
  `CrossCollisionsResearch.lean` is complete and reduces `HBCrossCollisions` to
  three interface lemmas — `CrossBaseOne` and `CrossDimStable` are proven, but
  the third, `CrossRecurrence`, is genuinely circular as an induction step: its
  `ext_cube(d,m)` term is not an arithmetic quantity but is _equivalent to_
  `HBCrossCollisions(m)` itself (the RHS of `CrossRecurrence` is a set
  cardinality whose evaluation _is_ the theorem at the smaller size `m`), so
  the driver would be handing itself its own conclusion as a hypothesis with
  nothing new supplied. `CrossTop` proposes a direct,
  non-recursive closed-form identity with its own combinatorial proof (a
  vertex is double-counted in the top-heavy ball's boundary iff it is a cube
  vertex; every top-strip cube vertex has multiplicity ≥ 1 via its "bottom
  partner"; summing the excess multiplicity reduces to a plain edge-boundary
  count in one lower dimension). `CrossRecurrence` and its driver have been
  archived to `docs/archive/CrossRecurrenceDriver.lean` for reference but
  remains a required reconciliation point before the direct route can be
  promoted into the stable capstone.
- **Formula values verified** via `predict --verify R` (predict.cpp) for
  `R ≤ 160`; the full sweep through `R = 260` remains pending. Exhaustive
  `arrangement` nauty-based search covers
  `R ≤ 10`.
- **`UniversalLowerBound` has no active Lean formalization strategy right
  now.** An earlier support-projection/Boolean-cube-injection approach was
  explored and abandoned — it relied on an inequality later refuted by a
  direct counterexample. See
  `docs/archive/collision-axiom-support-projection-abandoned.md` for the
  historical record; do not resume from it without addressing the
  counterexample first. The current math-first candidate is the equivalent
  weighted potential inequality
  `X(V') + (n-k+1) · D(V') ≤ C(R) + (n-k) · E(R)`.
  Exhaustive C++ testing now covers 24 parameter rows (up to 190M subsets)
  with no counterexample. The standard guarded symbol compression is known to
  increase boundary on a two-vertex `A(4,2)` example, so it cannot establish
  this candidate; see `docs/universal-lower-bound-work.md`.  Four further
  candidate proof mechanisms have been closed by finite counterexamples:
  edge-gradient charging overcounts collision mass, single-vertex
  hole-filling does not preserve its slack, the tested guarded
  Pinto-style dual-compression pair has no admissible non-worsening branch on
  finite witnesses, and the additive convex fiber-size Lyapunov ansatz
  Ψ_f(V') = Σ_s N_s(V') w_s is refuted by a normalized LP diagnostic
  (`scripts/lyapunov_profile_check.py`): the embedded-cube equalities
  alone are algebraically inconsistent for A(5,3)/A(6,3), and A(4,3)
  equality weights violate the corpus lower bound.  These closures do not
  alter any Lean interface; a non-additive Lyapunov extension (R-dependent
  weights, coordinate-coupled terms, or multi-root intersection terms) or
  a fundamentally different compression operator remains to be formalized.

## Immediate Lean Work Queue

1. `UniversalLowerBound` needs a formalization strategy from scratch (see
   "Current Status" above); it is the only live mathematical hypothesis of
   the capstone.
2. The archived recurrence driver remains available for research reference but
   is not a work-queue item.

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
- `Mathlib.Combinatorics.SetFamily.KruskalKatona` has the full KK inequality
  for standard set families
- Connecting that theorem to `A(n,k)`'s ordered, injective-sequence structure
  is exactly the open problem — see "Current Status" above; a previous
  attempt at this connection was abandoned (archived).

### Estimated effort

~200-300 lines for:

- Colex ordering on `Finset (Fin d → Bool)`
- Connection between shadow size and 4-cycle count, proven correctly
  (the archived attempt's version of this connection was refuted)

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

The permutation constraint (no duplicate symbols) in A(n,k) should relate
collision counts in A(n,k) to collision counts in the full Hamming graph
H(k,n), but the specific inequality chain explored for this was flagged as
directionally invalid and has been moved to
`docs/archive/collision-axiom-support-projection-abandoned.md`. This step
needs to be rederived from scratch, not resumed from the archived version.

### Estimated effort

~100-150 lines (mostly boilerplate connecting the two graph definitions),
plus whatever it costs to find a valid version of the transfer inequality.

## Step 4: Deriving C_constant(R)

### Status: SUPERSEDED (see `CrossTop.lean`, "Current Status" above)

`proofs/Arrangement/unstable/CrossCollisionsResearch.lean` formally proves the
entire arithmetic and inductive backbone for a _recursive_ form of the
`C_constant(R)` identity:

- Verified the binary reflection arithmetic decompositions of `E_seq` and `sum_bit_length` on the natural numbers, handling all exact subtractions on $\mathbb{N}$ safely without truncation.
- Formally proved the exact recurrence $C(R) = C(2^{d-1}) + C(m) + ext\_cube(d, m) + m$ where $R = 2^{d-1} + m$ and $0 < m \le 2^{d-1}$.
- Implemented a complete strong induction driver `hb_cross_collisions_of_recurrence` using `Nat.strong_induction_on` which reduces the final target statement `HBCrossCollisions` for all $R \ge 1$ to three pure combinatorial interface lemmas:
  1. `CrossBaseOne` (single-vertex ball has 0 collisions) — proven
  2. `CrossDimStable` (fresh dimensions don't change the set) — proven
  3. `CrossRecurrence` (the corrected combinatorial split) — genuinely circular as a proof target (see "Current Status"), not merely unfinished

This arithmetic work is not wasted — `E_seq_sum_decomposition`,
`sum_bit_length_sum_decomposition`, `sum_bit_length_pow`, and
`E_seq_le_sum_bit_length` are reused directly by `CrossTop.lean`'s closed-form
route — but the driver itself and `CrossRecurrence` are not the path forward.

### What remains

The direct route must be reconciled with the remaining recurrence and unstable
scaffold obligations, then wired into the public capstone to discharge the
`HBCrossCollisions` parameter.  `UniversalLowerBound` remains the separate
universal-boundary obligation.

## Total Remaining Estimated Effort

**`HBCrossCollisions`**: still an explicit capstone hypothesis.  The direct
`CrossTop.lean` route and the remaining recurrence/scaffold obligations must
be reconciled before it can be discharged and removed from the capstone API.

**`UniversalLowerBound`**: has no working formalization strategy. Steps 1-3
above (Kruskal-Katona shadow operators, Hamming Ball maximizes squares,
transfer to arrangement graphs) sketch what a Kruskal-Katona-based approach to
_this_ hypothesis would need (~450-650 lines combined), but the specific
version of that approach explored so far was abandoned after a counterexample
refuted its central inequality (see
`docs/archive/collision-axiom-support-projection-abandoned.md`); a corrected
approach would need to be found before any effort estimate is reliable, so
this remains currently unscoped.

## Uniqueness (Open Problem)

The current proof establishes that the Hamming Ball **achieves** the minimum
external boundary, but does not prove it is the **unique** minimizer.

### What uniqueness would require

- Show that equality in the defect bound `D(V') = E_seq(|V'|)` forces
  V' to be isomorphic to a Hamming Ball
- This is equivalent to showing that E_seq is **strictly** subadditive
  for non-Hamming-Ball partitions
- The computational search confirms uniqueness for small R (one minimum-cut
  topology class per R); formula values verified via `predict --verify`
- Formalized as `uniqueness_conjecture` using the full automorphism group
  S_n × S_k (symbol permutation σ + coordinate permutation τ)

### Why this is hard

Equality cases in Kruskal-Katona are known but technically involved.
The transfer to arrangement graphs adds another layer of complexity
because the permutation constraint may create additional minimizers
in degenerate cases (small n-k).

This remains an open question for future work.
