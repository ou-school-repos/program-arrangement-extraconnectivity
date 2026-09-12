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

## Status of the four former `sorry`s

1. `popcount (2^(d-1) + m') = popcount m' + 1`      — PROVEN  (`popcount_two_pow_add`)
2. `bit_length (2^(d-1) + m') = d`                  — PROVEN  (`bit_length_two_pow_add`)
3. `C_constant_recurrence` arithmetic               — PROVEN  (all Nat-subtraction handled)
4. `hb_cross_collisions_recurrence`                 — statement was WRONG as drafted
   (see §5); corrected, numerically validated for R ≤ 4096, and reduced to three
   precisely-stated combinatorial interface lemmas that require the definitions of
   `cross_collisions` / `hamming_ball_subset` / `embed_vertex` (in `ArrDefs`) to prove.
   The entire arithmetic induction driver is proven (`hb_cross_collisions_of_recurrence`);
   ONLY the interface lemmas remain.

## ⚠ Two statement-level corrections (both confirmed by machine enumeration)

* `sum_bit_length_sum_decomposition` was FALSE as drafted at `d = 0`:
  `sum_bit_length 2 = 1 ≠ 0 = sum_bit_length 1 + 1*0`.  It now carries `1 ≤ d`.
  (`E_seq_sum_decomposition` is fine for all `d` and keeps its original statement.)

* The drafted recurrence's boundary term
  `external_neighbors (hamming_ball_subset m n k (d-1) …)` cannot be correct: that
  quantity scales with `(n-k)` (e.g. it is 65 for `HB(2) ⊆ A(12,6)`), while the
  required term is the ambient-independent cube quantity
  `(m*(d-1) - E_seq m) - C_constant m` (which is 2 in the same example).
  The corrected recurrence, `cross(R) = cross(2^(d-1)) + cross(m) + ext_cube d m`,
  was verified for every `2 ≤ R ≤ 4096`.

## Definitional interface (§0)

I do not have `ArrDefs.lean` in front of me, so §0 isolates the five one-line
facts this file consumes about your definitions.  Expected proofs are given in
comments; if your definitions match the C++ (`predict.cpp`) and the usage
patterns in the original draft (`rw [E_seq]`, `unfold bit_length`,
`d = bit_length (R-1) = Nat.size (R-1)`), each is `rfl`/`simp`-trivial.
Everything below §0 depends on your code ONLY through these five lemmas.
-/

namespace Arrangement

/-! ### §0 Definitional interface — expected to be `rfl`/`simp` one-liners -/

section Interface

/-- `popcount 0 = 0`.  Expected: `rfl` or `simp [popcount]`. -/
lemma popcount_zero : popcount 0 = 0 := by
  simp [popcount]

/-- The fundamental div/2 characterization of `popcount`.  Holds for `n = 0` too
    (both sides are `0`).  Expected: `rfl` for `n+1` via the equation lemmas if
    `popcount` is defined as `| 0 => 0 | n+1 => popcount ((n+1)/2) + (n+1) % 2`;
    if it is defined by `Nat.binaryRec`, use `Nat.binaryRec_eq` instead.
    NOTE: do not add to a `simp` set — it self-loops at `n = 0`. -/
lemma popcount_div_two (n : ℕ) : popcount n = popcount (n / 2) + n % 2 := by
  cases n with
  | zero => simp [popcount]
  | succ n => rw [popcount, dif_neg (Nat.succ_ne_zero n)]; omega

/-- `bit_length = Nat.size` (per `lean-proof-status.md`).  Expected: `rfl`. -/
lemma bit_length_eq_size (n : ℕ) : bit_length n = Nat.size n := rfl

/-- Closed form of `C_constant`, matching `constant_analytical` in the C++:
    `C(R) = (R-1) + Σ_{x<R} bit_length x − E(R)`.  Expected: `rfl`. -/
lemma C_constant_def (R : ℕ) :
    C_constant R = (R - 1) + sum_bit_length R - E_seq R := rfl

/- `E_seq` and `sum_bit_length` are consumed only through their structural
   equation lemmas (`E_seq 0 = 0`, `E_seq (n+1) = E_seq n + popcount n`, and
   likewise for `sum_bit_length` with `bit_length`), exactly as the original
   draft already used them via `rw [E_seq]` / `rw [sum_bit_length]`. -/

end Interface

/-! ### §1 Popcount arithmetic — kills `sorry` #1 -/

section PopcountArith

/-- **Former `sorry` #1 (generalized).**
    Adding `2^j` to `m < 2^j` sets one fresh bit: `popcount (2^j + m) = popcount m + 1`.
    Pure induction on `j`; no bitwise Mathlib lemmas needed, so it is robust to
    whatever `Nat.testBit` API your Mathlib pin has. -/
lemma popcount_two_pow_add {j m : ℕ} (h : m < 2 ^ j) :
    popcount (2 ^ j + m) = popcount m + 1 := by
  induction j generalizing m with
  | zero =>
    -- m < 1 forces m = 0; popcount 1 = popcount 0 + 1.
    interval_cases m
    have h1 := popcount_div_two 1
    have h0 := popcount_zero
    norm_num at h1
    simpa [h0] using h1
  | succ j ih =>
    have hp : (2 : ℕ) ^ (j + 1) = 2 * 2 ^ j := by
      rw [pow_succ, Nat.mul_comm]
    -- Peel one binary digit: 2^(j+1) + m = 2*(2^j + m/2) + m%2.
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

/-! ### §2 Size arithmetic — kills `sorry` #2 -/

section SizeArith

/-- **Former `sorry` #2 (generalized).**
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

/-! ### §3 The two decomposition lemmas, completed -/

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
    -- Former `sorry` #1, discharged:
    have h_pop : popcount (2 ^ (d - 1) + m') = popcount m' + 1 :=
      popcount_two_pow_add h_lt
    rw [h_eq, E_seq, E_seq, ih h_le, h_pop]
    ring

/-- `sum_bit_length (2^(d-1) + m) = sum_bit_length (2^(d-1)) + m*d`.

    ⚠ CORRECTED STATEMENT: the drafted version omitted `1 ≤ d` and is FALSE at
    `d = 0, m = 1` (LHS `= sum_bit_length 2 = 1`, RHS `= 0`).  Every caller in
    this development satisfies `1 ≤ d`. -/
lemma sum_bit_length_sum_decomposition (d m : ℕ) (hd : 1 ≤ d)
    (hm : m ≤ 2 ^ (d - 1)) :
    sum_bit_length (2 ^ (d - 1) + m) = sum_bit_length (2 ^ (d - 1)) + m * d := by
  induction m with
  | zero => simp
  | succ m' ih =>
    have h_le : m' ≤ 2 ^ (d - 1) := by omega
    have h_lt : m' < 2 ^ (d - 1) := by omega
    have h_eq : 2 ^ (d - 1) + (m' + 1) = (2 ^ (d - 1) + m') + 1 := by omega
    -- Former `sorry` #2, discharged:
    have h_bit : bit_length (2 ^ (d - 1) + m') = d :=
      bit_length_two_pow_add hd h_lt
    rw [h_eq, sum_bit_length, ih h_le, h_bit]
    ring

end Decompositions

/-! ### §4 Auxiliary closed forms (needed to tame ℕ-subtraction in §5) -/

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
    -- sum_bit_length 1 = bit_length 0 = Nat.size 0 = 0.
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
    for `m ≤ 2^j`.  Equivalently, `ext_cube` below never truncates.
    Interpretation: the first `m` binary strings occupy at most `m·j` coordinate
    slots, of which `m-1` are consumed by internal edges and `Σ bit_length`
    by the used prefix. -/
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
    · -- 2^j < m ≤ 2^(j+1): split off the top half, m = 2^j + t, 1 ≤ t ≤ 2^j.
      push Not at hsmall
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

/-! ### §5 The C_constant recurrence — kills `sorry` #3 -/

section CConstantRecurrence

/-- The cube-internal boundary term of the reflected half.
    Both subtractions are exact by `boundary_term_nonneg` (given `m ≤ 2^(d-1)`). -/
def ext_cube (d m : ℕ) : ℕ := (m * (d - 1) - E_seq m) - C_constant m

/-- Small closed values, proved structurally (keeps `native_decide` out of the
    trusted chain; see open_issues.md #3). -/
lemma popcount_one : popcount 1 = 1 := by
  have h := popcount_div_two 1
  simpa [popcount_zero] using h

lemma bit_length_zero : bit_length 0 = 0 := by
  rw [bit_length_eq_size]; exact Nat.size_zero

lemma bit_length_one : bit_length 1 = 1 := by
  have h := bit_length_two_pow_add (d := 1) (m := 0) (by omega) (by omega)
  simpa using h

lemma E_seq_one : E_seq 1 = 0 := by
  have h : E_seq 1 = E_seq 0 + popcount 0 := rfl
  simp [popcount_zero, E_seq]

lemma E_seq_two : E_seq 2 = 1 := by
  have h : E_seq 2 = E_seq 1 + popcount 1 := rfl
  rw [h, E_seq_one, popcount_one]

lemma sum_bit_length_one : sum_bit_length 1 = 0 := by
  have h : sum_bit_length 1 = sum_bit_length 0 + bit_length 0 := rfl
  simp [bit_length_zero, sum_bit_length]

lemma sum_bit_length_two : sum_bit_length 2 = 1 := by
  have h : sum_bit_length 2 = sum_bit_length 1 + bit_length 1 := rfl
  rw [h, sum_bit_length_one, bit_length_one]

lemma C_constant_one : C_constant 1 = 0 := by
  rw [C_constant_def, sum_bit_length_one, E_seq_one]

lemma C_constant_two : C_constant 2 = 1 := by
  rw [C_constant_def, sum_bit_length_two, E_seq_two]

/-- Core form of the recurrence, no `let` binders (easier to apply). -/
theorem C_constant_recurrence' (d m : ℕ) (hm : m ≤ 2 ^ (d - 1)) (hm_pos : 0 < m) :
    C_constant (2 ^ (d - 1) + m)
      = C_constant (2 ^ (d - 1)) + C_constant m + ext_cube d m + m := by
  rcases Nat.eq_zero_or_pos d with rfl | hd
  · -- d = 0 forces m = 1 (since 2^(0-1) = 2^0 = 1); a closed computation.
    have hm1 : m = 1 := by simpa using le_antisymm hm hm_pos
    subst hm1
    have hext : ext_cube 0 1 = 0 := by
      show (1 * (0 - 1) - E_seq 1) - C_constant 1 = 0
      rw [E_seq_one, C_constant_one]
    have h2 : (2 : ℕ) ^ (0 - 1) = 1 := by norm_num
    rw [h2, hext, C_constant_one, C_constant_two]
  · -- Main case, 1 ≤ d.  Reduce everything to linear ℕ-arithmetic with
    -- truncated subtraction; `omega` case-splits the truncations, and the
    -- inequalities below pin every split to the exact branch.
    have hE := E_seq_sum_decomposition d m hm
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
    rw [ext_cube, C_constant_def, C_constant_def, C_constant_def]
    omega

/-- **Former `sorry` #3, discharged** — the theorem in its original drafted
    shape (`let`-bound `R` and `ext_m`, no extra hypotheses; the `d = 0`
    corner turns out to hold and is handled inside `C_constant_recurrence'`). -/
theorem C_constant_recurrence (d m : ℕ) (hm : m ≤ 2 ^ (d - 1)) (hm_pos : 0 < m) :
    let R := 2 ^ (d - 1) + m
    let ext_m := (m * (d - 1) - E_seq m) - C_constant m
    C_constant R = C_constant (2 ^ (d - 1)) + C_constant m + ext_m + m := by
  intro R ext_m
  exact C_constant_recurrence' d m hm hm_pos

end CConstantRecurrence

/-! ### §6 Binary reflection of the ball (unchanged from the draft: sorry-free) -/

section BinaryReflection

variable {n k d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)

/-- The first half of the Hamming Ball of size R: numbers 0 .. 2^(d-1) - 1. -/
def hb_half0 (_R d n k : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n) : Finset (ArrVertex n k) :=
  (range (2 ^ (d - 1))).image (fun i => embed_vertex n k d (nat_to_cube d i) hk hnk)

/-- The second half of the Hamming Ball of size R: numbers 2^(d-1) .. R - 1. -/
def hb_half1 (R d n k : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n) : Finset (ArrVertex n k) :=
  (Ico (2 ^ (d - 1)) R).image (fun i => embed_vertex n k d (nat_to_cube d i) hk hnk)

/-- Decomposing the range R image into the union of the two halves. -/
lemma hb_decomposition {R : ℕ} (hd : 2 ^ (d - 1) < R) (_hr : R ≤ 2 ^ d) :
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
lemma hb_disjoint {R : ℕ} (_hd : 2 ^ (d - 1) < R) (_hr : R ≤ 2 ^ d) :
    Disjoint (hb_half0 R d n k hk hnk) (hb_half1 R d n k hk hnk) := by
  rw [disjoint_iff_ne]
  rintro x hx y hy rfl
  simp only [hb_half0, mem_image, mem_range] at hx
  simp only [hb_half1, mem_image, mem_Ico] at hy
  obtain ⟨i, hi, rfl⟩ := hx
  obtain ⟨j, hj, heq⟩ := hy
  have hpow_le : (2 : ℕ) ^ (d - 1) ≤ 2 ^ d := Nat.pow_le_pow_right (by norm_num) (by omega)
  have h_inj := embed_vertex_injective_cube n k d hk hnk heq
  have h_inj_cube := nat_to_cube_injective d j i (by omega) (by omega) h_inj
  omega

/-- Bijecting the second half with the smaller Hamming Ball HB(m). -/
lemma hb_half1_eq_image_shifted (R : ℕ) (m : ℕ) (hm : R = 2 ^ (d - 1) + m) :
    hb_half1 R d n k hk hnk =
    (range m).image
      (fun j => embed_vertex n k d (nat_to_cube d (2 ^ (d - 1) + j)) hk hnk) := by
  unfold hb_half1
  subst hm
  have h_bij : Ico (2 ^ (d - 1)) (2 ^ (d - 1) + m)
      = (range m).map ⟨fun j => 2 ^ (d - 1) + j, fun a b h => by simp only at h; omega⟩ := by
    ext x
    simp only [mem_Ico, mem_map, mem_range, Function.Embedding.coeFn_mk]
    constructor
    · intro h
      refine ⟨x - 2 ^ (d - 1), by omega, by omega⟩
    · rintro ⟨j, hj, rfl⟩
      omega
  rw [h_bij, Finset.map_eq_image, Finset.image_image]
  rfl

end BinaryReflection

/-! ### §7 `CrossBaseOne` and the singleton lemmas it needs

The strong-induction route that used to live here (`CrossDimStable`,
`CrossRecurrence`, `cross_step_arith`, `hb_cross_collisions_of_recurrence`,
and the `cross_dim_stable`/`cross_recurrence`/`hb_cross_collisions`
theorems) has moved to `Arrangement.unstable.CrossRecurrenceDriver` — it is
superseded dead code (see that file's docstring) and `CrossTop.lean` does
not need it. `CrossBaseOne`/`cross_base_one` stay here because
`CrossTop.lean` still calls `cross_base_one` directly. -/

section RecurrenceDriver

variable {n k : ℕ}

/-- INTERFACE (combinatorial, needs `cross_collisions`/`hamming_ball_subset` defs):
    a one-vertex ball has no cross collisions. -/
def CrossBaseOne (n k : ℕ) : Prop :=
  ∀ (d : ℕ) (hk : d ≤ k) (hnk : k + d ≤ n),
    cross_collisions (hamming_ball_subset 1 n k d hk hnk) = 0

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

end InterfaceProofs

end Arrangement
