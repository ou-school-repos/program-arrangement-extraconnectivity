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
  simp only at heq
  split_ifs at heq with h1 h2 h3 h4

  · -- Case 1: Neither shifted. h1 : ∈ V', h2 : ∈ V', heq : v1 = v2
    exact heq

  · -- Case 2: v2 shifted, v1 didn't. h1 : ∈ V', heq : v1 = shiftVertex v2 a b
    subst heq
    exact absurd (Finset.mem_coe.mp hv1) h2

  · -- Case 3: v1 shifted, v2 didn't. heq : shiftVertex v1 a b = v2
    subst heq
    exact absurd (Finset.mem_coe.mp hv2) h3

  · -- Case 4: Both shifted. heq : shiftVertex v1 a b = shiftVertex v2 a b
    -- Extract that the shift precondition was satisfied
    have h_shift_neq1 : shiftVertex v1 a b ≠ v1 := fun eq =>
      h3 (eq ▸ (Finset.mem_coe.mp hv1))
    have h_cond1 : uses_sym v1 b ∧ ¬uses_sym v1 a := by
      by_contra hc
      exact h_shift_neq1 (by unfold shiftVertex; rw [dif_neg hc])

    have h_shift_neq2 : shiftVertex v2 a b ≠ v2 := fun eq =>
      h4 (eq ▸ (Finset.mem_coe.mp hv2))
    have h_cond2 : uses_sym v2 b ∧ ¬uses_sym v2 a := by
      by_contra hc
      exact h_shift_neq2 (by unfold shiftVertex; rw [dif_neg hc])

    -- Extract the underlying functions and evaluate
    have h_val_eq : (shiftVertex v1 a b).val = (shiftVertex v2 a b).val :=
      congr_arg Subtype.val heq
    unfold shiftVertex at h_val_eq
    rw [dif_pos h_cond1, dif_pos h_cond2] at h_val_eq
    dsimp only at h_val_eq

    apply Subtype.ext
    funext p
    have hp := congr_fun h_val_eq p

    -- Evaluate the swap at each position
    by_cases h_p1 : v1.val p = b
    · rw [if_pos h_p1] at hp
      by_cases h_p2 : v2.val p = b
      · rw [h_p1, h_p2]
      · rw [if_neg h_p2] at hp
        -- Contradiction: v2 would contain 'a' originally
        exact False.elim (h_cond2.2 ⟨p, hp.symm⟩)
    · rw [if_neg h_p1] at hp
      by_cases h_p2 : v2.val p = b
      · rw [if_pos h_p2] at hp
        -- Contradiction: v1 would contain 'a' originally
        exact False.elim (h_cond1.2 ⟨p, hp⟩)
      · rw [if_neg h_p2] at hp
        exact hp

/-- Idempotence: compressing with the same symbols twice is a no-op.
    PROOF BLUEPRINT (case split on how v' entered the compressed set):
    Case 1 (v' was shifted in): v' now contains a, lacks b.
      shiftVertex precondition (uses_sym v b) is False. Unchanged.
    Case 2a (v' didn't shift, lacked b or had a): precondition still
      fails on second pass. Unchanged.
    Case 2b (v' wanted to shift but was blocked): the blocker is still
      present (cardinality preserved by Lemma 1), so the global
      guard (destination ∉ V') still fires. Unchanged. -/
lemma compressSet_idempotent (V' : Finset (ArrVertex n k)) (a b : Fin n) :
    compressSet (compressSet V' a b) a b = compressSet V' a b := by
  sorry

-- ============================================================================
-- Section 4: The Extremal Squeeze
-- ============================================================================

/-- The core extremal lemma (~200-500 lines when complete):
    shifting vertices toward smaller symbols can only increase root
    collisions, which reduces external neighbors.
    PROOF BLUEPRINT (boundary injection argument):
    1. The shift operator is a graph automorphism (symbol relabeling),
       so boundary of shifted set = shifted boundary of original set.
    2. Every external neighbor u of compressSet(V') is adjacent to some
       v_compressed ∈ compressSet(V').
    3. Trace pre-image: un-compress v_compressed to v_orig ∈ V', and
       reverse-shift u to u_orig. Then u_orig is adjacent to v_orig.
    4. u_orig ∉ V' (if it were, u would be in the compressed set,
       contradicting u being external). So u_orig ∈ ∂(V').
    5. This gives an injection ∂(compressSet(V')) → ∂(V'), hence
       |∂(compressSet(V'))| ≤ |∂(V')|. -/
lemma compressSet_boundary_le (V' : Finset (ArrVertex n k))
    (a b : Fin n) (_hab : a < b) :
    external_neighbors (compressSet V' a b) ≤ external_neighbors V' := by
  sorry

-- ============================================================================
-- Section 5: Colex Ordering and Convergence
-- ============================================================================

/-!
  Define a colexicographic ordering on ArrVertex that enumerates
  the Hamming Ball construction step-by-step.

  Prove that repeated compression over all pairs a < b converges
  to the colex initial segment (the Hamming Ball).

  The iteration: construct a function that applies compressSet for
  all pairs (a, b) with a < b until the set is completely stable.
  Once stable, prove it equals hamming_ball_subset.
-/

-- ============================================================================
-- Section 6: The Isoperimetric Theorem
-- ============================================================================

/-!
  Composing Sections 3-5 replaces both axioms:

  theorem isoperimetric_partial_perm (R n k : ℕ) :
    ∀ (V' : Finset (ArrVertex n k)), V'.card = R →
      external_neighbors V' ≥ external_neighbors (hamming_ball R n k) :=

  This directly implies:
  - external_neighbors_collision_bound (the universal lower bound)
  - hamming_ball_eval (the Hamming Ball achieves the bound)
-/
