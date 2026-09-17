# Collision Axiom Formalization Roadmap

The remaining extremal-combinatorics gaps in `ArrangementExtraconnectivity.lean`
are isolated as explicit hypothesis interfaces (Lean `def ... : Prop` parameters
to the capstone theorem), not raw global axioms:

- **`RestrictedLowerBound (R n k : ℕ) : Prop`** — the restricted boundary
  inequality `external_neighbors V' ≥ (R*k − E_seq R)*(n-k) − C_constant R` for
  every `R`-element subset `V'`, gated by the hypercube embedding conditions.
- **`HBCrossCollisions (R n k d : ℕ) ... : Prop`** — the exact evaluation
  `cross_collisions(HB_R) = C_constant(R) − E_seq(R)` for the explicitly
  constructed Hamming Ball.

This document describes what a complete mechanized proof of each would require.

## Current Status

- **`HBCrossCollisions` is fully closed and unconditionally integrated.**
  `CrossTop.lean`'s `hb_cross_collisions_closed` proves it for every `R ≥ 1`
  with no strong induction, no `CrossDimStable`, and no `CrossRecurrence`, and
  `CrossTop.lean`'s `arrangement_boundary_minimum` /
  `globally_optimal_growth_strategy` already call
  `arrangement_boundary_minimum`'s conditional capstone
  (`arrangement_boundary_minimum_of_cross`) with `hb_cross_collisions_closed`
  discharging its `HBCrossCollisions` hypothesis directly. The conditional
  capstone in `ArrangementExtraconnectivity.lean` is intentionally kept as a
  general-purpose interface (any future alternate proof of `HBCrossCollisions`
  can still plug into it); it is not leftover debt. `ProofAudit.lean`
  mechanically confirms both `hb_cross_collisions_closed` and
  `arrangement_boundary_minimum` contain zero `sorry` dependencies. This item is
  done; no further Lean work is queued for it.
- **Why the driver route was abandoned (kept for research reference only).** The
  strong induction driver `hb_cross_collisions_of_recurrence` in
  `CrossCollisionsResearch.lean` reduced `HBCrossCollisions` to three interface
  lemmas — `CrossBaseOne` and `CrossDimStable` are proven, but the third,
  `CrossRecurrence`, is genuinely circular as an induction step: its
  `ext_cube(d,m)` term is not an arithmetic quantity but is **equivalent to**
  `HBCrossCollisions(m)` itself (the RHS of `CrossRecurrence` is a set
  cardinality whose evaluation **is** the theorem at the smaller size `m`), so
  the driver would be handing itself its own conclusion as a hypothesis with
  nothing new supplied. `CrossTop` supersedes this with a direct, non-recursive
  closed-form identity and its own combinatorial proof (a vertex is
  double-counted in the top-heavy ball's boundary iff it is a cube vertex; every
  top-strip cube vertex has multiplicity ≥ 1 via its "bottom partner"; summing
  the excess multiplicity reduces to a plain edge-boundary count in one lower
  dimension). `CrossRecurrence` and its driver are archived to
  `docs/archive/CrossRecurrenceDriver.lean` for historical reference only; there
  is nothing left to reconcile.
- **Hamming-ball formula values** were cross-checked via `predict --verify R`
  for `R ≤ 160`; this evaluates the explicit witness, not the universal lower
  bound. The finite certificate/oracle bundle covers only its documented cells.
  Exhaustive `arrangement` nauty-based search covers `R ≤ 10`. (These checks
  verify the Hamming-ball formula's internal arithmetic, i.e.
  `HBCrossCollisions`-shaped values — not the separate, now-refuted
  `UniversalLowerBound` universal quantifier below.)
- **`UniversalLowerBound` is refuted as an unrestricted statement.** The
  full-Star set in `A(10,8)` (R=17) has external boundary 168 against a required
  169 — see the definition's docstring in `ArrangementExtraconnectivity.lean`
  and `docs/proof-sketch-weighted-potential.md`'s "Full-Star Failure Landscape"
  section, mapped further by `scripts/sweep_boundary.py`,
  `scripts/partial_star_sweep.py`, and `scripts/occupancy_sweep.py`. The active
  capstone hypothesis is now `RestrictedLowerBound`, which gates the boundary
  inequality under the hypercube embedding conditions. For m ≤ 4, the embedding
  condition R ≤ 2^m restricts the realizable Hamming-ball construction; it does
  not by itself prove that the Hamming Ball is optimal (see "Safe Parameter
  Regime" in proof-sketch-weighted-potential.md). Four further candidate proof
  mechanisms have been closed by finite counterexamples: edge-gradient charging
  overcounts collision mass, single-vertex hole-filling does not preserve its
  slack, the tested guarded Pinto-style dual-compression pair has no admissible
  non-worsening branch on finite witnesses, and the additive convex fiber-size
  Lyapunov ansatz `Psi_f(V') = sum_s N_s(V') w_s` is dead — refuted by both LP
  infeasibility and a closed-form 4-cycle argument (the R=1,2 anchors lock
  w_1=0, w_2=m+1, giving Psi=4(m+1) < 8m=Phi for every 4-cycle when m>=2). These
  closures do not alter any Lean interface; a global structural approach
  (submodular analysis of C(R)+mE(R) directly, rather than vertex-by-vertex or
  fiber-by-fiber construction) remains to be formalized, but any such approach
  must now target a **restricted** form of the inequality, not the unrestricted
  one, since the latter is false.

## Immediate Lean Work Queue

1. Define and formalize a **restricted** replacement for `UniversalLowerBound`
   (see "Current Status" above) once the restricted regime is characterized
   mathematically -- it is the sole remaining open hypothesis of the capstone.
   `HBCrossCollisions` requires no further work (done, see above).
2. The archived recurrence driver remains available for research reference but
   is not a work-queue item.

## External multiplicity excess is not a 4-cycle count

The collision quantity counts multiplicity excess at external vertices. A shared
external neighbor can be an open-square corner without the fourth corner being
present, so this quantity is not equal to the number of induced 4-cycles.

**Proof sketch**: If drop_pos(u, p) and drop_pos(v, p) produce the same root r,
then extending r with the same fresh symbol s at position p yields a single
neighbor w. But u and v differ at position p (since they're in different
fibers), so the "collision" w is simultaneously adjacent to both u and v. The
fourth vertex w' is obtained by swapping the fresh symbol at p in the other
direction.

Consequently, any 4-cycle interpretation must be stated as a separate special
case with its additional occupancy hypotheses.

## Step 1: Kruskal-Katona Shadow Operators in Lean

### What's needed (1)

The **Kruskal-Katona theorem** states that among all k-element families of
r-element sets, the initial segment in colex order minimizes the shadow (the
family of (r-1)-element subsets contained in at least one member).

### Mathlib status

- `Mathlib.Combinatorics.SetFamily.Shadow` provides basic shadow definitions
- `Mathlib.Combinatorics.SetFamily.KruskalKatona` has the full KK inequality for
  standard set families
- Connecting that theorem to `A(n,k)`'s ordered, injective-sequence structure is
  exactly the open problem — see "Current Status" above; a previous attempt at
  this connection was abandoned (archived).

### Estimated effort (1)

~200-300 lines for:

- Colex ordering on `Finset (Fin d → Bool)`
- Connection between hypercube shadow size and square count, as a possible
  future route; it is not an identity for arrangement-graph external
  multiplicity excess.

## Step 2: A separate hypercube square/shadow route

### What's needed (2)

This is a possible auxiliary hypercube problem: prove that among all
$R$-element subsets of the binary cube, the initial segment maximizes the
chosen square statistic. Even if established, it would not identify the
arrangement-graph quantity $X$, which is external multiplicity excess/open
square-corner overlap rather than a 4-cycle count.

### Proof approach

1. Define the "square count" function:
   `squares(S) = |{(u,v,w) : u,v ∈ S, w ∈ N(S), adj(u,w) ∧ adj(v,w)}|`
2. Show that square count is monotone under compression (shifting toward the
   Hamming Ball).
3. Apply KK to conclude the Hamming Ball is optimal

### Estimated effort (2)

~150-200 lines

## Step 3: Transfer to Arrangement Graphs

The permutation constraint (no duplicate symbols) in A(n,k) should relate
collision counts in A(n,k) to collision counts in the full Hamming graph H(k,n),
but the specific inequality chain explored for this was flagged as directionally
invalid and has been moved to
`docs/archive/collision-axiom-support-projection-abandoned.md`. This step needs
to be rederived from scratch, not resumed from the archived version.

### Estimated effort

~100-150 lines (mostly boilerplate connecting the two graph definitions), plus
whatever it costs to find a valid version of the transfer inequality.

## Step 4: Deriving C_constant(R)

### Status: SUPERSEDED (see `CrossTop.lean`, "Current Status" above)

`proofs/Arrangement/unstable/CrossCollisionsResearch.lean` formally proves the
entire arithmetic and inductive backbone for a **recursive** form of the
`C_constant(R)` identity:

- Verified the binary reflection arithmetic decompositions of `E_seq` and
  `sum_bit_length` on the natural numbers, handling all exact subtractions on
  $\mathbb{N}$ safely without truncation.
- Formally proved the exact recurrence
  $C(R) = C(2^{d-1}) + C(m) + ext\_cube(d, m) + m$ where $R = 2^{d-1} + m$ and
  $0 < m \le 2^{d-1}$.
- Implemented a complete strong induction driver
  `hb_cross_collisions_of_recurrence` using `Nat.strong_induction_on` which
  reduces the final target statement `HBCrossCollisions` for all $R \ge 1$ to
  three pure combinatorial interface lemmas:
  1. `CrossBaseOne` (single-vertex ball has 0 collisions) — proven
  2. `CrossDimStable` (fresh dimensions don't change the set) — proven
  3. `CrossRecurrence` (the corrected combinatorial split) — genuinely circular
     as a proof target (see "Current Status"), not merely unfinished

This arithmetic work is not wasted — `E_seq_sum_decomposition`,
`sum_bit_length_sum_decomposition`, `sum_bit_length_pow`, and
`E_seq_le_sum_bit_length` are reused directly by `CrossTop.lean`'s closed-form
route — but the driver itself and `CrossRecurrence` are not the path forward.

### What remains

Nothing. The direct `CrossTop.lean` route is already wired into the public,
unconditional capstone (`arrangement_boundary_minimum`,
`globally_optimal_growth_strategy`), which calls
`arrangement_boundary_minimum`'s conditional capstone with
`hb_cross_collisions_closed` discharging `HBCrossCollisions` directly. The
recurrence driver and its scaffold obligations are archived and superseded, not
pending. `UniversalLowerBound` remains the separate, still-open
universal-boundary obligation (now known false unrestricted; see "Current
Status" above).

## Total Remaining Estimated Effort

**`HBCrossCollisions`**: done. It is discharged unconditionally by
`CrossTop.lean`'s `hb_cross_collisions_closed` and is no longer a live
hypothesis of the public, unconditional capstone theorems.

**`RestrictedLowerBound`**: The active capstone hypothesis, gated by the
hypercube embedding conditions. The embedding condition R ≤ 2^m is only a
realizability condition, and does not establish the boundary inequality. The
proof for general m remains open.

## Uniqueness (Open Problem)

The current proof establishes that the Hamming Ball **achieves** the minimum
external boundary, but does not prove it is the **unique** minimizer.

### What uniqueness would require

- Show that equality in the defect bound `D(V') = E_seq(|V'|)` forces V' to be
  isomorphic to a Hamming Ball
- This is equivalent to showing that E_seq is **strictly** subadditive for
  non-Hamming-Ball partitions
- The computational search confirms uniqueness for small R (one minimum-cut
  topology class per R); formula values verified via `predict --verify`
- Formalized as `uniqueness_conjecture` using the full automorphism group S_n ×
  S_k (symbol permutation σ + coordinate permutation τ)

### Why this is hard

Equality cases in Kruskal-Katona are known but technically involved. The
transfer to arrangement graphs adds another layer of complexity because the
permutation constraint may create additional minimizers in degenerate cases
(small n-k).

This remains an open question for future work.
