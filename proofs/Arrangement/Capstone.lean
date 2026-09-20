import Arrangement.CrossTop
import Mathlib.Data.Nat.Lattice

/-!
# Hamming sandwich for the fixed-volume boundary profile

This replaces the former conditional exact-minimum capstone. It concerns the
external vertex boundary of fixed-size subsets, not the extra-connectivity
parameter `κ_g`; no boundary-to-extra-connectivity reduction is asserted.
-/

namespace Arrangement

/-- The value attained by the embedded Boolean Hamming ball. -/
def hamming_profile (R n k : ℕ) : ℕ :=
  (R * k - E_seq R) * (n - k) - C_constant R

/-- Minimum external boundary among `R`-element subsets. -/
noncomputable def boundary_profile (R n k : ℕ) : ℕ :=
  sInf {b : ℕ | ∃ V' : Finset (ArrVertex n k),
    V'.card = R ∧ external_neighbors V' = b}

/-- An additive error depending only on the volume. -/
def sandwich_error (R : ℕ) : ℕ := E_seq R + (R * R - R)

lemma boundary_profile_le {R n k : ℕ} (V' : Finset (ArrVertex n k))
    (hV : V'.card = R) :
    boundary_profile R n k ≤ external_neighbors V' := by
  unfold boundary_profile
  exact Nat.sInf_le ⟨V', hV, rfl⟩

lemma boundary_profile_attained {R n k : ℕ}
    (h : ∃ V' : Finset (ArrVertex n k), V'.card = R) :
    ∃ V' : Finset (ArrVertex n k), V'.card = R ∧
      external_neighbors V' = boundary_profile R n k := by
  obtain ⟨W, hW⟩ := h
  have hne : ({b : ℕ | ∃ V' : Finset (ArrVertex n k),
      V'.card = R ∧ external_neighbors V' = b}).Nonempty :=
    ⟨_, W, hW, rfl⟩
  have hmem := Nat.sInf_mem hne
  simp only [Set.mem_setOf_eq] at hmem
  obtain ⟨V', hV, heq⟩ := hmem
  refine ⟨V', hV, ?_⟩
  unfold boundary_profile
  exact heq

/-- Per-set lower half; it needs only `k ≤ n`, not the cube-embedding gate. -/
theorem hamming_le_boundary_add_error {n k : ℕ}
    (V' : Finset (ArrVertex n k)) (hnk : k ≤ n) :
    hamming_profile V'.card n k ≤
      external_neighbors V' + sandwich_error V'.card := by
  have h := restricted_lower_bound_up_to_error V' hnk
  unfold hamming_profile sandwich_error
  omega

theorem hamming_linear_le_profile_add_error (R n k : ℕ) (hnk : k ≤ n)
    (hR : ∃ V' : Finset (ArrVertex n k), V'.card = R) :
    (R * k - E_seq R) * (n - k) ≤
      boundary_profile R n k + sandwich_error R := by
  obtain ⟨V', hV, heq⟩ := boundary_profile_attained hR
  have h := restricted_lower_bound_up_to_error V' hnk
  rw [hV, heq] at h
  unfold sandwich_error
  omega

/-- Exact Hamming-ball witness on the embedding range. -/
theorem hamming_witness (R n k : ℕ)
    (h_cond : can_embed_hypercube R n k) :
    ∃ V' : Finset (ArrVertex n k), V'.card = R ∧
      external_neighbors V' = hamming_profile R n k := by
  unfold hamming_profile
  by_cases hR : R = 0
  · subst hR
    exact ⟨∅, by simp,
      by simp [external_neighbors, C_constant, E_seq, sum_bit_length]⟩
  · exact exists_optimal_embedding R n k h_cond
      (fun d hk hnk hd => hb_cross_collisions_closed R (by omega) d hd hk hnk)

/-- **Capstone: Hamming sandwich.** On the embedding range, the least
    external boundary is at most the Hamming-ball value and at least that
    value minus an explicit volume-only error. The witness attaining the
    Hamming value is included. This is not an exact isoperimetric theorem. -/
theorem arrangement_boundary_sandwich (R n k : ℕ)
    (h_cond : can_embed_hypercube R n k) :
    boundary_profile R n k ≤ hamming_profile R n k ∧
    hamming_profile R n k ≤ boundary_profile R n k + sandwich_error R ∧
    (∃ V' : Finset (ArrVertex n k), V'.card = R ∧
      external_neighbors V' = hamming_profile R n k) := by
  have hnk : k ≤ n := by
    obtain ⟨hkn, _⟩ := h_cond
    omega
  obtain ⟨W, hW, hWeq⟩ := hamming_witness R n k h_cond
  refine ⟨?_, ?_, ⟨W, hW, hWeq⟩⟩
  · rw [← hWeq]
    exact boundary_profile_le W hW
  · have h := hamming_linear_le_profile_add_error R n k hnk ⟨W, hW⟩
    unfold hamming_profile
    omega

/-- First-order profile estimate: the Hamming linear term differs from the
    profile by at most the explicit error depending only on `R`. -/
theorem boundary_profile_first_order (R n k : ℕ)
    (h_cond : can_embed_hypercube R n k) :
    boundary_profile R n k ≤ (R * k - E_seq R) * (n - k) ∧
    (R * k - E_seq R) * (n - k) ≤
      boundary_profile R n k + sandwich_error R := by
  have hnk : k ≤ n := by
    obtain ⟨h1, _⟩ := h_cond
    omega
  obtain ⟨W, hW, hWeq⟩ := hamming_witness R n k h_cond
  refine ⟨?_, hamming_linear_le_profile_add_error R n k hnk ⟨W, hW⟩⟩
  have hle := boundary_profile_le W hW
  rw [hWeq] at hle
  unfold hamming_profile at hle
  omega

end Arrangement
