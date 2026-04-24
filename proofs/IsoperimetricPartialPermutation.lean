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
-- Section 4: Automorphisms & Boundary Injections
-- ============================================================================

/-- Finset definition of the external boundary, matching external_neighbors. -/
def boundary_set (S : Finset (ArrVertex n k)) : Finset (ArrVertex n k) :=
  Finset.univ.filter (fun v => v ∉ S ∧ ∃ u ∈ S, arr_adjacent u v)

lemma external_neighbors_eq_card_boundary (S : Finset (ArrVertex n k)) :
    external_neighbors S = (boundary_set S).card := rfl

-- ATOMIC GATE 1: Unconditional swap of two symbols (graph automorphism)

/-- Raw symbol swap on Fin n: swap a↔b. -/
def symSwap (a b : Fin n) (x : Fin n) : Fin n :=
  if x = b then a else if x = a then b else x

lemma symSwap_involutive (a b : Fin n) (x : Fin n) :
    symSwap a b (symSwap a b x) = x := by
  simp only [symSwap]; split_ifs <;> simp_all

lemma symSwap_injective (a b : Fin n) : Function.Injective (symSwap a b) := by
  intro x y heq
  have := congr_arg (symSwap a b) heq
  rwa [symSwap_involutive, symSwap_involutive] at this

/-- Global swap of symbols a↔b in a vertex's sequence. -/
def swapVertex (v : ArrVertex n k) (a b : Fin n) : ArrVertex n k :=
  ⟨fun p => symSwap a b (v.val p),
   fun _ _ heq => v.prop (symSwap_injective a b heq)⟩

-- ATOMIC GATE 2: Swap is an involution
lemma swapVertex_involutive (v : ArrVertex n k) (a b : Fin n) :
    swapVertex (swapVertex v a b) a b = v := by
  apply Subtype.ext
  funext p
  simp only [swapVertex]
  exact symSwap_involutive a b (v.val p)

-- ATOMIC GATE 3: Adjacency is preserved under global swap
lemma arr_adjacent_swap (u v : ArrVertex n k) (a b : Fin n) :
    arr_adjacent u v ↔ arr_adjacent (swapVertex u a b) (swapVertex v a b) := by
  -- Prove the filter sets are equal first
  have hfilt : Finset.univ.filter (fun p : Fin k => u.val p ≠ v.val p) =
      Finset.univ.filter (fun p : Fin k => (swapVertex u a b).val p ≠ (swapVertex v a b).val p) := by
    apply Finset.filter_congr
    intro p _
    simp only [swapVertex, ne_eq]
    constructor
    · intro hne heq; exact hne (symSwap_injective a b heq)
    · intro hne heq; exact hne (congr_arg (symSwap a b) heq)
  -- Force the goal into filter form and rewrite
  show (Finset.univ.filter (fun p : Fin k => u.val p ≠ v.val p)).card = 1 ↔
       (Finset.univ.filter (fun p : Fin k => (swapVertex u a b).val p ≠ (swapVertex v a b).val p)).card = 1
  rw [hfilt]

-- ATOMIC GATE 4: shiftVertex relates to swapVertex
-- When uses_sym v b ∧ ¬uses_sym v a, shiftVertex only replaces b→a.
-- swapVertex also replaces a→b, but v doesn't use a, so that never fires.
lemma shiftVertex_eq_swap (v : ArrVertex n k) (a b : Fin n)
    (h : uses_sym v b ∧ ¬uses_sym v a) :
    shiftVertex v a b = swapVertex v a b := by
  apply Subtype.ext
  funext p
  simp only [shiftVertex, swapVertex, symSwap, dif_pos h]
  by_cases h1 : v.val p = b
  · simp [h1]
  · by_cases h2 : v.val p = a
    · exact False.elim (h.2 ⟨p, h2⟩)
    · simp [h1, h2]

-- ATOMIC GATE 5: The Boundary Injection Map
/-- Maps an external neighbor of the compressed set back to an external
    neighbor of the original set.
    Key insight: if u is outside V', it's already a candidate for ∂(V').
    If u is inside V' (but outside compressSet V'), we swap it to find
    the pre-image outside V'. -/
def reverseShiftMap (V' : Finset (ArrVertex n k)) (a b : Fin n)
    (u : ArrVertex n k) : ArrVertex n k :=
  if u ∈ V' then swapVertex u a b else u

-- Key helper: if u ∉ compressSet V' a b but u ∈ V', then swapVertex u a b ∉ V'
lemma swap_of_compressed_mem {V' : Finset (ArrVertex n k)} {a b : Fin n}
    {u : ArrVertex n k} (hu_in_V : u ∈ V') (hu_not_comp : u ∉ compressSet V' a b) :
    swapVertex u a b ∉ V' := by
  intro h_swap_in
  apply hu_not_comp
  -- u ∈ V' and u wasn't shifted (shiftVertex u ∈ V' already via h_swap_in-like arg)
  -- u is in compressSet V' via its own image: the compression map sends u to
  -- either shiftVertex u (if shiftVertex u ∉ V') or u (if shiftVertex u ∈ V')
  unfold compressSet
  rw [Finset.mem_image]
  use u
  refine ⟨hu_in_V, ?_⟩
  -- Need: (if shiftVertex u a b ∉ V' then shiftVertex u a b else u) = u
  -- We need shiftVertex u a b ∈ V' to take the else branch
  change (if shiftVertex u a b ∉ V' then shiftVertex u a b else u) = u
  by_cases hshift : shiftVertex u a b ∉ V'
  · -- shiftVertex u ∉ V': then compressSet maps u to shiftVertex u ≠ u
    -- But then shiftVertex u ∈ compressSet V' (as image of u)
    -- This means u ≠ shiftVertex u, so u might not be in compressSet
    -- Actually we need to check: is u = shiftVertex u a b?
    by_cases heq : shiftVertex u a b = u
    · rw [if_pos hshift, heq]
    · -- shiftVertex u ≠ u and shiftVertex u ∉ V'
      -- u maps to shiftVertex u in compressSet, not to u
      -- So u ∈ compressSet iff some other v maps to u
      exfalso; apply hu_not_comp
      unfold compressSet
      rw [Finset.mem_image]
      use u
      exact ⟨hu_in_V, if_pos hshift⟩
  · rw [if_neg hshift]

-- ATOMIC GATE 5b: The reverse map lands in the old boundary
lemma reverseShiftMap_mem (V' : Finset (ArrVertex n k)) (a b : Fin n)
    (u : ArrVertex n k) (hu : u ∈ boundary_set (compressSet V' a b)) :
    reverseShiftMap V' a b u ∈ boundary_set V' := by
  rw [boundary_set, Finset.mem_filter] at hu ⊢
  rcases hu with ⟨_, hu_not_comp, w, hw_in, hw_adj⟩
  unfold reverseShiftMap
  by_cases hu_V : u ∈ V'
  · -- u ∈ V' but u ∉ compressSet V': map to swapVertex u
    rw [if_pos hu_V]
    refine ⟨Finset.mem_univ _, ?_, ?_⟩
    · exact swap_of_compressed_mem hu_V hu_not_comp
    · -- Need: ∃ neighbor of swapVertex u in V'
      -- u ∈ V', and swapVertex is an involution, so u = swapVertex (swapVertex u)
      -- We need a vertex in V' adjacent to swapVertex u
      -- u itself is in V' and adjacent to swapVertex u iff u ≠ swapVertex u and they differ in ≤1 pos
      -- Actually, we know w ∈ compressSet V' and arr_adjacent w u
      -- We need to trace w back to V'
      -- w ∈ compressSet V' means ∃ v ∈ V', w = compress(v)
      -- We trace back: compress(v) is adjacent to u, need to find neighbor in V'
      unfold compressSet at hw_in
      rw [Finset.mem_image] at hw_in
      rcases hw_in with ⟨v, hv_in, hv_eq⟩
      change (if shiftVertex v a b ∉ V' then shiftVertex v a b else v) = w at hv_eq
      by_cases hshift_v : shiftVertex v a b ∉ V'
      · -- v was shifted: w = shiftVertex v a b
        rw [if_pos hshift_v] at hv_eq
        -- w = shiftVertex v, and arr_adjacent w u
        -- We know u ∈ V' and v ∈ V'
        -- Use v as witness: need arr_adjacent v (swapVertex u a b)
        -- Since w = shiftVertex v and arr_adjacent w u,
        -- and shiftVertex v = swapVertex v (when precondition holds),
        -- we have arr_adjacent (swapVertex v a b) u
        -- By arr_adjacent_swap: arr_adjacent v (swapVertex u a b)
        -- But we need to know shiftVertex = swapVertex for this v
        -- If shiftVertex v ≠ v, then uses_sym v b ∧ ¬uses_sym v a holds
        by_cases hpre : uses_sym v b ∧ ¬uses_sym v a
        · have heq_sw : shiftVertex v a b = swapVertex v a b := shiftVertex_eq_swap v a b hpre
          use v
          refine ⟨hv_in, ?_⟩
          rw [← hv_eq] at hw_adj
          rw [heq_sw] at hw_adj
          exact (arr_adjacent_swap v (swapVertex u a b) a b).mp
            ((arr_adjacent_swap (swapVertex v a b) u a b).mpr hw_adj)
        · -- shiftVertex v = v (precondition failed), so w = v
          have : shiftVertex v a b = v := by
            unfold shiftVertex; rw [dif_neg hpre]
          rw [this] at hv_eq
          use v
          exact ⟨hv_in, hv_eq ▸ hw_adj⟩
      · -- v was NOT shifted: w = v
        rw [if_neg hshift_v] at hv_eq
        use v
        exact ⟨hv_in, hv_eq ▸ hw_adj⟩
  · -- u ∉ V': reverseShiftMap returns u
    rw [if_neg hu_V]
    refine ⟨Finset.mem_univ _, hu_V, ?_⟩
    · -- Need: ∃ v ∈ V', arr_adjacent v u
      -- We know: w ∈ compressSet V' and arr_adjacent w u
      unfold compressSet at hw_in
      rw [Finset.mem_image] at hw_in
      rcases hw_in with ⟨v, hv_in, hv_eq⟩
      change (if shiftVertex v a b ∉ V' then shiftVertex v a b else v) = w at hv_eq
      by_cases hshift_v : shiftVertex v a b ∉ V'
      · -- v was shifted: w = shiftVertex v a b, arr_adjacent w u
        rw [if_pos hshift_v] at hv_eq
        -- Need neighbor of u in V'. v ∈ V'.
        -- w = shiftVertex v, arr_adjacent w u
        -- If shiftVertex v = swapVertex v, then by arr_adjacent_swap,
        -- arr_adjacent v (swapVertex u a b). But we need adj to u, not swap(u).
        -- However since u ∉ V', reverseShiftMap gives u.
        -- We need ∃ v' ∈ V', arr_adjacent v' u.
        -- v ∈ V' is the candidate. But arr_adjacent v u ≠ arr_adjacent (shiftVertex v) u in general.
        -- Actually, we can just use v as the witness if w = v (unshifted case)
        -- For shifted case, need more work. Let's use swapVertex w a b:
        -- swapVertex (shiftVertex v) = swapVertex (swapVertex v) = v (when shift = swap)
        -- So if arr_adjacent (shiftVertex v) u, and shiftVertex v = swapVertex v,
        -- then by arr_adjacent_swap: arr_adjacent v (swapVertex u a b)
        -- But we need arr_adjacent v u, not arr_adjacent v (swapVertex u a b)!
        -- This approach doesn't directly work. The map might need to be different.
        -- For now, use sorry — this requires the full adjacency trace
        sorry
      · -- v was NOT shifted: w = v, so v ∈ V' and arr_adjacent v u
        rw [if_neg hshift_v] at hv_eq
        use v
        exact ⟨hv_in, hv_eq ▸ hw_adj⟩

-- ATOMIC GATE 6: Injectivity of the reverse map on the boundary
lemma reverseShiftMap_injOn (V' : Finset (ArrVertex n k)) (a b : Fin n) :
    Set.InjOn (reverseShiftMap V' a b) (boundary_set (compressSet V' a b)) := by
  intro u1 hu1 u2 hu2 heq
  simp only [reverseShiftMap] at heq
  rw [boundary_set, Finset.mem_filter] at hu1 hu2
  by_cases h1 : u1 ∈ V' <;> by_cases h2 : u2 ∈ V'
  · -- Both in V': swapVertex u1 = swapVertex u2 → u1 = u2
    simp [h1, h2] at heq
    have := congr_arg (swapVertex · a b) heq
    simp [swapVertex_involutive] at this
    exact this
  · -- u1 ∈ V', u2 ∉ V': swapVertex u1 = u2
    simp [h1, h2] at heq
    -- swapVertex u1 = u2, so u2 = swapVertex u1 ∈ image of swap
    -- u1 ∈ V', u2 ∉ V', u2 ∉ compressSet V'
    -- But swapVertex u1 = u2, so u1 = swapVertex u2
    -- u2 ∉ compressSet V' a b (from hu2)
    -- u1 ∈ V' and u1 ∉ compressSet V' (from hu1)
    -- swap_of_compressed_mem gives swapVertex u1 ∉ V'
    -- But swapVertex u1 = u2 ∉ V' ✓, and u2 ∉ compressSet V' ✓
    -- The contradiction: u2 = swapVertex u1, and u2 ∉ compressSet V'
    -- But u1 ∉ compressSet V' either. This is not a contradiction per se.
    -- Actually: reverseShiftMap u1 = swapVertex u1 = u2 = reverseShiftMap u2
    -- So the images are equal. We need u1 = u2.
    -- u1 ∈ V', u2 ∉ V', so u1 ≠ u2. Contradiction with injectivity? No, we're proving injectivity.
    -- This means the case can't happen: reverseShiftMap u1 ∈ boundary(V') requires
    -- swapVertex u1 ∉ V' (from swap_of_compressed_mem). But heq says swapVertex u1 = u2 ∉ V'. ✓
    -- And reverseShiftMap u2 = u2 ∉ V'. So both map to the same thing.
    -- For injectivity we must show u1 = u2, but u1 ∈ V' and u2 ∉ V', impossible.
    -- So this case actually CAN'T produce equal images unless the boundary conditions prevent it.
    -- Wait: reverseShiftMap u1 = swapVertex u1 a b (since u1 ∈ V')
    -- reverseShiftMap u2 = u2 (since u2 ∉ V')
    -- Equal means swapVertex u1 a b = u2
    -- For u1 to be in boundary(compressSet), u1 ∉ compressSet V' (since it's in boundary_set)
    -- swap_of_compressed_mem: swapVertex u1 ∉ V'
    -- For u2 to be in boundary(compressSet), u2 ∉ compressSet V'
    -- swapVertex u1 = u2 and swapVertex u1 ∉ V', u2 ∉ V'. Both ∉ V'. Consistent.
    -- But we also need: swapVertex u1 a b ∉ compressSet V' (since u2 = swapVertex u1 ∉ compressSet)
    -- This should give a contradiction somehow...
    -- Actually, u1 ∈ V' ∧ u1 ∉ compressSet V' means something was shifted AWAY from u1.
    -- That means shiftVertex u1 ≠ u1 and shiftVertex u1 ∉ V'.
    -- So compressSet maps u1 → shiftVertex u1 (not to u1). And no other v maps to u1.
    -- But swapVertex u1 = u2 ∉ compressSet, meaning u2 is not the image of any v.
    -- This is consistent — both u1 and u2 are outside compressSet.
    -- To get contradiction: reverseShiftMap u1 = swapVertex u1 = u2 = reverseShiftMap u2
    -- but reverseShiftMap_mem places both in boundary(V'). Both map to the same point
    -- in boundary(V'). This violates injectivity only if u1 ≠ u2, which is our goal to PROVE.
    -- Since u1 ∈ V' and u2 ∉ V', we have u1 ≠ u2. But that doesn't help — we need to
    -- show this case is impossible (the images can't be equal).
    -- Hmm, actually this case IS possible and the map IS injective here:
    -- swapVertex u1 = u2 means u1 = swapVertex u2 (by involution)
    -- u1 ∈ V' means swapVertex u2 ∈ V', so reverseShiftMap u2 = swapVertex u2 = u1 (if u2 ∈ V')
    -- But we said u2 ∉ V'! So reverseShiftMap u2 = u2, not swapVertex u2.
    -- And reverseShiftMap u1 = swapVertex u1 = u2.
    -- So images: swapVertex u1 = u2 and u2. Both = u2. So images ARE equal.
    -- This means u1 = u2 for injectivity, but u1 ∈ V' and u2 ∉ V' → contradiction.
    -- So this case is genuinely impossible! We can derive False.
    -- Actually wait: reverseShiftMap u2 = u2 (since u2 ∉ V')
    -- reverseShiftMap u1 = swapVertex u1 (since u1 ∈ V')
    -- heq says these are equal: swapVertex u1 = u2
    -- But for u2 to be in boundary_set(compressSet V'), u2 must have a neighbor in compressSet V'.
    -- For u1 to be in boundary_set(compressSet V'), u1 must have a neighbor in compressSet V'.
    -- There's no direct contradiction from the V' membership alone.
    -- The question is: can swapVertex u1 = u2 when u1 ∈ V', u2 ∉ V', both ∉ compressSet V'?
    -- Yes it can! So we need to show u1 = u2 from heq, which is impossible.
    -- This means the reverseShiftMap definition might be wrong.
    -- Let me reconsider...
    sorry
  · -- u1 ∉ V', u2 ∈ V': symmetric case
    simp [h1, h2] at heq
    sorry
  · -- Both not in V': u1 = u2
    simp [h1, h2] at heq
    exact heq

/-- The core extremal lemma: compression does not increase boundary size. -/
lemma compressSet_boundary_le (V' : Finset (ArrVertex n k))
    (a b : Fin n) (_hab : a < b) :
    external_neighbors (compressSet V' a b) ≤ external_neighbors V' := by
  rw [external_neighbors_eq_card_boundary, external_neighbors_eq_card_boundary]
  apply Finset.card_le_card_of_injOn (reverseShiftMap V' a b)
  · intro u hu
    rw [Finset.mem_coe] at hu ⊢
    exact reverseShiftMap_mem V' a b u hu
  · exact reverseShiftMap_injOn V' a b

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
