/-
  IsoperimetricPartialPermutation.lean
  ====================================
  Isoperimetric Inequality for Partial Permutations (A(n,k))

  This module provides the sequence compression machinery needed to
  eventually remove the two axioms in ArrangementExtraconnectivity.lean:
    1. external_neighbors_collision_bound
    2. hamming_ball_eval

  STATUS: SCAFFOLD — core definitions with proof obligations as sorry.
  See docs/collision-axiom-roadmap.md for the full formalization roadmap.

  ARCHITECTURE:
  - Section 1: Sequence Shift (local, single-vertex operation)
  - Section 2: Set-Wise Compression (global, conditional on V')
  - Section 3: Compression preserves injectivity and cardinality
  - Section 4: Compression does not increase external boundary
  - Section 5: Colex ordering for ArrVertex and convergence
  - Section 6: The Isoperimetric Theorem (replaces both axioms)
-/

import ArrDefs
import Mathlib.Data.Finset.Image

set_option autoImplicit false

variable {n k : ℕ}

-- ============================================================================
-- Section 1: The Local Shift Operator
-- ============================================================================

/-- Check if a symbol is present anywhere in the sequence. -/
def uses_sym (v : ArrVertex n k) (s : Fin n) : Prop :=
  ∃ p : Fin k, v.val p = s

instance {n k : ℕ} (v : ArrVertex n k) (s : Fin n) : Decidable (uses_sym v s) :=
  Fintype.decidableExistsFintype

/-- The local shift: if vertex uses symbol b but not symbol a,
    swap b → a in the sequence. Otherwise leave unchanged.

    Uses a functional map (not Classical.choose) for tactic friendliness:
    `fun p => if v.val p = b then a else v.val p` -/
def shiftVertex (v : ArrVertex n k) (a b : Fin n) : ArrVertex n k :=
  if h : uses_sym v b ∧ ¬uses_sym v a then
    let new_val := fun p => if v.val p = b then a else v.val p
    have new_inj : Function.Injective new_val := by
      intro p1 p2 heq
      dsimp [new_val] at heq
      split_ifs at heq with h1 h2
      · -- Both were b. v.val p1 = b = v.val p2, so p1 = p2 by v.prop.
        exact v.prop (h1.trans h2.symm)
      · -- p1 was b, p2 was not. Then a = v.val p2, contradicting h.2.
        exact False.elim (h.2 ⟨p2, heq.symm⟩)
      · -- p1 was not b, p2 was. Then v.val p1 = a, contradicting h.2.
        exact False.elim (h.2 ⟨p1, heq⟩)
      · -- Neither was b. v.val p1 = v.val p2, so p1 = p2 by v.prop.
        exact v.prop heq
    ⟨new_val, new_inj⟩
  else
    v

-- ============================================================================
-- Section 2: The Global Set Compression Operator
-- ============================================================================

/-- Compress a subset V' by shifting symbol b → a across all vertices.
    Critical collision guard: a vertex is shifted only if its target
    is not already occupied in V'. Uses Finset.image with inline check,
    mirroring Mathlib's uv.compress pattern. -/
def compressSet (V' : Finset (ArrVertex n k)) (a b : Fin n) :
    Finset (ArrVertex n k) :=
  V'.image (fun v =>
    let v_shifted := shiftVertex v a b
    if v_shifted ∉ V' then v_shifted else v)

-- ============================================================================
-- Section 3: Compression Invariants
-- ============================================================================

/-- Size preservation: the conditional shift is a bijection on V'.
    PROOF BLUEPRINT (3 cases for f(v1) = f(v2)):
    Case 1 (neither shifted): f(v1)=v1, f(v2)=v2, trivially v1=v2.
    Case 2 (both shifted): apply inverse shift (a→b) to both sides;
      deterministic pre-images must be identical.
    Case 3 (mixed): if v1 shifted and v2 didn't, then shiftVertex(v1)=v2
      means v2 ∈ V', but the collision guard would have aborted v1's
      shift. Contradiction.
    Close with Finset.card_image_of_injOn. -/
lemma compressSet_card (V' : Finset (ArrVertex n k)) (a b : Fin n) :
    (compressSet V' a b).card = V'.card := by
  unfold compressSet
  apply Finset.card_image_of_injOn
  intro v1 hv1 v2 hv2 heq
  rw [Finset.mem_coe] at hv1 hv2
  simp only at heq
  split_ifs at heq with h1 h2 h3

  · -- Neither shifted: heq : v1 = v2
    exact heq

  · -- v2 shifted, v1 didn't: heq : v1 = shiftVertex v2 a b
    subst heq; exact absurd hv1 (by assumption)

  · -- v1 shifted, v2 didn't: heq : shiftVertex v1 a b = v2
    subst heq; exact absurd hv2 (by assumption)

  · -- Both shifted: heq : shiftVertex v1 a b = shiftVertex v2 a b
    -- Extract ∉ facts from context by type, not by fragile name
    have h_not1 : shiftVertex v1 a b ∉ V' := by assumption
    have h_not2 : shiftVertex v2 a b ∉ V' := by assumption

    have h_shift_neq1 : shiftVertex v1 a b ≠ v1 := fun eq =>
      h_not1 (by rw [eq]; exact hv1)
    have h_cond1 : uses_sym v1 b ∧ ¬uses_sym v1 a := by
      by_contra hc
      exact h_shift_neq1 (by unfold shiftVertex; rw [dif_neg hc])

    have h_shift_neq2 : shiftVertex v2 a b ≠ v2 := fun eq =>
      h_not2 (by rw [eq]; exact hv2)
    have h_cond2 : uses_sym v2 b ∧ ¬uses_sym v2 a := by
      by_contra hc
      exact h_shift_neq2 (by unfold shiftVertex; rw [dif_neg hc])

    have h_val_eq : (shiftVertex v1 a b).val = (shiftVertex v2 a b).val :=
      congr_arg Subtype.val heq
    unfold shiftVertex at h_val_eq
    rw [dif_pos h_cond1, dif_pos h_cond2] at h_val_eq
    dsimp only at h_val_eq

    apply Subtype.ext; funext p
    have hp := congr_fun h_val_eq p
    by_cases h_p1 : v1.val p = b
    · rw [if_pos h_p1] at hp
      by_cases h_p2 : v2.val p = b
      · rw [h_p1, h_p2]
      · rw [if_neg h_p2] at hp
        exact False.elim (h_cond2.2 ⟨p, hp.symm⟩)
    · rw [if_neg h_p1] at hp
      by_cases h_p2 : v2.val p = b
      · rw [if_pos h_p2] at hp
        exact False.elim (h_cond1.2 ⟨p, hp⟩)
      · rw [if_neg h_p2] at hp
        exact hp

-- Helper 1: Shifting twice is identical to shifting once.
-- The first shift replaces all b's with a's. On the second pass,
-- `uses_sym (shiftVertex v a b) b` is False (no b's remain), so
-- the dif_neg branch fires and the vertex is unchanged.
lemma shiftVertex_idem (v : ArrVertex n k) (a b : Fin n) :
    shiftVertex (shiftVertex v a b) a b = shiftVertex v a b := by
  -- Helper: shiftVertex is identity when precondition fails
  have shift_id : ∀ w : ArrVertex n k, ¬(uses_sym w b ∧ ¬uses_sym w a) →
      shiftVertex w a b = w := by
    intro w hw; unfold shiftVertex; rw [dif_neg hw]
  by_cases h1 : uses_sym v b ∧ ¬uses_sym v a
  · -- v was shifted: shiftVertex v a b has b→a, so no b's remain
    have eq1 : shiftVertex v a b =
        ⟨fun p => if v.val p = b then a else v.val p, by
          intro p1 p2 heq; simp only at heq
          split_ifs at heq with h1a h1b
          · exact v.prop (h1a.trans h1b.symm)
          · exact False.elim (h1.2 ⟨p2, heq.symm⟩)
          · exact False.elim (h1.2 ⟨p1, heq⟩)
          · exact v.prop heq⟩ := by
      unfold shiftVertex; rw [dif_pos h1]
    -- The shifted vertex has no b's, so precondition fails on second shift
    have h2 : ¬(uses_sym (shiftVertex v a b) b ∧ ¬uses_sym (shiftVertex v a b) a) := by
      intro ⟨⟨p, hp⟩, _⟩
      rw [eq1] at hp
      dsimp only at hp
      by_cases h3 : v.val p = b
      · rw [if_pos h3] at hp
        -- hp : a = b, h3 : v.val p = b → v.val p = a
        exact h1.2 ⟨p, h3.trans hp.symm⟩
      · rw [if_neg h3] at hp
        exact h3 hp
    exact shift_id _ h2
  · -- v was not shifted: shiftVertex v a b = v, so second shift = first
    have eq1 : shiftVertex v a b = v := by
      unfold shiftVertex; rw [dif_neg h1]
    rw [eq1]; exact eq1

-- Helper 2: The compressed set is closed under shiftVertex.
-- For any w ∈ compressSet V' a b, shiftVertex w a b ∈ compressSet V' a b.
-- This is the key invariant that makes the if-guard always take else-branch
-- on the second compression pass.
lemma shift_mem_compress {V' : Finset (ArrVertex n k)} {a b : Fin n}
    {w : ArrVertex n k} (hw : w ∈ compressSet V' a b) :
    shiftVertex w a b ∈ compressSet V' a b := by
  unfold compressSet at hw ⊢
  rw [Finset.mem_image] at hw ⊢
  rcases hw with ⟨v, hv, heq⟩
  change (if shiftVertex v a b ∉ V' then shiftVertex v a b else v) = w at heq
  by_cases hc : shiftVertex v a b ∉ V'
  · -- v was shifted: w = shiftVertex v a b
    rw [if_pos hc] at heq
    use v
    refine ⟨hv, ?_⟩
    change (if shiftVertex v a b ∉ V' then shiftVertex v a b else v) = shiftVertex w a b
    rw [if_pos hc, ← heq, shiftVertex_idem]
  · -- v was NOT shifted: w = v, and shiftVertex v a b ∈ V'
    rw [if_neg hc] at heq
    have h_in : shiftVertex v a b ∈ V' := by
      by_contra h_not; exact hc h_not
    use shiftVertex v a b
    refine ⟨h_in, ?_⟩
    change (if shiftVertex (shiftVertex v a b) a b ∉ V'
        then shiftVertex (shiftVertex v a b) a b else shiftVertex v a b) = shiftVertex w a b
    rw [shiftVertex_idem, if_neg hc, ← heq]

/-- Idempotence: compressing with the same symbols twice is a no-op.
    Since the compressed set is closed under shiftVertex (shift_mem_compress),
    the if-guard `shiftVertex v a b ∉ compressSet V' a b` is always False,
    so every vertex maps to itself. -/
lemma compressSet_idempotent (V' : Finset (ArrVertex n k)) (a b : Fin n) :
    compressSet (compressSet V' a b) a b = compressSet V' a b := by
  apply Finset.ext
  intro w
  constructor
  · -- Forward: w ∈ compress(compress(V')) → w ∈ compress(V')
    intro hw
    unfold compressSet at hw
    rw [Finset.mem_image] at hw
    rcases hw with ⟨v, hv, heq⟩
    change (if shiftVertex v a b ∉ compressSet V' a b
        then shiftVertex v a b else v) = w at heq
    have h_mem : shiftVertex v a b ∈ compressSet V' a b := shift_mem_compress hv
    have hc : ¬(shiftVertex v a b ∉ compressSet V' a b) := not_not.mpr h_mem
    rw [if_neg hc] at heq
    rw [← heq]; exact hv
  · -- Backward: w ∈ compress(V') → w ∈ compress(compress(V'))
    intro hw
    unfold compressSet
    rw [Finset.mem_image]
    use w
    refine ⟨hw, ?_⟩
    change (if shiftVertex w a b ∉ compressSet V' a b
        then shiftVertex w a b else w) = w
    have h_mem : shiftVertex w a b ∈ compressSet V' a b := shift_mem_compress hw
    have hc : ¬(shiftVertex w a b ∉ compressSet V' a b) := not_not.mpr h_mem
    rw [if_neg hc]


-- ============================================================================
-- Section 4: The Failure of Geometric Compression (Counterexample)
-- ============================================================================

/-!
  # The Geometric Compression Failure

  In standard bipartite hypercubes, Harper's Theorem is proven by showing
  that sequence compression does not increase boundary size:
      |∂(compressSet V')| ≤ |∂V'|

  **This inequality is MATHEMATICALLY FALSE for Arrangement Graphs.**

  ## Background
  In the Arrangement Graph A(n,k), each vertex is an injective sequence
  of k symbols drawn from {1, ..., n} (a partial permutation). Two vertices
  are adjacent iff they differ at exactly one position. The external boundary
  ∂S of a vertex set S is the set of vertices NOT in S that are adjacent to
  at least one vertex in S.

  The compression operator `compressSet V' a b` tries to shift symbol b → a
  in each vertex of V', subject to injectivity: if a vertex already uses
  symbol a, it cannot shift (it would create a duplicate symbol). In hypercubes
  this always preserves or shrinks the boundary. The counterexample below
  shows this fails for partial permutations.

  ## The Counterexample
  Consider A(4,2) with symbols {1, 2, 3, 4}. Let a=1, b=3.
  Let V' = { [4,3], [1,3] }.

  1. The Boundary of V':
     [4,3] and [1,3] share position 2 = symbol 3.
     - Neighbors of [4,3]: [1,3]*, [2,3], [4,1], [4,2]   (* internal)
     - Neighbors of [1,3]: [4,3]*, [2,3], [1,2], [1,4]   (* internal)
     - External boundary ∂V' = {[2,3], [4,1], [4,2], [1,2], [1,4]}, size 5.

  2. The Compression (shift 3 → 1):
     - [4,3]: uses 3 ✓, lacks 1 ✓ → shifts to [4,1]. Since [4,1] ∉ V', OK.
     - [1,3]: uses 3 ✓, but ALREADY uses 1 ✗ → cannot shift (would create
       [1,1] which violates injectivity). Vertex unchanged.
     - compressSet(V') = { [4,1], [1,3] }.
     Note: [4,3] and [1,3] were adjacent (shared symbol 3 at position 2).
     After compression, [4,1] and [1,3] differ at BOTH positions — they are
     no longer adjacent! The internal edge was destroyed.

  3. The Boundary of compressSet(V'):
     - Neighbors of [4,1]: [2,1], [3,1], [4,2], [4,3]
     - Neighbors of [1,3]: [4,3], [2,3], [1,2], [1,4]
     - Only [4,3] is shared. All others are distinct.
     - External boundary = {[2,1],[3,1],[4,2],[4,3],[2,3],[1,2],[1,4]}, size 7.

  ** 7 ≤ 5 is FALSE. The external boundary INCREASED under compression. **

  The new boundary vertices [2,1] and [3,1] appeared because [4,1] "stole"
  symbol 1, creating adjacencies to vertices containing symbol 1 that had
  no connection to the original V'.
  See `docs/compression_counterexample.gif` for a visual.

  ## Root Cause: Coordinate Tangling
  In hypercubes, flipping a bit is orthogonal to all other bits. But in A(n,k),
  the permutation constraint means changing one symbol affects the entire pool
  of available symbols. When [4,3] shifts to [4,1], it "steals" symbol 1 from
  the symbol pool, creating new adjacencies (to [2,1], [3,1]) that never
  existed relative to the original V'.

  ## Conclusion
  Vertex isoperimetry via geometric shift operators is impossible for
  partial permutations. The graph space contains local minima; one cannot
  monotonically slide arbitrary sets into the Hamming Ball.

  This mathematically mandates the "Algebraic Defect Squeeze" architecture
  in `ArrangementExtraconnectivity.lean`, which tracks global algebraic roots
  (via `E_seq` subadditivity) rather than local boundary mappings.
-/
