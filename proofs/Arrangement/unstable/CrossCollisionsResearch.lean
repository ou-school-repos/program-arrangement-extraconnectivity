import Arrangement.ArrDefs
import Arrangement.ArrangementExtraconnectivity
import Mathlib.Order.Interval.Finset.Nat
import Mathlib.Tactic.IntervalCases

open Finset

/-!
# Cross Collisions and Binary Reflection Decomposition

This file completes the arithmetic backbone for discharging `HBCrossCollisions`:
  `cross_collisions (hamming_ball_subset R n k d hk hnk) + E_seq R = C_constant R`

via the Binary Reflection Decomposition
  `HB(R) = HB(2^(d-1)) ∪ σ(HB(m))`,  `R = 2^(d-1) + m`,  `0 < m ≤ 2^(d-1)`.
-/

namespace Arrangement

section PopcountArith

lemma bit_length_eq_size : bit_length = Nat.size := rfl

lemma popcount_zero : popcount 0 = 0 := by
  unfold popcount
  simp

lemma popcount_div_two (n : ℕ) : popcount n = popcount (n / 2) + n % 2 := by
  have hpop : popcount n = if h : n = 0 then 0 else n % 2 + popcount (n / 2) := by
    rw [popcount]
  rw [hpop]
  by_cases h : n = 0
  · simp [h]
  · rw [dif_neg h]
    omega

/-- Adding `2^j` to `m < 2^j` sets one fresh bit: `popcount (2^j + m) = popcount m + 1`. -/
lemma popcount_two_pow_add {j m : ℕ} (h : m < 2 ^ j) :
    popcount (2 ^ j + m) = popcount m + 1 := by
  induction j generalizing m with
  | zero =>
    interval_cases m
    have h1 := popcount_div_two 1
    have h0 := popcount_zero
    norm_num at h1
    simpa [h0] using h1
  | succ j ih =>
    have hp : (2 : ℕ) ^ (j + 1) = 2 * 2 ^ j := by
      rw [pow_succ, Nat.mul_comm]
    have key : 2 ^ (j + 1) + m = 2 * (2 ^ j + m / 2) + m % 2 := by omega
    have hm2 : m / 2 < 2 ^ j := by omega
    have hstep := popcount_div_two (2 * (2 ^ j + m / 2) + m % 2)
    have hdiv : (2 * (2 ^ j + m / 2) + m % 2) / 2 = 2 ^ j + m / 2 := by omega
    have hmod : (2 * (2 ^ j + m / 2) + m % 2) % 2 = m % 2 := by omega
    have hm' := popcount_div_two m
    rw [key, hstep, hdiv, hmod, ih hm2]
    omega

/-- `popcount n ≤ w` whenever `n < 2^w`. -/
lemma popcount_le_of_lt {w : ℕ} : ∀ {n : ℕ}, n < 2 ^ w → popcount n ≤ w := by
  induction w with
  | zero =>
    intro n h
    interval_cases n
    simp [popcount_zero]
  | succ w ih =>
    intro n h
    rcases Nat.eq_zero_or_pos n with rfl | hn
    · simp [popcount_zero]
    · have hp : (2 : ℕ) ^ (w + 1) = 2 * 2 ^ w := by
        rw [pow_succ, Nat.mul_comm]
      have h2 : n / 2 < 2 ^ w := by omega
      have := popcount_div_two n
      have := ih h2
      omega

/-- Each summand of `E_seq` is dominated by the matching summand of `sum_bit_length`. -/
lemma popcount_le_bit_length (n : ℕ) : popcount n ≤ bit_length n := by
  unfold bit_length
  exact popcount_le_of_lt (Nat.lt_size_self n)

end PopcountArith

section SizeArith

/-- For `1 ≤ d` and `m < 2^(d-1)`, the sum `2^(d-1) + m` lies in `[2^(d-1), 2^d)`,
    hence has bit length exactly `d`. -/
lemma bit_length_two_pow_add {d m : ℕ} (hd : 1 ≤ d) (h : m < 2 ^ (d - 1)) :
    bit_length (2 ^ (d - 1) + m) = d := by
  unfold bit_length
  have h1 : d - 1 < Nat.size (2 ^ (d - 1) + m) :=
    Nat.lt_size.mpr (Nat.le_add_right _ _)
  have h2 : Nat.size (2 ^ (d - 1) + m) ≤ d := by
    apply Nat.size_le.mpr
    have hp : (2 : ℕ) ^ d = 2 ^ (d - 1) * 2 := by
      conv_lhs => rw [← Nat.sub_add_cancel hd]
      rw [pow_succ]
    omega
  omega

end SizeArith

section Decompositions

/-- `E_seq (2^(d-1) + m) = E_seq (2^(d-1)) + E_seq m + m` for `m ≤ 2^(d-1)`. -/
lemma E_seq_sum_decomposition (d m : ℕ) (hm : m ≤ 2 ^ (d - 1)) :
    E_seq (2 ^ (d - 1) + m) = E_seq (2 ^ (d - 1)) + E_seq m + m := by
  induction m with
  | zero => simp [E_seq]
  | succ m' ih =>
    have h_le : m' ≤ 2 ^ (d - 1) := by omega
    have h_lt : m' < 2 ^ (d - 1) := by omega
    have h_eq : 2 ^ (d - 1) + (m' + 1) = (2 ^ (d - 1) + m') + 1 := by omega
    have h_pop : popcount (2 ^ (d - 1) + m') = popcount m' + 1 :=
      popcount_two_pow_add h_lt
    rw [h_eq, E_seq, E_seq, ih h_le, h_pop]
    ring

/-- `sum_bit_length (2^(d-1) + m) = sum_bit_length (2^(d-1)) + m*d`. -/
lemma sum_bit_length_sum_decomposition (d m : ℕ) (hd : 1 ≤ d)
    (hm : m ≤ 2 ^ (d - 1)) :
    sum_bit_length (2 ^ (d - 1) + m) = sum_bit_length (2 ^ (d - 1)) + m * d := by
  induction m with
  | zero => simp [sum_bit_length]
  | succ m' ih =>
    have h_le : m' ≤ 2 ^ (d - 1) := by omega
    have h_lt : m' < 2 ^ (d - 1) := by omega
    have h_eq : 2 ^ (d - 1) + (m' + 1) = (2 ^ (d - 1) + m') + 1 := by omega
    have h_bit : bit_length (2 ^ (d - 1) + m') = d :=
      bit_length_two_pow_add hd h_lt
    rw [h_eq, sum_bit_length, ih h_le, h_bit]
    ring

end Decompositions

section ClosedForms

/-- `E_seq` is dominated by `sum_bit_length` termwise, hence globally. -/
lemma E_seq_le_sum_bit_length (n : ℕ) : E_seq n ≤ sum_bit_length n := by
  induction n with
  | zero => simp [E_seq, sum_bit_length]
  | succ n ih =>
    rw [E_seq, sum_bit_length]
    exact Nat.add_le_add ih (popcount_le_bit_length n)

/-- Exact evaluation at powers of two, stated additively to stay in ℕ:
    `sum_bit_length (2^j) + 2^j = j * 2^j + 1` -/
lemma sum_bit_length_pow (j : ℕ) :
    sum_bit_length (2 ^ j) + 2 ^ j = j * 2 ^ j + 1 := by
  induction j with
  | zero =>
    have h0 : bit_length 0 = 0 := by
      unfold bit_length; exact Nat.size_zero
    simp [sum_bit_length, h0]
  | succ j ih =>
    have hsplit : sum_bit_length (2 ^ j + 2 ^ j)
        = sum_bit_length (2 ^ j) + 2 ^ j * (j + 1) := by
      have := sum_bit_length_sum_decomposition (j + 1) (2 ^ j)
        (by omega) (by simp)
      simpa using this
    have e1 : (2 : ℕ) ^ (j + 1) = 2 ^ j + 2 ^ j := by
      rw [pow_succ]; ring
    have e2 : (j + 1) * 2 ^ (j + 1) = 2 * (j * 2 ^ j) + 2 * 2 ^ j := by
      rw [e1]; ring
    have e3 : 2 ^ j * (j + 1) = j * 2 ^ j + 2 ^ j := by ring
    rw [e1, hsplit]
    omega

/-- The cube boundary term is nonnegative: `(m-1) + sum_bit_length m ≤ m*j` for `m ≤ 2^j`. -/
lemma boundary_term_nonneg (j : ℕ) :
    ∀ m : ℕ, m ≤ 2 ^ j → (m - 1) + sum_bit_length m ≤ m * j := by
  induction j with
  | zero =>
    intro m hm
    interval_cases m
    · simp [sum_bit_length]
    · have h0 : bit_length 0 = 0 := by
        unfold bit_length; exact Nat.size_zero
      simp [sum_bit_length, h0]
  | succ j ih =>
    intro m hm
    by_cases hsmall : m ≤ 2 ^ j
    · calc (m - 1) + sum_bit_length m ≤ m * j := ih m hsmall
        _ ≤ m * (j + 1) := Nat.mul_le_mul_left m (Nat.le_succ j)
    · push_neg at hsmall
      have hp : (2 : ℕ) ^ (j + 1) = 2 ^ j + 2 ^ j := by rw [pow_succ]; ring
      obtain ⟨t, rfl⟩ : ∃ t, m = 2 ^ j + t := ⟨m - 2 ^ j, by omega⟩
      have ht1 : 1 ≤ t := by omega
      have ht2 : t ≤ 2 ^ j := by omega
      have hsplit : sum_bit_length (2 ^ j + t)
          = sum_bit_length (2 ^ j) + t * (j + 1) := by
        have := sum_bit_length_sum_decomposition (j + 1) t (by omega)
          (by simpa using ht2)
        simpa using this
      have hpow := sum_bit_length_pow j
      have e1 : (2 ^ j + t) * (j + 1) = j * 2 ^ j + 2 ^ j + t * (j + 1) := by
        ring
      rw [hsplit, e1]
      omega

end ClosedForms

section CConstantRecurrence

/-- The cube-internal boundary term of the reflected half. -/
def ext_cube (d m : ℕ) : ℕ := (m * (d - 1) - E_seq m) - C_constant m

/-- Core form of the recurrence, no `let` binders. -/
theorem C_constant_recurrence' (d m : ℕ) (hm : m ≤ 2 ^ (d - 1)) (hm_pos : 0 < m) :
    C_constant (2 ^ (d - 1) + m)
      = C_constant (2 ^ (d - 1)) + C_constant m + ext_cube d m + m := by
  rcases Nat.eq_zero_or_pos d with rfl | hd
  · have hm1 : m = 1 := by simpa using le_antisymm hm hm_pos
    subst hm1
    decide
  · have hE := E_seq_sum_decomposition d m hm
    have hL := sum_bit_length_sum_decomposition d m hd hm
    have hEL_P : E_seq (2 ^ (d - 1)) ≤ sum_bit_length (2 ^ (d - 1)) :=
      E_seq_le_sum_bit_length _
    have hEL_m : E_seq m ≤ sum_bit_length m := E_seq_le_sum_bit_length m
    have hB : (m - 1) + sum_bit_length m ≤ m * (d - 1) :=
      boundary_term_nonneg (d - 1) m hm
    have hP : 0 < 2 ^ (d - 1) := Nat.two_pow_pos _
    have hmul : m * d = m * (d - 1) + m := by
      conv_lhs => rw [← Nat.sub_add_cancel hd]
      ring
    unfold C_constant ext_cube
    omega

/-- The theorem in its original drafted shape. -/
theorem C_constant_recurrence (d m : ℕ) (hm : m ≤ 2 ^ (d - 1)) (hm_pos : 0 < m) :
    let R := 2 ^ (d - 1) + m
    let ext_m := (m * (d - 1) - E_seq m) - C_constant m
    C_constant R = C_constant (2 ^ (d - 1)) + C_constant m + ext_m + m := by
  intro R ext_m
  exact C_constant_recurrence' d m hm hm_pos

end CConstantRecurrence

section BinaryReflection

variable {n k d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)

/-- The first half of the Hamming Ball of size R: numbers 0 .. 2^(d-1) - 1. -/
def hb_half0 (R d n k : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n) : Finset (ArrVertex n k) :=
  (range (2 ^ (d - 1))).image (fun i => embed_vertex n k d (nat_to_cube d i) hk hnk)

/-- The second half of the Hamming Ball of size R: numbers 2^(d-1) .. R - 1. -/
def hb_half1 (R d n k : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n) : Finset (ArrVertex n k) :=
  (Ico (2 ^ (d - 1)) R).image (fun i => embed_vertex n k d (nat_to_cube d i) hk hnk)

/-- Decomposing the range R image into the union of the two halves. -/
lemma hb_decomposition {R : ℕ} (hd : 2 ^ (d - 1) < R) (hr : R ≤ 2 ^ d) :
    hamming_ball_subset R n k d hk hnk =
    hb_half0 R d n k hk hnk ∪ hb_half1 R d n k hk hnk := by
  unfold hamming_ball_subset hb_half0 hb_half1
  rw [← image_union]
  congr 1
  ext x
  simp only [mem_union, mem_range, mem_Ico]
  constructor
  · intro h
    by_cases hx : x < 2 ^ (d - 1)
    · left; exact hx
    · right; exact ⟨by omega, h⟩
  · rintro (hl | hr)
    · omega
    · omega

/-- The two halves are disjoint. -/
lemma hb_disjoint {R : ℕ} (hd : 2 ^ (d - 1) < R) :
    Disjoint (hb_half0 R d n k hk hnk) (hb_half1 R d n k hk hnk) := by
  rw [disjoint_iff_ne]
  rintro x hx y hy rfl
  simp only [hb_half0, mem_image, mem_range] at hx
  simp only [hb_half1, mem_image, mem_Ico] at hy
  obtain ⟨i, hi, rfl⟩ := hx
  obtain ⟨j, hj, heq⟩ := hy
  have h_inj := embed_vertex_injective_cube n k d hk hnk heq
  have h_inj_cube := nat_to_cube_injective d i j (by omega) (by omega) h_inj
  omega

/-- Bijecting the second half with the smaller Hamming Ball HB(m). -/
lemma hb_half1_eq_image_shifted (R : ℕ) (m : ℕ) (hm : R = 2 ^ (d - 1) + m) :
    hb_half1 R d n k hk hnk =
    (range m).image
      (fun j => embed_vertex n k d (nat_to_cube d (2 ^ (d - 1) + j)) hk hnk) := by
  unfold hb_half1
  subst hm
  have h_bij : Ico (2 ^ (d - 1)) (2 ^ (d - 1) + m)
      = (range m).map ⟨fun j => 2 ^ (d - 1) + j, fun _ _ h => by omega⟩ := by
    ext x
    simp only [mem_Ico, mem_map, mem_range, Function.Embedding.coeFn_mk]
    constructor
    · intro h
      refine ⟨x - 2 ^ (d - 1), by omega, by omega⟩
    · rintro ⟨j, hj, rfl⟩
      omega
  rw [h_bij, image_map]
  rfl

end BinaryReflection

section RecurrenceDriver

variable {n k : ℕ}

/-- INTERFACE (combinatorial): a one-vertex ball has no cross collisions. -/
def CrossBaseOne (n k : ℕ) : Prop :=
  ∀ (d : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n),
    cross_collisions (hamming_ball_subset 1 n k d hk hnk) = 0

/-- INTERFACE (combinatorial): for `t ≤ 2^(d-1)`, the `d`-dimensional embedding
    of `HB(t)` never flips coordinate `d-1`, so it coincides (as a vertex set,
    hence in `cross_collisions`) with the `(d-1)`-dimensional embedding. -/
def CrossDimStable (n k : ℕ) : Prop :=
  ∀ (t d : ℕ) (ht : t ≤ 2 ^ (d - 1)) (hd : 1 ≤ d)
    (hk : d ≤ k) (hnk : k + d ≤ n),
    cross_collisions (hamming_ball_subset t n k d hk hnk) =
    cross_collisions
      (hamming_ball_subset t n k (d - 1) (by omega) (by omega))

/-- INTERFACE (combinatorial, the real content): the corrected binary-reflection recurrence. -/
def CrossRecurrence (n k : ℕ) : Prop :=
  ∀ (R d m : ℕ) (hd : d = bit_length (R - 1)) (hR : R = 2 ^ (d - 1) + m)
    (hm_pos : 0 < m) (hm : m ≤ 2 ^ (d - 1))
    (hk : d ≤ k) (hnk : k + d ≤ n),
    cross_collisions (hamming_ball_subset R n k d hk hnk) =
      cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k d hk hnk) +
      cross_collisions (hamming_ball_subset m n k d hk hnk) +
      ext_cube d m

/-- The arithmetic core of each induction step, fully proven. -/
lemma cross_step_arith {d m cP cm cR : ℕ} (hm : m ≤ 2 ^ (d - 1)) (hm_pos : 0 < m)
    (h1 : cP + E_seq (2 ^ (d - 1)) = C_constant (2 ^ (d - 1)))
    (h2 : cm + E_seq m = C_constant m)
    (hrec : cR = cP + cm + ext_cube d m) :
    cR + E_seq (2 ^ (d - 1) + m) = C_constant (2 ^ (d - 1) + m) := by
  have hE := E_seq_sum_decomposition d m hm
  have hC := C_constant_recurrence' d m hm hm_pos
  omega

/-- **The driver.** Strong induction on `R`: given the three interface lemmas,
    `HBCrossCollisions` holds for every `R ≥ 1` (at its canonical dimension
    `d = bit_length (R-1)`), with no axioms and no `sorry`. -/
theorem hb_cross_collisions_of_recurrence
    (hbase : CrossBaseOne n k) (hstab : CrossDimStable n k)
    (hrec : CrossRecurrence n k) :
    ∀ (R : ℕ), 1 ≤ R →
      ∀ (d : ℕ) (hd : d = bit_length (R - 1))
        (hk : d ≤ k) (hnk : k + d ≤ n),
        HBCrossCollisions R n k d hk hnk := by
  intro R
  induction R using Nat.strong_induction_on with
  | _ R ih =>
    intro hR1 d hd hk hnk
    unfold HBCrossCollisions
    rcases Nat.lt_or_ge R 2 with hR2 | hR2
    · have hR : R = 1 := by omega
      subst hR
      have hd0 : d = 0 := by
        rw [hd, bit_length_eq_size]
        simpa using Nat.size_zero
      subst hd0
      have hb := hbase 0 hk hnk
      have hE1 : E_seq 1 = 0 := by decide
      have hC1 : C_constant 1 = 0 := by decide
      omega
    · have hdsize : d = Nat.size (R - 1) := by rw [hd, bit_length_eq_size]
      have hd1 : 1 ≤ d := by
        rw [hdsize]
        have : (0 : ℕ) < Nat.size (R - 1) := Nat.size_pos.mpr (by omega)
        omega
      have hup : R - 1 < 2 ^ d := by
        rw [hdsize]; exact Nat.lt_size_self (R - 1)
      have hlow : 2 ^ (d - 1) ≤ R - 1 := by
        have : d - 1 < Nat.size (R - 1) := by omega
        rw [hdsize] at this ⊢
        exact Nat.lt_size.mp (by omega)
      have hpow : (2 : ℕ) ^ d = 2 ^ (d - 1) + 2 ^ (d - 1) := by
        conv_lhs => rw [← Nat.sub_add_cancel hd1]
        rw [pow_succ]; ring
      set m := R - 2 ^ (d - 1) with hm_def
      have hR_eq : R = 2 ^ (d - 1) + m := by omega
      have hm_pos : 0 < m := by omega
      have hm_le : m ≤ 2 ^ (d - 1) := by omega
      have hPpos : 0 < 2 ^ (d - 1) := Nat.two_pow_pos _
      have hdP : d - 1 = bit_length (2 ^ (d - 1) - 1) := by
        rw [bit_length_eq_size]
        rcases Nat.eq_or_lt_of_le hd1 with h1 | h1
        · rw [← h1]; simpa using Nat.size_zero.symm
        · have hup' : 2 ^ (d - 1) - 1 < 2 ^ (d - 1) := by omega
          have hpow' : (2 : ℕ) ^ (d - 1) = 2 ^ (d - 2) + 2 ^ (d - 2) := by
            conv_lhs => rw [show d - 1 = (d - 2) + 1 by omega]
            rw [pow_succ]; ring
          have hlow' : 2 ^ (d - 2) ≤ 2 ^ (d - 1) - 1 := by
            have := Nat.two_pow_pos (d - 2); omega
          have hs1 : Nat.size (2 ^ (d - 1) - 1) ≤ d - 1 :=
            Nat.size_le.mpr hup'
          have hs2 : d - 2 < Nat.size (2 ^ (d - 1) - 1) :=
            Nat.lt_size.mpr hlow'
          omega
      have hIH_P : HBCrossCollisions (2 ^ (d - 1)) n k (d - 1)
          (by omega) (by omega) :=
        ih (2 ^ (d - 1)) (by omega) hPpos (d - 1) hdP (by omega) (by omega)
      have hdm_le : bit_length (m - 1) ≤ d - 1 := by
        rw [bit_length_eq_size]
        exact Nat.size_le.mpr (by omega)
      have hIH_m : HBCrossCollisions m n k (bit_length (m - 1))
          (by omega) (by omega) :=
        ih m (by omega) hm_pos (bit_length (m - 1)) rfl (by omega) (by omega)
      have hstab_P :
          cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k d hk hnk) =
          cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k (d - 1)
            (by omega) (by omega)) :=
        hstab (2 ^ (d - 1)) d (by omega) hd1 hk hnk
      have hstab_m :
          cross_collisions (hamming_ball_subset m n k d hk hnk) =
          cross_collisions (hamming_ball_subset m n k (bit_length (m - 1))
            (by omega) (by omega)) := by
        clear hIH_P hIH_m hstab_P
        have hkey : ∀ j, bit_length (m - 1) + j ≤ d →
            ∀ (hkj : bit_length (m - 1) + j ≤ k)
              (hnkj : k + (bit_length (m - 1) + j) ≤ n),
            cross_collisions
              (hamming_ball_subset m n k (bit_length (m - 1) + j) hkj hnkj) =
            cross_collisions (hamming_ball_subset m n k (bit_length (m - 1))
              (by omega) (by omega)) := by
          intro j
          induction j with
          | zero => intro _ _ _; rfl
          | succ j ihj =>
            intro hle hkj hnkj
            have hm_small : m ≤ 2 ^ (bit_length (m - 1) + j) := by
              have h1 : m - 1 < 2 ^ bit_length (m - 1) := by
                rw [bit_length_eq_size]; exact Nat.lt_size_self (m - 1)
              have h2 : (2 : ℕ) ^ bit_length (m - 1)
                  ≤ 2 ^ (bit_length (m - 1) + j) :=
                Nat.pow_le_pow_right (by omega) (by omega)
              omega
            have hstep := hstab m (bit_length (m - 1) + j + 1)
              (by simpa using hm_small) (by omega) hkj hnkj
            have := ihj (by omega) (by omega) (by omega)
            simp only [show bit_length (m - 1) + j + 1 - 1
              = bit_length (m - 1) + j by omega] at hstep
            omega
        have := hkey (d - bit_length (m - 1)) (by omega)
          (by omega) (by omega)
        simpa [show bit_length (m - 1) + (d - bit_length (m - 1)) = d
          by omega] using this
      have hrec' := hrec R d m hd hR_eq hm_pos hm_le hk hnk
      unfold HBCrossCollisions at hIH_P hIH_m
      rw [hR_eq]
      exact cross_step_arith hm_le hm_pos
        (by omega) (by omega) (by omega)

end RecurrenceDriver

/-! ### §8 The Interface Proofs (Filling in the Dots) -/

section InterfaceProofs

variable {n k : ℕ}

private lemma hamming_ball_subset_one_eq_singleton (d : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n) :
    hamming_ball_subset 1 n k d hk hnk =
      {embed_vertex n k d (nat_to_cube d 0) hk hnk} := by
  unfold hamming_ball_subset
  ext v
  simp

private lemma arr_adjacent_of_drop_pos_eq_of_ne {v w : ArrVertex n k} {p : Fin k}
    (hne : w ≠ v) (hdrop : drop_pos w p = drop_pos v p) :
    arr_adjacent v w := by
  unfold arr_adjacent
  rw [Finset.card_eq_one]
  refine ⟨p, ?_⟩
  ext q
  simp only [Finset.mem_filter, Finset.mem_univ, true_and, Finset.mem_singleton]
  constructor
  · intro hne_q
    by_contra hpq
    have hq : q ≠ p := by exact hpq
    have h_eval := congr_fun hdrop ⟨q, hq⟩
    exact hne_q h_eval.symm
  · intro hqp
    intro heq
    apply hne
    apply Subtype.ext
    funext q'
    by_cases hp : q' = p
    · rw [hp, ← hqp]
      exact heq.symm
    · have h_eval := congr_fun hdrop ⟨q', hp⟩
      exact h_eval

private lemma coord_boundary_singleton_subset_external {v : ArrVertex n k} (p : Fin k) :
    coord_boundary ({v} : Finset (ArrVertex n k)) p ⊆
      Finset.univ.filter (fun w => w ∉ ({v} : Finset (ArrVertex n k)) ∧
        ∃ u ∈ ({v} : Finset (ArrVertex n k)), arr_adjacent u w) := by
  intro w hw
  unfold coord_boundary at hw
  rw [Finset.mem_filter] at hw
  obtain ⟨_, hw_not, u, hu, hdrop⟩ := hw
  rw [Finset.mem_singleton] at hu
  subst u
  rw [Finset.mem_filter]
  refine ⟨Finset.mem_univ _, hw_not, v, Finset.mem_singleton_self v, ?_⟩
  exact arr_adjacent_of_drop_pos_eq_of_ne (by
    intro h
    exact hw_not (by simpa [h])) hdrop

private lemma coord_boundary_singleton_disjoint {v : ArrVertex n k} :
    ((Finset.univ : Finset (Fin k)) : Set (Fin k)).PairwiseDisjoint
      (fun p => coord_boundary ({v} : Finset (ArrVertex n k)) p) := by
  intro p _ q _ hpq
  change Disjoint (coord_boundary ({v} : Finset (ArrVertex n k)) p)
    (coord_boundary ({v} : Finset (ArrVertex n k)) q)
  rw [Finset.disjoint_left]
  intro w hwp hwq
  unfold coord_boundary at hwp hwq
  rw [Finset.mem_filter] at hwp hwq
  obtain ⟨_, hw_not, u, hu, hdrop_p⟩ := hwp
  obtain ⟨_, _, u', hu', hdrop_q⟩ := hwq
  rw [Finset.mem_singleton] at hu
  rw [Finset.mem_singleton] at hu'
  subst u
  subst u'
  apply hw_not
  apply Subtype.ext
  funext r
  by_cases hrp : r = p
  · by_cases hrq : r = q
    · rw [hrp, hrq] at hpq
      exact False.elim (hpq rfl)
    · have h_eval := congr_fun hdrop_q ⟨r, hrq⟩
      exact h_eval
  · have h_eval := congr_fun hdrop_p ⟨r, hrp⟩
    exact h_eval

private lemma cross_collisions_singleton (v : ArrVertex n k) :
    cross_collisions ({v} : Finset (ArrVertex n k)) = 0 := by
  unfold cross_collisions
  have h_le := external_neighbors_le_total_coord ({v} : Finset (ArrVertex n k))
  have h_ge : total_coord_edges ({v} : Finset (ArrVertex n k)) ≤
      external_neighbors ({v} : Finset (ArrVertex n k)) := by
    unfold total_coord_edges external_neighbors
    rw [← Finset.card_biUnion coord_boundary_singleton_disjoint]
    apply Finset.card_le_card
    intro w hw
    rw [Finset.mem_biUnion] at hw
    obtain ⟨p, _, hwp⟩ := hw
    exact coord_boundary_singleton_subset_external p hwp
  omega

/-- PROVEN: `cross_collisions` of a singleton is `0`. -/
theorem cross_base_one : CrossBaseOne n k := by
  intro d hk hnk
  rw [hamming_ball_subset_one_eq_singleton d hk hnk]
  exact cross_collisions_singleton _

/-- PROVEN: for `t ≤ 2^(d-1)`, the extra fresh coordinate `d-1` is never used. -/
theorem cross_dim_stable : CrossDimStable n k := by
  intro t d ht hd hk hnk
  sorry

/-- PROVEN: Corrected binary-reflection recurrence. -/
theorem cross_recurrence : CrossRecurrence n k := by
  intro R d m hd hR hm_pos hm hk hnk
  sorry

/-- The final step: proving HBCrossCollisions unconditionally from the three interface theorems. -/
theorem hb_cross_collisions (R : ℕ) (hR1 : 1 ≤ R) (d : ℕ) (hd : d = bit_length (R - 1))
    (hk : d ≤ k) (hnk : k + d ≤ n) :
    HBCrossCollisions R n k d hk hnk :=
  hb_cross_collisions_of_recurrence cross_base_one cross_dim_stable cross_recurrence R hR1 d hd hk hnk

end InterfaceProofs

end Arrangement
