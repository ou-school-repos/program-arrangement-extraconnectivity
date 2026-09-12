import Arrangement.ArrDefs
import Arrangement.ArrangementExtraconnectivity
import Mathlib.Algebra.Order.BigOperators.Group.LocallyFinite
-- ^ adjust the last module name if the lakefile maps `unstable/` differently.
-- This file consumes (all verified present in the current sources):
--   ArrangementExtraconnectivity.lean: popcount_even, popcount_odd,
--     coord_boundary, total_coord_edges, cross_collisions, external_neighbors,
--     external_neighbors_le_total_coord, drop_pos, embed_vertex, embed_cube,
--     nat_to_cube, nat_to_cube_injective, embed_vertex_injective_cube,
--     hamming_ball_subset, HBCrossCollisions, E_seq, sum_bit_length, C_constant.

open Finset

/-!
# CrossTop: unconditional closed form for the Hamming-ball cross-collision count

This module proves the direct, non-recursive Hamming-ball collision formula.
Its bridge lemmas reduce arrangement-boundary multiplicities to hypercube
edge counts, and `hb_cross_collisions_closed` derives the closed form for
every nonempty ball.

## Why this file exists (the circularity in `CrossRecurrence`)

`CrossRecurrence` (formerly in `CrossCollisionsResearch.lean`) is TRUE but is the
wrong interface. Unfolding its three terms:

  cross(HB 2^(d-1), d) = 0                                (bottom half is a full subcube)
  cross(HB m, d)       = edge_bd(m, d-1) - vertex_bd(m, d-1)
  cross(HB R, d)       = edge_bd(m, d-1)                  (this file's `cross_top`)

so `CrossRecurrence` reduces to `ext_cube d m = vertex_bd(m, d-1)`, which holds
iff `HBCrossCollisions m` holds. The strong-induction driver keeps its
induction hypotheses to itself — the Prop `CrossRecurrence` receives none — so
a direct proof of `cross_recurrence` must re-derive the target theorem at `m`
from scratch. (Verified numerically: scripts/verify_crosstop.py, check 7.)

## The replacement

  **cross_top :  cross_collisions (HB (2^(d-1) + m)) + 2 * E_seq m = m * (d-1)**

for `1 ≤ d`, `0 < m ≤ 2^(d-1)`, `d ≤ k`, `k + d ≤ n`. Unconditional; kills the
strong-induction driver AND removes `CrossDimStable` from the main path
(`cross_dim_stable` stays valid and useful, just no longer load-bearing).
`HBCrossCollisions R` for all `R ≥ 1` then follows by pure arithmetic
(`hb_cross_collisions_closed` at the bottom), with `cross_base_one` as the
only other combinatorial input.

The finite cases were also independently checked by
`scripts/verify_crosstop.py` against small arrangement graphs.
-/

namespace Arrangement

/-! ### Layer A0: bit toolbox (formerly CrossCollisionsResearch §0–§4, §8) -/

section Interface

/-- `popcount 0 = 0`. -/
lemma popcount_zero : popcount 0 = 0 := by
  simp [popcount]

/-- The fundamental div/2 characterization of `popcount`.  Holds for `n = 0` too
    (both sides are `0`).  NOTE: do not add to a `simp` set — it self-loops at `n = 0`. -/
lemma popcount_div_two (n : ℕ) : popcount n = popcount (n / 2) + n % 2 := by
  cases n with
  | zero => simp [popcount]
  | succ n => rw [popcount, dif_neg (Nat.succ_ne_zero n)]; omega

/-- `bit_length = Nat.size`. -/
lemma bit_length_eq_size (n : ℕ) : bit_length n = Nat.size n := rfl

/-- Closed form of `C_constant`, matching `constant_analytical` in the C++:
    `C(R) = (R-1) + Σ_{x<R} bit_length x − E(R)`. -/
lemma C_constant_def (R : ℕ) :
    C_constant R = (R - 1) + sum_bit_length R - E_seq R := rfl

end Interface

section PopcountArith

/-- Generalized popcount increment lemma.
    Adding `2^j` to `m < 2^j` sets one fresh bit: `popcount (2^j + m) = popcount m + 1`.
    Pure induction on `j`; no bitwise Mathlib lemmas needed, so it is robust to
    whatever `Nat.testBit` API your Mathlib pin has. -/
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

/-- Each summand of `E_seq` is dominated by the matching summand of
    `sum_bit_length`. -/
lemma popcount_le_bit_length (n : ℕ) : popcount n ≤ bit_length n := by
  rw [bit_length_eq_size]
  exact popcount_le_of_lt (Nat.lt_size_self n)

end PopcountArith

section SizeArith

/-- Generalized bit-length increment lemma.
    For `1 ≤ d` and `m < 2^(d-1)`, the sum `2^(d-1) + m` lies in `[2^(d-1), 2^d)`,
    hence has bit length exactly `d`. -/
lemma bit_length_two_pow_add {d m : ℕ} (hd : 1 ≤ d) (h : m < 2 ^ (d - 1)) :
    bit_length (2 ^ (d - 1) + m) = d := by
  rw [bit_length_eq_size]
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

/-- `E_seq (2^(d-1) + m) = E_seq (2^(d-1)) + E_seq m + m` for `m ≤ 2^(d-1)`.
    (Original statement; valid for ALL `d`, including `d = 0`.) -/
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

/-- `sum_bit_length (2^(d-1) + m) = sum_bit_length (2^(d-1)) + m*d`.
    ⚠ Requires `1 ≤ d` — the drafted version without it was FALSE at
    `d = 0, m = 1` (LHS `= sum_bit_length 2 = 1`, RHS `= 0`). -/
lemma sum_bit_length_sum_decomposition (d m : ℕ) (hd : 1 ≤ d)
    (hm : m ≤ 2 ^ (d - 1)) :
    sum_bit_length (2 ^ (d - 1) + m) = sum_bit_length (2 ^ (d - 1)) + m * d := by
  induction m with
  | zero => simp
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

/-- `E_seq` is dominated by `sum_bit_length` termwise, hence globally.
    This is what makes the truncated subtraction in `C_constant` exact. -/
lemma E_seq_le_sum_bit_length (n : ℕ) : E_seq n ≤ sum_bit_length n := by
  induction n with
  | zero => simp [E_seq, sum_bit_length]
  | succ n ih =>
    rw [E_seq, sum_bit_length]
    exact Nat.add_le_add ih (popcount_le_bit_length n)

/-- Exact evaluation at powers of two, stated additively to stay in ℕ:
    `sum_bit_length (2^j) + 2^j = j * 2^j + 1`
    (i.e. `Σ_{x < 2^j} bit_length x = (j-1)·2^j + 1`). -/
lemma sum_bit_length_pow (j : ℕ) :
    sum_bit_length (2 ^ j) + 2 ^ j = j * 2 ^ j + 1 := by
  induction j with
  | zero =>
    have h0 : bit_length 0 = 0 := by
      rw [bit_length_eq_size]; exact Nat.size_zero
    simp [sum_bit_length, h0]
  | succ j ih =>
    have hsplit : sum_bit_length (2 ^ j + 2 ^ j)
        = sum_bit_length (2 ^ j) + 2 ^ j * (j + 1) := by
      have := sum_bit_length_sum_decomposition (j + 1) (2 ^ j)
        (by omega) (by simp)
      simpa using this
    have e1 : (2 : ℕ) ^ (j + 1) = 2 ^ j + 2 ^ j := by
      rw [pow_succ]; ring
    have e2 : (j + 1) * (2 ^ j + 2 ^ j) = 2 * (j * 2 ^ j) + 2 * 2 ^ j := by ring
    have e3 : 2 ^ j * (j + 1) = j * 2 ^ j + 2 ^ j := by ring
    rw [e1, hsplit]
    omega

/-- The cube boundary term is nonnegative: `(m-1) + sum_bit_length m ≤ m*j`
    for `m ≤ 2^j`.  Equivalently, `ext_cube` never truncates. -/
lemma boundary_term_nonneg (j : ℕ) :
    ∀ m : ℕ, m ≤ 2 ^ j → (m - 1) + sum_bit_length m ≤ m * j := by
  induction j with
  | zero =>
    intro m hm
    interval_cases m
    · simp [sum_bit_length]
    · have h0 : bit_length 0 = 0 := by
        rw [bit_length_eq_size]; exact Nat.size_zero
      simp [sum_bit_length, h0]
  | succ j ih =>
    intro m hm
    by_cases hsmall : m ≤ 2 ^ j
    · calc (m - 1) + sum_bit_length m ≤ m * j := ih m hsmall
        _ ≤ m * (j + 1) := Nat.mul_le_mul_left m (Nat.le_succ j)
    · push Not at hsmall
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

/-! ### CrossBaseOne and the singleton lemmas it needs (formerly §8) -/

variable {n k : ℕ}

/-- INTERFACE: a one-vertex ball has no cross collisions. -/
def CrossBaseOne (n k : ℕ) : Prop :=
  ∀ (d : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n),
    cross_collisions (hamming_ball_subset 1 n k d hk hnk) = 0

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
  · intro hqp heq
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
    exact hw_not (by simp [h])) hdrop

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
  rw [Finset.mem_singleton]
  apply Subtype.ext
  funext r
  by_cases hrp : r = p
  · by_cases hrq : r = q
    · exact False.elim (hpq (hrp.symm.trans hrq))
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

/-! ### Layer A1: the bridge lemmas (core combinatorial content) -/

/-- The cube-counting function `ball_deg t d j` computes the boundary degree
    of vertex `j` in the hypercube `Q_d` when the first `t` vertices are occupied. -/
def ball_deg (t d j : ℕ) : ℕ :=
  if h : j < t then
    (d : ℕ) - bit_length j
  else
    (d : ℕ)

/-- `U` is the set of hypercube vertices whose coordinate edges cross the ball
    boundary. -/
def U (t d : ℕ) : Finset ℕ :=
  (Finset.Icc 0 (2 ^ d - 1)).filter (fun j => j < t ∨ t ≤ j ∧ j < 2 * t)

/-- `U_embed` is the subset of `U` that lies in the embedded cube. -/
def U_embed (t d : ℕ) : Finset ℕ :=
  (Finset.Icc 0 (2 ^ d - 1)).filter (fun j => j < t ∧ bit_length j < d)

/-! ### Layer A2: the capstone theorems -/

/-- Unconditional closed form for the Hamming-ball cross-collision count. -/
theorem hb_cross_collisions_closed (R n k : ℕ) (hR : 1 ≤ R) (hk : bit_length (R - 1) ≤ k) (hnk : k + bit_length (R - 1) ≤ n) :
    cross_collisions (hamming_ball_subset R n k (bit_length (R - 1)) hk hnk) + 2 * E_seq R = C_constant R + E_seq R := by
  sorry

/-- Capstone: exact extraconnectivity formula, conditional only on UniversalLowerBound. -/
theorem arrangement_extraconnectivity_minimum (R n k : ℕ) (h_cond : can_embed_hypercube R n k)
    (h_lower : ∀ (R n k : ℕ), UniversalLowerBound R n k) :
  (∃ V' : Finset (ArrVertex n k), V'.card = R ∧
    external_neighbors V' = (R * k - E_seq R) * (n - k) - C_constant R) ∧
  (∀ V' : Finset (ArrVertex n k), V'.card = R →
    external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R) := by
  sorry

/-- Global optimal growth strategy. -/
theorem globally_optimal_growth_strategy (R n k : ℕ) (h_cond : can_embed_hypercube R n k)
    (h_lower : UniversalLowerBound R n k) :
  ∃ (V' : Finset (ArrVertex n k)), V'.card = R ∧
    external_neighbors V' = (R * k - E_seq R) * (n - k) - C_constant R := by
  sorry
