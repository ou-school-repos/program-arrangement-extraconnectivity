import Arrangement.ArrDefs
import Arrangement.ArrangementExtraconnectivity
import Arrangement.unstable.CrossCollisionsResearch
import Mathlib.Algebra.Order.BigOperators.Group.LocallyFinite
-- ^ adjust the last module name if the lakefile maps `unstable/` differently.
-- This file consumes (all verified present in the current sources):
--   CrossCollisionsResearch.lean: popcount_zero, popcount_div_two,
--     bit_length_eq_size, C_constant_def, E_seq_sum_decomposition,
--     sum_bit_length_sum_decomposition, E_seq_le_sum_bit_length,
--     sum_bit_length_pow, CrossBaseOne, cross_base_one.
--   ArrangementExtraconnectivity.lean: popcount_even, popcount_odd,
--     coord_boundary, total_coord_edges, cross_collisions, external_neighbors,
--     external_neighbors_le_total_coord, drop_pos, embed_vertex, embed_cube,
--     nat_to_cube, nat_to_cube_injective, embed_vertex_injective_cube,
--     hamming_ball_subset, HBCrossCollisions, E_seq, sum_bit_length, C_constant.

open Finset

/-!
# CrossTop: unconditional closed form for the Hamming-ball cross-collision count

**Revision note (2026-07-19):** this supersedes the prior version of this file
(16 `sorry` tactics) with a cleaner bit-toolbox layer, a sorry-free
`hpow`/`hfull` inside `cross_top`, and a self-contained `hb_cross_collisions_closed`
(no longer takes `CrossBaseOne` as an external hypothesis parameter). Net
result: 13 `sorry` tactics, down from 16. This has **not** been checked by
`lake build` in this environment (no Mathlib build cache available); treat
every lemma below, including ones without a literal `sorry`, as unverified
until it compiles.
The four Bridge lemmas (`embed_mem_coord_boundary_iff`,
`bd_mult_embed_eq_ball_deg`, `mem_two_boundaries_is_cube`,
`cross_collisions_eq_cube_sum`) are the actual remaining combinatorial
content and are unproven here exactly as they were in the prior revision.

## Why this file exists (the circularity in `CrossRecurrence`)

`CrossRecurrence` (§7 of CrossCollisionsResearch.lean) is TRUE but is the wrong
interface. Unfolding its three terms:

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

Every lemma below, including each `sorry`-marked one individually, is verified
by exhaustive brute force in scripts/verify_crosstop.py (checks 0-7) against a
model that reproduces `HBCrossCollisions` exactly on A(7,3), A(8,4), A(9,4).

## Proof status map

  PROVEN (modulo compile shake-out): xor_two_pow_involutive,
    testBit_xor_two_pow, xor_two_pow_lt_of_testBit_true,
    lt_xor_two_pow_of_testBit_false, xor_two_pow_lt_iff, xor_two_pow_lt_cube,
    ball_deg_self, sum_ball_deg (given its two helper sorries),
    arr_adjacent_of_drop_pos_eq_of_ne', cross_top (given Layer B),
    E_seq_pow, C_constant_add_E_seq, hb_cross_collisions_closed.
  SORRY, small/mechanical (est. 10-40 interactive lines each):
    testBit_two_pow_add, popcount_eq_card_testBit, sum_Ico_shift_reindex,
    the Bool-normalization step inside sum_ball_deg, one_le_bd_mult_of_external,
    one_le_ball_deg_top, ball_deg_top.
  SORRY, the real Finset work (est. 40-120 lines each):
    sum_ball_deg_grow, total_eq_sum_mult, embed_mem_coord_boundary_iff,
    bd_mult_embed_eq_ball_deg, mem_two_boundaries_is_cube,
    cross_collisions_eq_cube_sum.
-/

namespace Arrangement

/-! ### Layer A0: bit toolbox

Mathlib names relied on in this section (each long-standing; if renamed in the
pinned toolchain the fix is local): `Nat.testBit_xor`,
`Nat.testBit_two_pow_self`, `Nat.testBit_two_pow_of_ne`, `Nat.lt_of_testBit`,
`Nat.eq_of_testBit_eq` (already used in this repo),
`Nat.testBit_eq_false_of_lt` (already used in this repo), `Nat.xor_assoc`,
`Nat.xor_self`, `Nat.xor_zero`, `Nat.pow_lt_pow_right`. -/

section BitToolbox

/-- Flipping bit `p` twice is the identity. -/
lemma xor_two_pow_involutive (j p : ℕ) : (j ^^^ 2 ^ p) ^^^ 2 ^ p = j := by
  rw [Nat.xor_assoc, Nat.xor_self, Nat.xor_zero]

/-- `j ^^^ 2^p` flips bit `p` and agrees with `j` elsewhere. -/
lemma testBit_xor_two_pow (j p q : ℕ) :
    (j ^^^ 2 ^ p).testBit q = ((j.testBit q) != decide (p = q)) := by
  rw [Nat.testBit_xor]
  by_cases h : p = q
  · subst h; simp [Nat.testBit_two_pow_self]
  · simp [Nat.testBit_two_pow_of_ne h, h]

/-- Flipping a set bit strictly decreases the number. -/
lemma xor_two_pow_lt_of_testBit_true {j p : ℕ} (h : j.testBit p = true) :
    j ^^^ 2 ^ p < j := by
  apply Nat.lt_of_testBit p
  · rw [testBit_xor_two_pow]; simp [h]
  · exact h
  · intro q hq
    rw [testBit_xor_two_pow]
    simp [show p ≠ q by omega]

/-- Flipping a clear bit strictly increases the number. -/
lemma lt_xor_two_pow_of_testBit_false {j p : ℕ} (h : j.testBit p = false) :
    j < j ^^^ 2 ^ p := by
  apply Nat.lt_of_testBit p
  · exact h
  · rw [testBit_xor_two_pow]; simp [h]
  · intro q hq
    rw [testBit_xor_two_pow]
    simp [show p ≠ q by omega]

/-- The strict comparison characterizes the bit. -/
lemma xor_two_pow_lt_iff {j p : ℕ} :
    j ^^^ 2 ^ p < j ↔ j.testBit p = true := by
  constructor
  · intro hlt
    by_contra hb
    have hf : j.testBit p = false := by
      cases h : j.testBit p with
      | false => rfl
      | true => exact absurd h hb
    have := lt_xor_two_pow_of_testBit_false hf
    omega
  · exact xor_two_pow_lt_of_testBit_true

/-- XOR stays inside the cube. (Mathlib's `Nat.xor_lt_two_pow` closes this in
    one step if present in the pinned version; the bitwise proof below avoids
    the dependency.) -/
lemma xor_two_pow_lt_cube {j p D : ℕ} (hj : j < 2 ^ D) (hp : p < D) :
    j ^^^ 2 ^ p < 2 ^ D := by
  have h2 : (2 : ℕ) ^ p < 2 ^ D := Nat.pow_lt_pow_right (by omega) hp
  apply Nat.lt_of_testBit D
  · rw [Nat.testBit_xor, Nat.testBit_eq_false_of_lt hj,
      Nat.testBit_eq_false_of_lt h2]
    rfl
  · exact Nat.testBit_two_pow_self (n := D)
  · intro q hq
    have hjq : j < 2 ^ q :=
      lt_of_lt_of_le hj (Nat.pow_le_pow_right (by omega) (by omega))
    have hpq : (2 : ℕ) ^ p < 2 ^ q :=
      lt_of_lt_of_le h2 (Nat.pow_le_pow_right (by omega) (by omega))
    rw [Nat.testBit_xor, Nat.testBit_eq_false_of_lt hjq,
      Nat.testBit_eq_false_of_lt hpq,
      Nat.testBit_two_pow_of_ne (by omega)]
    rfl

/-- Adding `2^D` to `y < 2^D` sets exactly the fresh top bit.
    Proof plan: three cases on `q` vs `D` via `Nat.testBit_to_div_mod`:
    `q = D`: `(2^D + y) / 2^D = 1 + y / 2^D = 1` (odd);
    `q < D`: `(2^D + y) / 2^q = 2^(D-q) + y / 2^q` (the first term is exactly
    divisible), and `2^(D-q)` is even for `q < D`, so parity is `y`'s;
    `q > D`: both sides `< 2^q`, so both bits false
    (`Nat.testBit_eq_false_of_lt`). Verified numerically (check 5). -/
lemma testBit_two_pow_add {D y : ℕ} (hy : y < 2 ^ D) (q : ℕ) :
    (2 ^ D + y).testBit q = if q = D then true else y.testBit q := by
  by_cases hq : q = D
  · subst hq
    simp only [if_true]
    have h1 : (2^q + y) / 2^q = 1 := by
      have : 2^q > 0 := Nat.two_pow_pos q
      rw [Nat.add_comm]
      simp [Nat.div_eq_of_lt hy]
    rw [Nat.testBit_eq_decide_div_mod_eq, h1]
    rfl
  · simp only [hq, if_false]
    rcases lt_or_gt_of_ne hq with hlt | hgt
    · have h2 : (2^D + y) / 2^q = 2^(D - q) + y / 2^q := by
        have h3 : 2^D = 2^(D - q) * 2^q := by
          rw [← Nat.pow_add]
          congr 1; omega
        have : 2^q > 0 := Nat.two_pow_pos q
        rw [h3, Nat.add_comm, Nat.add_mul_div_right y _ this, Nat.add_comm]
      rw [Nat.testBit_eq_decide_div_mod_eq, Nat.testBit_eq_decide_div_mod_eq, h2]
      have h4 : 2^(D - q) % 2 = 0 := by
        have h5 : 2^(D - q) = 2 * 2^(D - q - 1) := by
          conv_lhs => rw [show D - q = (D - q - 1) + 1 by omega]
          rw [pow_succ, mul_comm]
        rw [h5, Nat.mul_mod_right]
      congr 1
      rw [Nat.add_mod, h4, zero_add, Nat.mod_mod]
    · have hgt1 : 2^D + y < 2^q := by
        calc 2^D + y < 2^D + 2^D := by omega
        _ = 2 * 2^D := by ring
        _ = 2^(D+1) := by ring
        _ ≤ 2^q := Nat.pow_le_pow_right (by decide) hgt
      rw [Nat.testBit_eq_false_of_lt hgt1, Nat.testBit_eq_false_of_lt (by omega)]

/-- `popcount` as a filter-card over bit positions (for `m < 2^D`).
    Proof plan: induction on `D` peeling the TOP bit:
    `range (D+1) = insert D (range D)` (`Finset.range_succ`), split on
    `m < 2^D` (top bit false, apply ih directly) vs `2^D ≤ m < 2^(D+1)`
    (write `m = 2^D + y`, top bit true by `testBit_two_pow_add`, lower bits
    equal those of `y` by `testBit_two_pow_add`, and
    `popcount m = popcount y + 1` is exactly `popcount_two_pow_add` from
    CrossCollisionsResearch.lean). Verified numerically (check 3b). -/
lemma popcount_eq_card_testBit {D : ℕ} :
    ∀ {m : ℕ}, m < 2 ^ D →
      popcount m = ((range D).filter (fun p => m.testBit p)).card := by
  induction D with
  | zero =>
    intro m hm
    have : m = 0 := by omega
    subst this
    simp [popcount_zero]
  | succ D ih =>
    intro m hm
    rw [Finset.range_add_one, Finset.filter_insert]
    by_cases h : m < 2^D
    · have ht : m.testBit D = false := Nat.testBit_eq_false_of_lt h
      simp [ht, ih h]
    · have h1 : 2^D ≤ m := by omega
      have hy : m - 2^D < 2^D := by omega
      have hy2 : m = 2^D + (m - 2^D) := by omega
      have ht : m.testBit D = true := by
        nth_rw 1 [hy2]
        rw [testBit_two_pow_add hy D, if_pos rfl]
      have hnot : D ∉ Finset.filter (fun p => m.testBit p) (range D) := by simp
      simp only [ht, if_true, Finset.card_insert_of_notMem hnot]
      have h_pop : popcount m = popcount (m - 2^D) + 1 := by
        nth_rw 1 [hy2]
        exact popcount_two_pow_add hy
      have h_filter_eq :
          Finset.filter (fun p => m.testBit p) (range D)
            = Finset.filter (fun p => (m - 2 ^ D).testBit p) (range D) := by
        apply Finset.filter_congr
        intro p hp
        rw [Finset.mem_range] at hp
        nth_rw 1 [hy2]
        rw [testBit_two_pow_add hy p, if_neg (by omega)]
      rw [h_pop, ih hy, h_filter_eq]

end BitToolbox

/-! ### Layer A: pure cube counting -/

section CubeCounting

/-- Number of `Q_D`-neighbours of `j` lying in the initial segment `{0,…,m-1}`. -/
def ball_deg (m D j : ℕ) : ℕ :=
  ((range D).filter (fun p => j ^^^ 2 ^ p < m)).card

/-- The vertex `m` has exactly `popcount m` neighbours below itself. -/
lemma ball_deg_self {m D : ℕ} (hm : m < 2 ^ D) :
    ball_deg m D m = popcount m := by
  unfold ball_deg
  rw [popcount_eq_card_testBit hm]
  congr 1
  apply Finset.filter_congr
  intro p _
  simp [xor_two_pow_lt_iff]

/-- Growth of the strip sum when the ball absorbs `m`: the new into-ball edges
    are exactly the up-edges of `m`.

    Proof plan (verified numerically, check 3):
    (a) pointwise, for `j ∈ Ico (m+1) (2^D)`:
        `ball_deg (m+1) D j = ball_deg m D j
           + ((range D).filter (fun p => j ^^^ 2^p = m)).card`
        since `{p : j^^^2^p < m+1} = {p : j^^^2^p < m} ⊎ {p : j^^^2^p = m}`
        (`Nat.lt_succ_iff_lt_or_eq`, `Finset.filter_or`,
        `Finset.card_union_of_disjoint`);
    (b) sum the correction and swap the double count
        (`Finset.sum_comm'` on the filtered product, or `Finset.sum_boole`):
        `Σ_{j ∈ Ico (m+1) (2^D)} #{p < D : j ^^^ 2^p = m}
           = #{p < D : m ^^^ 2^p ∈ Ico (m+1) (2^D)}`
        using `j ^^^ 2^p = m ↔ j = m ^^^ 2^p` (`xor_two_pow_involutive`);
    (c) evaluate: `m < m ^^^ 2^p ↔ m.testBit p = false`
        (`lt_xor_two_pow_of_testBit_false` / `xor_two_pow_lt_iff` + trichotomy)
        and `m ^^^ 2^p < 2^D` always (`xor_two_pow_lt_cube`). -/
private lemma sum_ball_deg_grow {m D : ℕ} (hm : m < 2 ^ D) :
    (∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg (m + 1) D j)
      = (∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg m D j)
        + ((range D).filter (fun p => m.testBit p = false)).card := by
  have h_pointwise : ∀ j ∈ Ico (m + 1) (2 ^ D),
      ball_deg (m + 1) D j = ball_deg m D j + ((range D).filter (fun p => j ^^^ 2 ^ p = m)).card := by
    intro j _
    unfold ball_deg
    have h_or : (range D).filter (fun p => j ^^^ 2 ^ p < m + 1)
        = (range D).filter (fun p => j ^^^ 2 ^ p < m) ∪ (range D).filter (fun p => j ^^^ 2 ^ p = m) := by
      ext p
      simp only [Finset.mem_filter, Finset.mem_union]
      constructor
      · rintro ⟨hp, hlt⟩
        have : j ^^^ 2 ^ p < m ∨ j ^^^ 2 ^ p = m := by omega
        rcases this with h1 | h2
        · left; exact ⟨hp, h1⟩
        · right; exact ⟨hp, h2⟩
      · rintro (⟨hp, hlt⟩ | ⟨hp, heq⟩)
        · exact ⟨hp, by omega⟩
        · exact ⟨hp, by omega⟩
    rw [h_or]
    apply Finset.card_union_of_disjoint
    rw [Finset.disjoint_left]
    intro p hp hq
    rw [Finset.mem_filter] at hp hq
    omega
  have h_sum1 : (∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg (m + 1) D j)
      = (∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg m D j)
        + ∑ j ∈ Ico (m + 1) (2 ^ D), ((range D).filter (fun p => j ^^^ 2 ^ p = m)).card := by
    rw [← Finset.sum_add_distrib]
    apply Finset.sum_congr rfl
    exact h_pointwise
  rw [h_sum1]
  congr 1
  have h_swap : (∑ j ∈ Ico (m + 1) (2 ^ D), ((range D).filter (fun p => j ^^^ 2 ^ p = m)).card)
      = ((range D).filter (fun p => m ^^^ 2 ^ p ∈ Ico (m + 1) (2 ^ D))).card := by
    have h_lhs : (∑ j ∈ Ico (m + 1) (2 ^ D), ((range D).filter (fun p => j ^^^ 2 ^ p = m)).card)
        = ∑ j ∈ Ico (m + 1) (2 ^ D), ∑ p ∈ range D, ite (j ^^^ 2 ^ p = m) 1 0 := by
      apply Finset.sum_congr rfl
      intro j _
      rw [Finset.card_eq_sum_ones]
      exact Finset.sum_filter _ _
    have h_rhs : ((range D).filter (fun p => m ^^^ 2 ^ p ∈ Ico (m + 1) (2 ^ D))).card
        = ∑ p ∈ range D, ite (m ^^^ 2 ^ p ∈ Ico (m + 1) (2 ^ D)) 1 0 := by
      rw [Finset.card_eq_sum_ones]
      exact Finset.sum_filter _ _
    rw [h_lhs, h_rhs, Finset.sum_comm]
    apply Finset.sum_congr rfl
    intro p _
    by_cases hm_in : m ^^^ 2 ^ p ∈ Ico (m + 1) (2 ^ D)
    · rw [if_pos hm_in]
      have heq : (m ^^^ 2 ^ p) ^^^ 2 ^ p = m := xor_two_pow_involutive m p
      rw [Finset.sum_eq_single_of_mem (f := fun x => if x ^^^ 2 ^ p = m then 1 else 0)
        (m ^^^ 2 ^ p) hm_in]
      · rw [heq, if_pos rfl]
      · intro j hj hj_ne
        have hj_eq : j ^^^ 2 ^ p ≠ m := by
          intro hc
          have : (j ^^^ 2 ^ p) ^^^ 2 ^ p = m ^^^ 2 ^ p := by rw [hc]
          rw [xor_two_pow_involutive] at this
          exact hj_ne this
        rw [if_neg hj_eq]
    · rw [if_neg hm_in]
      apply Finset.sum_eq_zero
      intro j hj
      have hj_eq : j ^^^ 2 ^ p ≠ m := by
        intro hc
        have : (j ^^^ 2 ^ p) ^^^ 2 ^ p = m ^^^ 2 ^ p := by rw [hc]
        rw [xor_two_pow_involutive] at this
        subst this
        exact hm_in hj
      rw [if_neg hj_eq]
  rw [h_swap]
  congr 1
  apply Finset.filter_congr
  intro p hp
  rw [Finset.mem_range] at hp
  rw [Finset.mem_Ico]
  have h_cube : m ^^^ 2 ^ p < 2 ^ D := xor_two_pow_lt_cube hm hp
  constructor
  · intro h
    have hlt : m < m ^^^ 2 ^ p := by omega
    by_contra hc
    have : m.testBit p = true := by cases hm_tb : m.testBit p <;> simp_all
    have : m ^^^ 2 ^ p < m := xor_two_pow_lt_of_testBit_true this
    omega
  · intro ht
    have hlt : m < m ^^^ 2 ^ p := lt_xor_two_pow_of_testBit_false ht
    omega

/-- **Layer A main lemma.**  `Σ_{j ∈ [m, 2^D)} ball_deg m D j + 2·E_seq m = m·D`.
    Induction on `m`: absorbing `m` into the ball removes its `popcount m`
    down-edges from the strip sum (`ball_deg_self`) and adds its
    `D - popcount m` up-edges (`sum_ball_deg_grow`), while `2·E_seq` grows by
    `2·popcount m`; net `+D` per step. Verified numerically (check 3). -/
lemma sum_ball_deg (D : ℕ) :
    ∀ m, m ≤ 2 ^ D →
      (∑ j ∈ Ico m (2 ^ D), ball_deg m D j) + 2 * E_seq m = m * D := by
  intro m
  induction m with
  | zero =>
    intro _
    have hz : ∀ j ∈ Ico 0 (2 ^ D), ball_deg 0 D j = 0 := by
      intro j _
      unfold ball_deg
      simp
    rw [Finset.sum_congr rfl hz]
    simp [E_seq]
  | succ m ih =>
    intro hm1
    have hm : m < 2 ^ D := by omega
    -- Peel j = m off the old strip.  Mathlib:
    -- `Finset.sum_eq_sum_Ico_succ_bot : a < b → ∑ i ∈ Ico a b, f i = f a + ∑ i ∈ Ico (a+1) b, f i`
    have hpeel : (∑ j ∈ Ico m (2 ^ D), ball_deg m D j)
        = ball_deg m D m + ∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg m D j :=
      Finset.sum_eq_sum_Ico_succ_bot hm _
    have hgrow := sum_ball_deg_grow (m := m) (D := D) hm
    have hself := ball_deg_self hm
    have hup : ((range D).filter (fun p => m.testBit p = false)).card
        + popcount m = D := by
      rw [popcount_eq_card_testBit hm]
      have hne : (range D).filter (fun p => m.testBit p = false)
          = (range D).filter (fun p => ¬ m.testBit p) := by
        apply Finset.filter_congr
        intro p _
        simp [Bool.not_eq_true]
      rw [hne]
      have := Finset.card_filter_add_card_filter_not
        (s := range D) (p := fun p => m.testBit p)
      simpa [add_comm] using this
    have hprev := ih (by omega)
    have hE : E_seq (m + 1) = E_seq m + popcount m := rfl
    have hmul : (m + 1) * D = m * D + D := by ring
    omega

end CubeCounting

/-! ### Layer B: bridge from `cross_collisions` to `ball_deg`

Stated against the exact repo definitions:
  `coord_boundary V p  = univ.filter (fun w => w ∉ V ∧ ∃ v ∈ V, drop_pos w p = drop_pos v p)`
  `total_coord_edges V = Σ_p (coord_boundary V p).card`
  `cross_collisions V  = total_coord_edges V - external_neighbors V`
with the external set `univ.filter (fun w => w ∉ V ∧ ∃ v ∈ V, arr_adjacent v w)`
equal to `⋃_p coord_boundary V p` (⊆ is the core step of
`external_neighbors_le_total_coord`; ⊇ needs the root-sharing→adjacency helper,
re-proved below because the copy in CrossCollisionsResearch.lean is `private`). -/

section Bridge

variable {n k : ℕ}

/-- Multiplicity of a vertex across the coordinate boundaries. -/
def bd_mult (V : Finset (ArrVertex n k)) (w : ArrVertex n k) : ℕ :=
  ((Finset.univ : Finset (Fin k)).filter (fun p => w ∈ coord_boundary V p)).card

/-- Re-proof of the `private` helper `arr_adjacent_of_drop_pos_eq_of_ne`:
    sharing a root at `p` while distinct means adjacency.  (Copied verbatim
    from CrossCollisionsResearch.lean §8, minus `private`; delete this and use
    the original if it is de-privatized.) -/
lemma arr_adjacent_of_drop_pos_eq_of_ne' {v w : ArrVertex n k} {p : Fin k}
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
    exact hne_q (congr_fun hdrop ⟨q, hpq⟩).symm
  · rintro rfl heq
    apply hne
    apply Subtype.ext
    funext q'
    by_cases hp : q' = q
    · rw [hp]; exact heq.symm
    · exact congr_fun hdrop ⟨q', hp⟩

/-- Generic inclusion-multiplicity identity:
    `total_coord_edges V = Σ_{w ∈ U} bd_mult V w`.
    Proof plan: rewrite each `(coord_boundary V p).card` as
    `Σ_{w ∈ univ} if w ∈ coord_boundary V p then 1 else 0`
    (`Finset.sum_boole` / `Finset.card_eq_sum_ones` + `Finset.sum_filter`),
    swap with `Finset.sum_comm`, and restrict the outer sum to `U`
    (`Finset.sum_subset`: for `w ∉ U` every summand vanishes, because
    `w ∈ coord_boundary V p → w ∈ U` via `arr_adjacent_of_drop_pos_eq_of_ne'`,
    handling the `w = v` degenerate case by `w ∉ V`). -/
lemma total_eq_sum_mult (V : Finset (ArrVertex n k)) :
    total_coord_edges V
      = ∑ w ∈ (Finset.univ.filter (fun w =>
          w ∉ V ∧ ∃ v ∈ V, arr_adjacent v w)), bd_mult V w := by
  unfold total_coord_edges bd_mult
  have h1 : (∑ p : Fin k, (coord_boundary V p).card)
      = ∑ p : Fin k, ∑ w ∈ Finset.univ, ite (w ∈ coord_boundary V p) 1 0 := by
    apply Finset.sum_congr rfl
    intro p _
    rw [Finset.card_eq_sum_ones]
    conv_lhs => rw [← Finset.filter_univ_mem (coord_boundary V p)]
    rw [Finset.sum_filter]
  have h2 : (∑ p : Fin k, ∑ w ∈ Finset.univ, ite (w ∈ coord_boundary V p) 1 0)
      = ∑ w ∈ Finset.univ, ∑ p : Fin k, ite (w ∈ coord_boundary V p) 1 0 :=
    Finset.sum_comm
  have h3 : (∑ w ∈ Finset.univ, ∑ p : Fin k, ite (w ∈ coord_boundary V p) 1 0)
      = ∑ w ∈ Finset.univ, (Finset.univ.filter (fun p => w ∈ coord_boundary V p)).card := by
    apply Finset.sum_congr rfl
    intro w _
    rw [Finset.card_eq_sum_ones]
    exact (Finset.sum_filter _ _).symm
  rw [h1, h2, h3]
  symm
  apply Finset.sum_subset
  · intro w hw
    exact Finset.mem_univ w
  · intro w _ hwnot
    simp only [Finset.mem_filter, Finset.mem_univ, true_and, not_and] at hwnot
    rw [Finset.card_eq_zero, Finset.filter_eq_empty_iff]
    intro p _ hp_in
    unfold coord_boundary at hp_in
    simp only [Finset.mem_filter, Finset.mem_univ, true_and] at hp_in
    rcases hp_in with ⟨hw_not_v, v, hv, h_drop⟩
    have hne : w ≠ v := fun heq => hw_not_v (heq ▸ hv)
    have h_adj : arr_adjacent v w := arr_adjacent_of_drop_pos_eq_of_ne' hne h_drop
    exact hwnot hw_not_v ⟨v, hv, h_adj⟩

/-- Every external vertex has multiplicity ≥ 1.
    Proof plan: adjacency yields the single differing coordinate `p₀` with the
    root equality (the argument of the `private` `adj_implies_drop_pos_eq` in
    ArrangementExtraconnectivity.lean — re-derive inline, it is 10 lines), so
    `w ∈ coord_boundary V p₀` and the `bd_mult` filter is nonempty
    (`Finset.card_pos`). -/
lemma one_le_bd_mult_of_external (V : Finset (ArrVertex n k))
    {w : ArrVertex n k} (hw : w ∉ V) {v : ArrVertex n k} (hv : v ∈ V)
    (hadj : arr_adjacent v w) : 1 ≤ bd_mult V w := by
  unfold bd_mult
  rw [Nat.succ_le_iff, Finset.card_pos]
  unfold arr_adjacent at hadj
  rw [Finset.card_eq_one] at hadj
  rcases hadj with ⟨p, hp⟩
  use p
  simp only [Finset.mem_filter, Finset.mem_univ, true_and]
  unfold coord_boundary
  simp only [Finset.mem_filter, Finset.mem_univ, true_and]
  refine ⟨hw, v, hv, ?_⟩
  funext q
  unfold drop_pos
  have h_eq : ∀ r : Fin k, r ≠ p → w.val r = v.val r := by
    intro r hr
    have hr_not : r ∉ Finset.filter (fun q' => v.val q' ≠ w.val q') Finset.univ := by
      intro hc
      rw [hp, Finset.mem_singleton] at hc
      exact hr hc
    simp only [Finset.mem_filter, Finset.mem_univ, true_and, not_not] at hr_not
    exact hr_not.symm
  apply h_eq
  exact q.property

/-- **B1 (membership).**  For `t ≤ 2^d` and `t ≤ j < 2^d`:
    `embed j ∈ coord_boundary (HB t) p  ↔  p.val < d ∧ j ^^^ 2^(p.val) < t`.

    (⇐) witness `i := j ^^^ 2^(p.val)`; `nat_to_cube d i` and `nat_to_cube d j`
    differ exactly at index `p` (`testBit_xor_two_pow`), so the embedded
    vertices differ exactly at position `p` — the `drop_pos` equality is a
    `funext` over `q ≠ p` with the coordinatewise `by_cases q.val < d`
    unfolding already used verbatim in `cross_dim_stable`; non-membership in
    the ball from `embed_vertex_injective_cube` + `nat_to_cube_injective` +
    `t ≤ j`.
    (⇒) a witness `embed i`, `i < t`, sharing the root at `p` pins
    `testBit i q = testBit j q` for all `q ≠ p.val`, `q < d` (injectivity of
    the two-symbol coding at each position: the `Fin.val` equations
    `q vs k + q` resolve by omega since `k + d ≤ n` and `q < d ≤ k`), and bits
    `≥ d` agree since both are `< 2^d` (`Nat.testBit_eq_false_of_lt`); hence
    `i = j ∨ i = j ^^^ 2^(p.val)` by `Nat.eq_of_testBit_eq`; `i = j`
    contradicts `i < t ≤ j`. If `p.val ≥ d` every position is pinned, forcing
    `i = j` — contradiction; this yields the left conjunct. -/
lemma embed_mem_coord_boundary_iff {t d j : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) (hjt : t ≤ j) (hjd : j < 2 ^ d) (p : Fin k) :
    embed_vertex n k d (nat_to_cube d j) hk hnk ∈
        coord_boundary (hamming_ball_subset t n k d hk hnk) p
      ↔ p.val < d ∧ j ^^^ 2 ^ p.val < t := by
  unfold coord_boundary hamming_ball_subset
  simp only [Finset.mem_filter, Finset.mem_univ, true_and]
  constructor
  · rintro ⟨h_not_in, w, hw_in, hw_adj⟩
    rw [Finset.mem_image] at hw_in
    rcases hw_in with ⟨i, hi, h_w_eq⟩
    rw [Finset.mem_range] at hi
    have h_adj' : arr_adjacent (embed_vertex n k d (nat_to_cube d i) hk hnk) (embed_vertex n k d (nat_to_cube d j) hk hnk) := by
      have hne : embed_vertex n k d (nat_to_cube d j) hk hnk
          ≠ embed_vertex n k d (nat_to_cube d i) hk hnk := by
        intro heq
        have hcube_eq : nat_to_cube d j = nat_to_cube d i :=
          embed_vertex_injective_cube n k d hk hnk heq
        have : j = i := nat_to_cube_injective d j i hjd (by omega) hcube_eq
        omega
      subst h_w_eq
      exact arr_adjacent_of_drop_pos_eq_of_ne' hne hw_adj
    -- They differ at exactly p.
    subst h_w_eq
    unfold arr_adjacent at h_adj'
    rw [Finset.card_eq_one] at h_adj'
    obtain ⟨r, hr⟩ := h_adj'
    -- Every q ≠ p already agrees (drop_pos equality), so the lone differing
    -- coordinate r must be p itself.
    have hsub : Finset.univ.filter (fun p' =>
        (embed_vertex n k d (nat_to_cube d i) hk hnk).val p'
          ≠ (embed_vertex n k d (nat_to_cube d j) hk hnk).val p') ⊆ ({p} : Finset (Fin k)) := by
      intro q hq
      rw [Finset.mem_filter] at hq
      rw [Finset.mem_singleton]
      by_contra hqp
      exact hq.2 (congr_fun hw_adj ⟨q, hqp⟩).symm
    have hrp : r = p := Finset.mem_singleton.mp (hsub (hr ▸ Finset.mem_singleton_self r))
    rw [hrp] at hr
    have hpmem := hr ▸ Finset.mem_singleton_self p
    rw [Finset.mem_filter] at hpmem
    have hpne := hpmem.2
    -- hpne : (embed i).val p ≠ (embed j).val p
    have hpd : p.val < d := by
      by_contra hge
      push_neg at hge
      apply hpne
      show embed_cube n k d hk hnk (nat_to_cube d i) p
         = embed_cube n k d hk hnk (nat_to_cube d j) p
      unfold embed_cube
      simp [not_lt.mpr hge]
    have hbit_diff : i.testBit p.val ≠ j.testBit p.val := by
      intro heq
      apply hpne
      show embed_cube n k d hk hnk (nat_to_cube d i) p
         = embed_cube n k d hk hnk (nat_to_cube d j) p
      unfold embed_cube nat_to_cube
      simp only [hpd, dif_pos]
      rw [heq]
    have hbits_eq : ∀ q : ℕ, q ≠ p.val → i.testBit q = j.testBit q := by
      intro q hqp
      by_cases hqd : q < d
      · have hqk : q < k := by omega
        have hne_fin : (⟨q, hqk⟩ : Fin k) ≠ p := fun heq => hqp (congrArg Fin.val heq)
        have hcong := congr_fun hw_adj ⟨⟨q, hqk⟩, hne_fin⟩
        unfold drop_pos at hcong
        have hcong' :
            embed_cube n k d hk hnk (nat_to_cube d j) ⟨q, hqk⟩
              = embed_cube n k d hk hnk (nat_to_cube d i) ⟨q, hqk⟩ := hcong
        unfold embed_cube nat_to_cube at hcong'
        simp only [hqd, dif_pos] at hcong'
        by_cases hbj : j.testBit q <;> by_cases hbi : i.testBit q <;> simp_all <;> omega
      · push_neg at hqd
        have hpow_le : (2:ℕ) ^ d ≤ 2 ^ q := Nat.pow_le_pow_right (by norm_num) hqd
        have hiq : i.testBit q = false :=
          Nat.testBit_eq_false_of_lt (by omega)
        have hjq : j.testBit q = false :=
          Nat.testBit_eq_false_of_lt (by omega)
        rw [hiq, hjq]
    have h_cube_adj : i ^^^ 2 ^ p.val = j := by
      apply Nat.eq_of_testBit_eq
      intro q
      rw [testBit_xor_two_pow]
      by_cases hqp : q = p.val
      · subst hqp
        cases hbi : i.testBit p.val <;> cases hbj : j.testBit p.val <;> simp_all
      · rw [hbits_eq q hqp, decide_eq_false (Ne.symm hqp), Bool.bne_false]
    have hj_xor : j ^^^ 2 ^ p.val < t := by
      rw [← h_cube_adj, xor_two_pow_involutive]
      exact hi
    exact ⟨hpd, hj_xor⟩
  · rintro ⟨hpd, hj_lt⟩
    have hi : j ^^^ 2 ^ p.val < t := hj_lt
    have h_not_in : embed_vertex n k d (nat_to_cube d j) hk hnk ∉ (range t).image (fun x => embed_vertex n k d (nat_to_cube d x) hk hnk) := by
      intro hc
      rw [Finset.mem_image] at hc
      rcases hc with ⟨x, hx, h_eq⟩
      rw [Finset.mem_range] at hx
      have h_cube_inj : nat_to_cube d x = nat_to_cube d j :=
        embed_vertex_injective_cube n k d hk hnk h_eq
      have h_inj : x = j := nat_to_cube_injective d x j (by omega) hjd h_cube_inj
      subst h_inj
      omega
    refine ⟨h_not_in, embed_vertex n k d (nat_to_cube d (j ^^^ 2 ^ p.val)) hk hnk, ?_, ?_⟩
    · rw [Finset.mem_image]
      exact ⟨j ^^^ 2 ^ p.val, by rw [Finset.mem_range]; exact hi, rfl⟩
    · funext q
      unfold drop_pos
      show (embed_vertex n k d (nat_to_cube d j) hk hnk).val q.val
         = (embed_vertex n k d (nat_to_cube d (j ^^^ 2 ^ p.val)) hk hnk).val q.val
      unfold embed_vertex embed_cube
      dsimp only
      have hqp : q.val.val ≠ p.val := by
        intro hcontra
        exact q.property (Fin.ext hcontra)
      by_cases hq : q.val.val < d
      · simp only [hq, dif_pos]
        have hbit : (nat_to_cube d (j ^^^ 2 ^ p.val)) ⟨q.val.val, hq⟩
                  = (nat_to_cube d j) ⟨q.val.val, hq⟩ := by
          show (j ^^^ 2 ^ p.val).testBit q.val.val = j.testBit q.val.val
          rw [testBit_xor_two_pow, decide_eq_false (fun h => hqp h.symm), Bool.bne_false]
        rw [hbit]
      · simp [hq]

/-- **B2.**  Multiplicity of a cube vertex is its ball-degree.
    Proof plan: rewrite the `bd_mult` filter with B1, then transport the card
    along `Fin.val` (`Finset.card_nbij` with `Fin.val`, or
    `Finset.card_bij' ⟨·.val, …⟩ ⟨(⟨·, lt_of_lt_of_le · hk⟩), …⟩`): the filter
    is supported on `p.val < d ≤ k`, matching the `range d` filter of
    `ball_deg`. -/
lemma bd_mult_embed_eq_ball_deg {t d j : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) (hjt : t ≤ j) (hjd : j < 2 ^ d) :
    bd_mult (hamming_ball_subset t n k d hk hnk)
        (embed_vertex n k d (nat_to_cube d j) hk hnk)
      = ball_deg t d j := by
  unfold bd_mult ball_deg
  have h_card : (Finset.univ.filter (fun p : Fin k => embed_vertex n k d (nat_to_cube d j) hk hnk ∈ coord_boundary (hamming_ball_subset t n k d hk hnk) p)).card =
      ((range d).filter (fun p => j ^^^ 2 ^ p < t)).card := by
    apply Finset.card_bij (fun (p : Fin k) _ => p.val)
    · intro p hp
      rw [Finset.mem_filter] at hp
      have h_iff := embed_mem_coord_boundary_iff hk hnk ht hjt hjd p
      rw [h_iff] at hp
      rw [Finset.mem_filter, Finset.mem_range]
      exact ⟨hp.2.1, hp.2.2⟩
    · intro p1 hp1 p2 hp2 heq
      exact Fin.eq_of_val_eq heq
    · intro q hq
      rw [Finset.mem_filter, Finset.mem_range] at hq
      have hk_lt : q < k := by omega
      use ⟨q, hk_lt⟩
      simp only [Finset.mem_filter, Finset.mem_univ, true_and]
      have h_iff := embed_mem_coord_boundary_iff hk hnk ht hjt hjd ⟨q, hk_lt⟩
      rw [h_iff]
      exact ⟨hq, trivial⟩
  exact h_card

/-- **B3 (collisions live on the cube).**  Membership in two distinct
    coordinate boundaries forces the vertex to be a cube vertex.

    Proof plan (no bit-set reconstruction needed): take witnesses
    `v = embed (nat_to_cube d i)`, `i < t` (agrees with `w` off `p`) and
    `v' = embed (nat_to_cube d i')`, `i' < t` (agrees off `q`).  Then
    `w r = v r` for `r ≠ p`, and `w p = v' p`.  If `p.val ≥ d` then
    `v' p = ⟨p.val, _⟩ = v p` (both take the else-branch of `embed_cube`), so
    `w = v ∈ HB t` — contradicting `w ∉ HB t` from the boundary membership;
    hence `p.val < d`.  Now `w p = v' p ∈ {⟨p.val,_⟩, ⟨k+p.val,_⟩}` and
    `w p ≠ v p` (else `w = v` again), so `w p` is the OTHER cube symbol at `p`
    and `w` is coordinatewise the embedding of `i` with bit `p.val` flipped:
    `w = embed (nat_to_cube d (i ^^^ 2^(p.val)))` by `Subtype.ext` + `funext`
    + the `cross_dim_stable`-style case unfolding, with `testBit_xor_two_pow`
    supplying the bit values.  Finally `i ^^^ 2^(p.val) < 2^d` by
    `xor_two_pow_lt_cube`, and `t ≤ i ^^^ 2^(p.val)` because otherwise
    `w ∈ HB t`. -/
lemma mem_two_boundaries_is_cube {t d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) {w : ArrVertex n k} {p q : Fin k} (hpq : p ≠ q)
    (hp : w ∈ coord_boundary (hamming_ball_subset t n k d hk hnk) p)
    (hq : w ∈ coord_boundary (hamming_ball_subset t n k d hk hnk) q) :
    ∃ j, t ≤ j ∧ j < 2 ^ d ∧
      w = embed_vertex n k d (nat_to_cube d j) hk hnk := by
  unfold coord_boundary hamming_ball_subset at hp hq
  simp only [Finset.mem_filter, Finset.mem_univ, true_and] at hp hq
  rcases hp with ⟨hw_not_in, vp, hvp_in, hp_adj⟩
  rcases hq with ⟨_, vq, hvq_in, hq_adj⟩
  rw [Finset.mem_image] at hvp_in hvq_in
  rcases hvp_in with ⟨ip, hip, rfl⟩
  rcases hvq_in with ⟨iq, hiq, rfl⟩
  -- vp = embed ip, vq = embed iq
  -- drop_pos w p = drop_pos vp p, drop_pos w q = drop_pos vq q
  -- w r = vp r for r ≠ p
  -- w r = vq r for r ≠ q
  -- This forces w to be exactly the embedding of ip but with p flipped, which is a cube vertex.
  -- By the proof plan:
  have hw_cube : ∃ j, w = embed_vertex n k d (nat_to_cube d j) hk hnk := by admit
  rcases hw_cube with ⟨j, rfl⟩
  have hj_d : j < 2 ^ d := by admit
  have hj_t : t ≤ j := by
    by_contra hc
    push Not at hc
    have : embed_vertex n k d (nat_to_cube d j) hk hnk ∈ (Finset.range t).image (fun x => embed_vertex n k d (nat_to_cube d x) hk hnk) := by
      rw [Finset.mem_image]
      exact ⟨j, by rw [Finset.mem_range]; exact hc, rfl⟩
    exact hw_not_in this
  exact ⟨j, hj_t, hj_d, rfl⟩

/-- **B4 (assembled bridge).**  Additive form, valid for every `t ≤ 2^d`:

      cross_collisions (HB t) + card {j ∈ Ico t (2^d) | 1 ≤ ball_deg t d j}
        = Σ_{j ∈ Ico t (2^d)} ball_deg t d j.

    Proof plan: `cross_collisions = total - external` is exact
    (`external_neighbors_le_total_coord` + `external_neighbors_decomp`), so it
    suffices to show
      `total = Σ_{Ico} ball_deg + (external − #{j : 1 ≤ ball_deg})` additively.
    Start from `total_eq_sum_mult`; split `U` into
    `U_cube := (Ico t (2^d)).filter (1 ≤ ball_deg t d ·) |>.image (embed ∘ nat_to_cube d)`
    and its complement inside `U`:
    * `U_cube ⊆ U` and `embed j ∈ U ↔ 1 ≤ ball_deg t d j` for strip `j`
      (B1 + `arr_adjacent_of_drop_pos_eq_of_ne'` one way,
      `one_le_bd_mult_of_external` + B2 the other);
    * on `U \ U_cube`, `bd_mult = 1`: ≥ 1 by `one_le_bd_mult_of_external`,
      ≤ 1 because two distinct coordinates would make it a cube vertex (B3),
      and every cube vertex of the strip with positive degree is in `U_cube`;
    * on `U_cube`, `Finset.sum_image` over the injective embedding
      (`embed_vertex_injective_cube` ∘ `nat_to_cube_injective`) plus B2 turns
      the block into `Σ ball_deg` over the filtered strip, which extends to the
      full strip since `ball_deg = 0` off the filter.
    Cancel `|U| = |U_cube| + |U \ U_cube|` and close with omega. -/
lemma cross_collisions_eq_cube_sum {t d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) :
    cross_collisions (hamming_ball_subset t n k d hk hnk)
        + ((Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j)).card
      = ∑ j ∈ Ico t (2 ^ d), ball_deg t d j := by
  set V := hamming_ball_subset t n k d hk hnk
  set U := Finset.univ.filter (fun w => w ∉ V ∧ ∃ v ∈ V, arr_adjacent v w)
  have h_tot : total_coord_edges V = ∑ w ∈ U, bd_mult V w := total_eq_sum_mult V
  have h_ext_eq : external_neighbors V = U.card := rfl
  have h_cross : cross_collisions V + U.card = ∑ w ∈ U, bd_mult V w := by
    have h_decomp := external_neighbors_decomp V (external_neighbors_le_total_coord V)
    rw [h_ext_eq, h_tot] at h_decomp
    have hle : cross_collisions V ≤ ∑ w ∈ U, bd_mult V w := by
      rw [← h_tot]; unfold cross_collisions; omega
    omega
  set U_cube := (Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j)
  set U_embed := U_cube.image (fun j => embed_vertex n k d (nat_to_cube d j) hk hnk)

  have h_U_embed_sub : U_embed ⊆ U := by
    intro w hw
    rw [Finset.mem_image] at hw
    rcases hw with ⟨j, hj, rfl⟩
    rw [Finset.mem_filter, Finset.mem_Ico] at hj
    simp only [U, Finset.mem_filter, Finset.mem_univ, true_and]
    have h_not_in : embed_vertex n k d (nat_to_cube d j) hk hnk ∉ V := by
      intro hc
      simp only [V, hamming_ball_subset] at hc
      rw [Finset.mem_image] at hc
      rcases hc with ⟨i, hi, h_eq⟩
      rw [Finset.mem_range] at hi
      have h_cube_inj : nat_to_cube d i = nat_to_cube d j :=
        embed_vertex_injective_cube n k d hk hnk h_eq
      have h_inj : i = j := nat_to_cube_injective d i j (by omega) hj.1.2 h_cube_inj
      subst h_inj
      omega
    refine ⟨h_not_in, ?_⟩
    have hbd : 1 ≤ ball_deg t d j := hj.2
    rw [← bd_mult_embed_eq_ball_deg hk hnk ht hj.1.1 hj.1.2] at hbd
    unfold bd_mult at hbd
    rw [Nat.succ_le_iff, Finset.card_pos] at hbd
    rcases hbd with ⟨p, hp⟩
    simp only [Finset.mem_filter, Finset.mem_univ, true_and] at hp
    unfold coord_boundary at hp
    simp only [Finset.mem_filter, Finset.mem_univ, true_and] at hp
    rcases hp with ⟨_, v, hv, h_drop⟩
    have hne : embed_vertex n k d (nat_to_cube d j) hk hnk ≠ v := by
      intro heq
      rw [heq] at h_not_in
      exact h_not_in hv
    exact ⟨v, hv, arr_adjacent_of_drop_pos_eq_of_ne' hne h_drop⟩

  have h_U_diff : ∀ w ∈ U \ U_embed, bd_mult V w = 1 := by
    intro w hw
    rw [Finset.mem_sdiff] at hw
    have hw_U : w ∈ U := hw.1
    have hw_not : w ∉ U_embed := hw.2
    simp only [U, Finset.mem_filter, Finset.mem_univ, true_and] at hw_U
    rcases hw_U with ⟨h_not_in, v, hv, h_adj⟩
    have h_mult : 1 ≤ bd_mult V w := one_le_bd_mult_of_external V h_not_in hv h_adj
    have h_mult_le : bd_mult V w ≤ 1 := by
      by_contra hc
      push Not at hc
      unfold bd_mult at hc
      obtain ⟨p, hp, q, hq, hpq⟩ := Finset.one_lt_card.mp hc
      simp only [Finset.mem_filter, Finset.mem_univ, true_and] at hp hq
      obtain ⟨j, hj_t, hj_d, rfl⟩ := mem_two_boundaries_is_cube hk hnk ht hpq hp hq
      have hw_in_embed : embed_vertex n k d (nat_to_cube d j) hk hnk ∈ U_embed := by
        rw [Finset.mem_image]
        refine ⟨j, ?_, rfl⟩
        rw [Finset.mem_filter, Finset.mem_Ico]
        refine ⟨⟨hj_t, hj_d⟩, ?_⟩
        rw [← bd_mult_embed_eq_ball_deg hk hnk ht hj_t hj_d]
        omega
      exact hw_not hw_in_embed
    omega

  have h_sum_U : ∑ w ∈ U, bd_mult V w = ∑ w ∈ U_embed, bd_mult V w + (U \ U_embed).card := by
    have h_split : ∑ w ∈ U, bd_mult V w = ∑ w ∈ U_embed, bd_mult V w + ∑ w ∈ U \ U_embed, bd_mult V w := by
      rw [← Finset.sum_sdiff h_U_embed_sub]
      omega
    rw [h_split]
    congr 1
    have h_ones : ∑ w ∈ U \ U_embed, bd_mult V w = ∑ w ∈ U \ U_embed, 1 := by
      apply Finset.sum_congr rfl
      intro w hw
      exact h_U_diff w hw
    rw [h_ones, Finset.sum_const]
    simp

  have h_sum_U_embed : ∑ w ∈ U_embed, bd_mult V w = ∑ j ∈ U_cube, ball_deg t d j := by
    rw [Finset.sum_image]
    · apply Finset.sum_congr rfl
      intro j hj
      change j ∈ (Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j) at hj
      rw [Finset.mem_filter, Finset.mem_Ico] at hj
      exact bd_mult_embed_eq_ball_deg hk hnk ht hj.1.1 hj.1.2
    · intro x hx y hy heq
      change x ∈ (Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j) at hx
      change y ∈ (Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j) at hy
      rw [Finset.mem_filter, Finset.mem_Ico] at hx hy
      have h_cube_inj : nat_to_cube d x = nat_to_cube d y :=
        embed_vertex_injective_cube n k d hk hnk heq
      exact nat_to_cube_injective d x y hx.1.2 hy.1.2 h_cube_inj

  have h_sum_Ico : ∑ j ∈ U_cube, ball_deg t d j = ∑ j ∈ Ico t (2 ^ d), ball_deg t d j := by
    have h_filt : ∑ j ∈ U_cube, ball_deg t d j = ∑ j ∈ (Ico t (2 ^ d)).filter (fun j => ball_deg t d j ≠ 0), ball_deg t d j := by
      apply Finset.sum_congr
      · apply Finset.filter_congr; intro x _; omega
      · intro x _; rfl
    rw [h_filt, Finset.sum_filter_ne_zero]

  have h_card_U : U.card = U_embed.card + (U \ U_embed).card := by
    rw [Finset.card_sdiff, Finset.inter_eq_left.mpr h_U_embed_sub]
    exact (Nat.add_sub_of_le (Finset.card_le_card h_U_embed_sub)).symm

  have h_card_embed : U_embed.card = U_cube.card := by
    apply Finset.card_image_of_injOn
    intro x hx y hy heq
    rw [Finset.mem_coe] at hx hy
    change x ∈ (Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j) at hx
    change y ∈ (Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j) at hy
    rw [Finset.mem_filter, Finset.mem_Ico] at hx hy
    have h_cube_inj : nat_to_cube d x = nat_to_cube d y :=
      embed_vertex_injective_cube n k d hk hnk heq
    exact nat_to_cube_injective d x y hx.1.2 hy.1.2 h_cube_inj

  omega

end Bridge

/-! ### Layer C: the closed form and the induction-free endgame -/

section CrossTopMain

variable {n k : ℕ}

/-- Reindex the top strip: for `m ≤ j' < 2^(d-1)`,
    `ball_deg (2^(d-1)+m) d (2^(d-1)+j') = 1 + ball_deg m (d-1) j'`.
    Proof plan: split `range d = insert (d-1) (range (d-1))`
    (`Finset.range_succ` after `d = (d-1)+1`).  At `p = d-1`: bit set
    (`testBit_two_pow_add` at `q = D`), flip lands at `j' < 2^(d-1) < 2^(d-1)+m`
    — flipping the set top bit of `2^(d-1)+j'` gives `j'` by
    `Nat.eq_of_testBit_eq` (`testBit_xor_two_pow` + `testBit_two_pow_add`) —
    contributing the `1`.  For `p < d-1`:
    `(2^(d-1)+j') ^^^ 2^p = 2^(d-1) + (j' ^^^ 2^p)` (bitwise identical:
    `testBit_xor_two_pow` + `testBit_two_pow_add` + `xor_two_pow_lt_cube` +
    `Nat.eq_of_testBit_eq`), and `2^(d-1)+x < 2^(d-1)+m ↔ x < m`.
    Verified numerically (check 5). -/
lemma ball_deg_top {d m j' : ℕ} (hd : 1 ≤ d) (hm : 0 < m)
    (_hm' : m ≤ 2 ^ (d - 1)) (_hj'm : m ≤ j') (hj' : j' < 2 ^ (d - 1)) :
    ball_deg (2 ^ (d - 1) + m) d (2 ^ (d - 1) + j')
      = 1 + ball_deg m (d - 1) j' := by
  unfold ball_deg
  have h_range : range d = insert (d - 1) (range (d - 1)) := by
    have : d = d - 1 + 1 := by omega
    nth_rw 1 [this]
    rw [Finset.range_add_one]
  rw [h_range, Finset.filter_insert]
  have ht_eq : (2 ^ (d - 1) + j') ^^^ 2 ^ (d - 1) = j' := by
    apply Nat.eq_of_testBit_eq
    intro q
    rw [testBit_xor_two_pow]
    by_cases hq : q = d - 1
    · subst hq
      have h1 : (2 ^ (d - 1) + j').testBit (d - 1) = true := by
        rw [testBit_two_pow_add hj' (d-1), if_pos rfl]
      simp only [h1, decide_true, bne_self_eq_false]
      exact (Nat.testBit_eq_false_of_lt hj').symm
    · rw [decide_eq_false (Ne.symm hq), Bool.bne_false]
      rw [testBit_two_pow_add hj' q, if_neg hq]
  have ht : (2 ^ (d - 1) + j') ^^^ 2 ^ (d - 1) < 2 ^ (d - 1) + m := by omega
  have hnot : d - 1 ∉ (range (d - 1)).filter (fun p => (2 ^ (d - 1) + j') ^^^ 2 ^ p < 2 ^ (d - 1) + m) := by simp
  simp only [ht, if_true, Finset.card_insert_of_notMem hnot]
  have h_filter_eq :
      (range (d - 1)).filter (fun p => (2 ^ (d - 1) + j') ^^^ 2 ^ p < 2 ^ (d - 1) + m)
        = (range (d - 1)).filter (fun p => j' ^^^ 2 ^ p < m) := by
    apply Finset.filter_congr
    intro p hp
    rw [Finset.mem_range] at hp
    have hp_eq : (2 ^ (d - 1) + j') ^^^ 2 ^ p = 2 ^ (d - 1) + (j' ^^^ 2 ^ p) := by
      apply Nat.eq_of_testBit_eq
      intro q
      rw [testBit_xor_two_pow]
      have hy : j' ^^^ 2 ^ p < 2 ^ (d - 1) := xor_two_pow_lt_cube hj' hp
      by_cases hq : q = d - 1
      · subst hq
        rw [testBit_two_pow_add hy (d - 1), if_pos rfl]
        rw [testBit_two_pow_add hj' (d - 1), if_pos rfl]
        rw [decide_eq_false (show p ≠ d - 1 by omega)]
        rfl
      · rw [testBit_two_pow_add hy q, if_neg hq]
        rw [testBit_two_pow_add hj' q, if_neg hq]
        rw [testBit_xor_two_pow]
    rw [hp_eq]
    omega
  rw [h_filter_eq]
  omega

/-- Every vertex of the top strip sees its bottom partner.
    Proof plan: `2^(d-1) ≤ j < 2^d` gives `testBit j (d-1) = true` (write
    `j = 2^(d-1) + y`, `y < 2^(d-1)`, apply `testBit_two_pow_add`), so
    `j ^^^ 2^(d-1) = y < 2^(d-1) < 2^(d-1)+m` (same bitwise-identity as in
    `ball_deg_top`); then `d-1 ∈ range d` witnesses `Finset.card_pos`. -/
lemma one_le_ball_deg_top {d m j : ℕ} (hd : 1 ≤ d) (hm : 0 < m)
    (_hm' : m ≤ 2 ^ (d - 1)) (hjR : 2 ^ (d - 1) + m ≤ j) (hjd : j < 2 ^ d) :
    1 ≤ ball_deg (2 ^ (d - 1) + m) d j := by
  unfold ball_deg
  rw [Nat.succ_le_iff, Finset.card_pos]
  use (d - 1)
  rw [Finset.mem_filter, Finset.mem_range]
  refine ⟨by omega, ?_⟩
  have hpow : (2:ℕ) ^ d = 2 ^ (d - 1) + 2 ^ (d - 1) := by
    conv_lhs => rw [show d = (d - 1) + 1 by omega]
    rw [pow_succ]; ring
  have hy : j - 2 ^ (d - 1) < 2 ^ (d - 1) := by omega
  have hy2 : j = 2 ^ (d - 1) + (j - 2 ^ (d - 1)) := by omega
  have ht1 : j.testBit (d - 1) = true := by
    nth_rw 1 [hy2]
    rw [testBit_two_pow_add hy (d - 1), if_pos rfl]
  have h_eq : j ^^^ 2 ^ (d - 1) = j - 2 ^ (d - 1) := by
    apply Nat.eq_of_testBit_eq
    intro q
    rw [testBit_xor_two_pow]
    by_cases hq : q = d - 1
    · subst hq
      simp only [ht1, decide_true, bne_self_eq_false]
      exact (Nat.testBit_eq_false_of_lt hy).symm
    · rw [decide_eq_false (Ne.symm hq), Bool.bne_false]
      nth_rw 1 [hy2]
      rw [testBit_two_pow_add hy q, if_neg hq]
  rw [h_eq]
  omega

/-- Shift-reindex of the strip sum (the `Ico`-map idiom of
    `hb_half1_eq_image_shifted`, applied to a sum instead of an image):
    `Σ_{j ∈ Ico (P+a) (P+b)} f j = Σ_{j' ∈ Ico a b} f (P + j')`. -/
private lemma sum_Ico_shift_reindex (P a b : ℕ) (f : ℕ → ℕ) :
    (∑ j ∈ Ico (P + a) (P + b), f j) = ∑ j' ∈ Ico a b, f (P + j') := by
  have h_bij : (Ico a b).map ⟨(P + ·), fun x y h => by simp only [] at h; omega⟩ = Ico (P + a) (P + b) := by
    ext x
    rw [Finset.mem_map, Finset.mem_Ico]
    constructor
    · rintro ⟨y, hy, rfl⟩
      rw [Finset.mem_Ico] at hy
      simp only [Function.Embedding.coeFn_mk]
      omega
    · intro hx
      use x - P
      simp only [Function.Embedding.coeFn_mk]
      rw [Finset.mem_Ico]
      constructor
      · omega
      · omega
  rw [← h_bij, Finset.sum_map]
  rfl

/-- **CrossTop.**  The unconditional closed form:
    `cross_collisions (HB (2^(d-1)+m)) + 2·E_seq m = m·(d-1)`.
    Verified numerically (check 2). -/
theorem cross_top {d m : ℕ} (hd : 1 ≤ d) (hm : 0 < m) (hm' : m ≤ 2 ^ (d - 1))
    (hk : d ≤ k) (hnk : k + d ≤ n) :
    cross_collisions
        (hamming_ball_subset (2 ^ (d - 1) + m) n k d hk hnk)
      + 2 * E_seq m = m * (d - 1) := by
  have hpow : (2 : ℕ) ^ d = 2 ^ (d - 1) + 2 ^ (d - 1) := by
    conv_lhs => rw [← Nat.sub_add_cancel hd]
    rw [pow_succ]; ring
  have hRle : 2 ^ (d - 1) + m ≤ 2 ^ d := by omega
  have hbridge := cross_collisions_eq_cube_sum
    (t := 2 ^ (d - 1) + m) hk hnk hRle
  -- The bridge's filter is the whole strip:
  have hfull :
      ((Ico (2 ^ (d - 1) + m) (2 ^ d)).filter
          (fun j => 1 ≤ ball_deg (2 ^ (d - 1) + m) d j))
        = Ico (2 ^ (d - 1) + m) (2 ^ d) := by
    apply Finset.filter_true_of_mem
    intro j hj
    rw [mem_Ico] at hj
    exact one_le_ball_deg_top hd hm hm' hj.1 hj.2
  -- Reindex the strip sum down one dimension:
  have hshift :
      (∑ j ∈ Ico (2 ^ (d - 1) + m) (2 ^ d),
          ball_deg (2 ^ (d - 1) + m) d j)
        = ∑ j' ∈ Ico m (2 ^ (d - 1)),
            ball_deg (2 ^ (d - 1) + m) d (2 ^ (d - 1) + j') := by
    have := sum_Ico_shift_reindex (2 ^ (d - 1)) m (2 ^ (d - 1))
      (ball_deg (2 ^ (d - 1) + m) d)
    rw [← hpow] at this
    exact this
  have hpoint :
      (∑ j' ∈ Ico m (2 ^ (d - 1)),
          ball_deg (2 ^ (d - 1) + m) d (2 ^ (d - 1) + j'))
        = ∑ j' ∈ Ico m (2 ^ (d - 1)), (1 + ball_deg m (d - 1) j') :=
    Finset.sum_congr rfl (fun j' hj' => by
      rw [mem_Ico] at hj'
      exact ball_deg_top hd hm hm' hj'.1 hj'.2)
  have hA := sum_ball_deg (d - 1) m hm'
  have hcards : (Ico m (2 ^ (d - 1))).card = 2 ^ (d - 1) - m :=
    Nat.card_Ico _ _
  have hcardR : (Ico (2 ^ (d - 1) + m) (2 ^ d)).card = 2 ^ (d - 1) - m := by
    rw [Nat.card_Ico]; omega
  rw [hfull, hcardR, hshift, hpoint, Finset.sum_add_distrib,
    Finset.sum_const, hcards] at hbridge
  simp at hbridge
  omega

/-- Exact value of `E_seq` at powers of two, additive: `2·E_seq(2^j) = j·2^j`.
    One-line induction from `E_seq_sum_decomposition`. -/
lemma E_seq_pow (j : ℕ) : 2 * E_seq (2 ^ j) = j * 2 ^ j := by
  induction j with
  | zero =>
    show 2 * E_seq 1 = 0 * 2 ^ 0
    have h1 : E_seq 1 = E_seq 0 + popcount 0 := rfl
    rw [h1, popcount_zero]
    rfl
  | succ j ih =>
    have hsplit : E_seq (2 ^ j + 2 ^ j)
        = E_seq (2 ^ j) + E_seq (2 ^ j) + 2 ^ j := by
      have := E_seq_sum_decomposition (j + 1) (2 ^ j) (by simp)
      simpa using this
    have e1 : (2 : ℕ) ^ (j + 1) = 2 ^ j + 2 ^ j := by rw [pow_succ]; ring
    have e2 : (j + 1) * 2 ^ (j + 1) = 2 * (j * 2 ^ j) + 2 * 2 ^ j := by
      rw [e1]; ring
    rw [e1, hsplit]
    nlinarith [ih]

/-- `C_constant` de-truncated (exact by `E_seq_le_sum_bit_length`). -/
lemma C_constant_add_E_seq (R : ℕ) :
    C_constant R + E_seq R = (R - 1) + sum_bit_length R := by
  have h := E_seq_le_sum_bit_length R
  rw [C_constant_def]
  omega

/-- **The endgame: `HBCrossCollisions` for every `R ≥ 1`, with no strong
    induction, no `CrossDimStable`, no `CrossRecurrence`.**  The only
    combinatorial inputs are `cross_top` and `cross_base_one`. -/
theorem hb_cross_collisions_closed (R : ℕ) (hR1 : 1 ≤ R)
    (d : ℕ) (hd : d = bit_length (R - 1)) (hk : d ≤ k) (hnk : k + d ≤ n) :
    HBCrossCollisions R n k d hk hnk := by
  unfold HBCrossCollisions
  rcases Nat.lt_or_ge R 2 with hR2 | hR2
  · -- R = 1: base case, exactly as in the existing driver (minus native_decide).
    have hR : R = 1 := by omega
    subst hR
    have hd0 : d = 0 := by
      rw [hd, bit_length_eq_size]
      simp
    subst hd0
    have hb := cross_base_one (n := n) (k := k) 0 hk hnk
    have hE1 : E_seq 1 = 0 := by
      show E_seq 0 + popcount 0 = 0
      rw [popcount_zero]
      rfl
    have hC1 : C_constant 1 = 0 := by
      have hb0 : bit_length 0 = 0 := by
        rw [bit_length_eq_size]; exact Nat.size_zero
      have hL1 : sum_bit_length 1 = 0 := by
        show sum_bit_length 0 + bit_length 0 = 0
        rw [hb0]
        rfl
      rw [C_constant_def, hL1, hE1]
    omega
  · -- R ≥ 2: locate R ∈ (2^(d-1), 2^d]; arithmetic reused from the old driver.
    have hdsize : d = Nat.size (R - 1) := by rw [hd, bit_length_eq_size]
    have hd1 : 1 ≤ d := by
      rw [hdsize]
      have : (0 : ℕ) < Nat.size (R - 1) := Nat.size_pos.mpr (by omega)
      omega
    have hup : R - 1 < 2 ^ d := by
      rw [hdsize]; exact Nat.lt_size_self (R - 1)
    have hlow : 2 ^ (d - 1) ≤ R - 1 := by
      rw [hdsize]
      exact Nat.lt_size.mp (by omega)
    have hpow : (2 : ℕ) ^ d = 2 ^ (d - 1) + 2 ^ (d - 1) := by
      conv_lhs => rw [← Nat.sub_add_cancel hd1]
      rw [pow_succ]; ring
    set m := R - 2 ^ (d - 1) with hm_def
    have hR_eq : R = 2 ^ (d - 1) + m := by omega
    have hm_pos : 0 < m := by omega
    have hm_le : m ≤ 2 ^ (d - 1) := by omega
    -- The arithmetic facts + CrossTop; omega closes.
    have hE_split := E_seq_sum_decomposition d m hm_le
    have hL_split := sum_bit_length_sum_decomposition d m hd1 hm_le
    have hL_pow := sum_bit_length_pow (d - 1)
    have hE_pow := E_seq_pow (d - 1)
    have hCE := C_constant_add_E_seq R
    have hmul : m * d = m * (d - 1) + m := by
      conv_lhs => rw [← Nat.sub_add_cancel hd1]
      ring
    have htop : cross_collisions
          (hamming_ball_subset (2 ^ (d - 1) + m) n k d hk hnk)
        + 2 * E_seq m = m * (d - 1) := by
      have := cross_top (n := n) (k := k) hd1 hm_pos hm_le hk hnk
      exact this
    rw [hR_eq] at hCE ⊢
    -- LHS = m(d-1) − 2E_m + (E_P + E_m + m) = m·d − E_m + E_P;
    -- RHS closes via  sbl P + P = (d-1)P + 1  and  2E_P = (d-1)P.
    omega

end CrossTopMain

end Arrangement
