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

- **Isolated as explicit Lean hypotheses** in the stable capstone theorem, rather
  than declared as raw global axioms.
- **Arithmetic & inductive scaffold for `HBCrossCollisions`** lives in
  `proofs/Arrangement/unstable/CrossCollisionsResearch.lean`. The file now
  checks standalone. The full induction driver
  (`hb_cross_collisions_of_recurrence`) is complete and reduces the entire
  hypothesis to three combinatorial interface lemmas (see below); several
  unrelated unstable arithmetic placeholders in the same file remain explicit
  `sorry`s while that scaffold is repaired independently.
- **Formula values verified** via `predict --verify R` (predict.cpp) for
  `R ≤ 260`, and exhaustively via the `arrangement` nauty-based search for
  `R ≤ 10`.
- **`UniversalLowerBound` has no active Lean formalization strategy right
  now.** An earlier support-projection/Boolean-cube-injection approach was
  explored and abandoned — it relied on an inequality later refuted by a
  direct counterexample. See
  `docs/archive/collision-axiom-support-projection-abandoned.md` for the
  historical record; do not resume from it without addressing the
  counterexample first.

## Immediate Lean Work Queue

1. Finish the three `HBCrossCollisions` interface lemmas in
   `proofs/Arrangement/unstable/CrossCollisionsResearch.lean`:
   `CrossBaseOne` and `CrossDimStable` are proven; `CrossRecurrence` remains.
2. Discharge the remaining explicit `sorry`s in the unstable scaffold
   (closed-form arithmetic, binary-reflection half lemmas, the recurrence
   driver) — these predate and are independent of `CrossRecurrence`.
3. Promote the completed `HBCrossCollisions` proof into the stable proof path
   and remove the corresponding hypothesis parameter from
   `arrangement_extraconnectivity_minimum`.
4. `UniversalLowerBound` needs a formalization strategy from scratch (see
   "Current Status" above) — this is not close to done and has no
   in-progress Lean work.

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

### Status: COMPLETED (Arithmetic & Inductive Driver)

We have formally proved the entire arithmetic and inductive backbone for the $C\_constant(R)$ recurrence in `proofs/Arrangement/unstable/CrossCollisionsResearch.lean`:

- Verified the binary reflection arithmetic decompositions of `E_seq` and `sum_bit_length` on the natural numbers, handling all exact subtractions on $\mathbb{N}$ safely without truncation.
- Formally proved the exact recurrence $C(R) = C(2^{d-1}) + C(m) + ext\_cube(d, m) + m$ where $R = 2^{d-1} + m$ and $0 < m \le 2^{d-1}$.
- Implemented a complete strong induction driver `hb_cross_collisions_of_recurrence` using `Nat.strong_induction_on` which derives the final target statement `HBCrossCollisions` for all $R \ge 1$ from three pure combinatorial interface lemmas:
  1. `CrossBaseOne` (single-vertex ball has 0 collisions) — proven
  2. `CrossDimStable` (fresh dimensions don't change the set) — proven
  3. `CrossRecurrence` (the corrected combinatorial split) — remaining

### What remains

Proving `CrossRecurrence` over the definitions of `cross_collisions` and
`hamming_ball_subset` to cleanly plug into the proven induction driver.

## Total Remaining Estimated Effort

**`HBCrossCollisions`**: only `CrossRecurrence` remains to plug into the
already-completed induction driver (Step 4's arithmetic/inductive scaffold is
done; `CrossBaseOne` and `CrossDimStable` are proven). The proof source still
records an open circularity concern between this interface and the driver, so
the remaining effort is unscoped until that concern is resolved.

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
