import Arrangement.ArrDefs
import Arrangement.ArrangementExtraconnectivity
import Arrangement.unstable.CrossCollisionsResearch

open Finset

/-!
# The superseded strong-induction route to `HBCrossCollisions`

**STATUS: DEAD CODE, kept for its numerically-validated recurrence statement
and its fully-proven arithmetic driver.** `CrossTop.lean` proves
`HBCrossCollisions` unconditionally through a completely different,
non-circular route (`hb_cross_collisions_closed`) and does **not** import
this file. Nothing here is on the live proof path.

`CrossRecurrence` is TRUE but structurally circular as an induction step:
its correction term `ext_cube d m` is essentially the `m`-scale instance of
the very goal being proven, so a direct proof would have to re-derive
`HBCrossCollisions` at `m` independently — see `CrossTop.lean`'s own module
docstring ("Why this file exists (the circularity in `CrossRecurrence`)")
for the full argument. `cross_recurrence` remains unresolved for that reason;
closing it is a research-scale undertaking, not missing tactic work,
and is not required for `HBCrossCollisions` (proven elsewhere).

Everything in this file besides `cross_recurrence` itself is fully proven:
the strong-induction driver `hb_cross_collisions_of_recurrence` is complete,
and `cross_dim_stable` is a proven interface lemma. Both are
inert unless something outside this file starts using them again.
-/

namespace Arrangement

section RecurrenceDriver

variable {n k : ℕ}

/-- INTERFACE (combinatorial): for `t ≤ 2^(d-1)`, the `d`-dimensional embedding
    of `HB(t)` never flips coordinate `d-1`, so it coincides (as a vertex set,
    hence in `cross_collisions`) with the `(d-1)`-dimensional embedding. -/
def CrossDimStable (n k : ℕ) : Prop :=
  ∀ (t d : ℕ) (ht : t ≤ 2 ^ (d - 1)) (hd : 1 ≤ d)
    (hk : d ≤ k) (hnk : k + d ≤ n),
    cross_collisions (hamming_ball_subset t n k d hk hnk) =
    cross_collisions
      (hamming_ball_subset t n k (d - 1) (by omega) (by omega))

/-- INTERFACE (combinatorial, the real content): the corrected binary-reflection
    recurrence.  Split `HB(R)` by `hb_decomposition`/`hb_disjoint`; within-half
    collisions restrict to the halves (the lower half by `CrossDimStable`, the
    upper half via the relabeling `σ` of `hb_half1_eq_image_shifted`, which is a
    graph isomorphism onto `HB(m)`); the cross-half pairs `(v, σ v)` contribute
    exactly `ext_cube d m`. -/
def CrossRecurrence (n k : ℕ) : Prop :=
  ∀ (R d m : ℕ) (_hd : d = bit_length (R - 1)) (_hR : R = 2 ^ (d - 1) + m)
    (_hm_pos : 0 < m) (_hm : m ≤ 2 ^ (d - 1))
    (hk : d ≤ k) (hnk : k + d ≤ n),
    cross_collisions (hamming_ball_subset R n k d hk hnk) =
      cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k d hk hnk) +
      cross_collisions (hamming_ball_subset m n k d hk hnk) +
      ext_cube d m

/-- The arithmetic core of each induction step, fully proven.  Given the two
    sub-instances of the target identity and the recurrence, the target
    identity at `R = P + m` follows from `E_seq_sum_decomposition` and
    `C_constant_recurrence'` by linear arithmetic — note the additive
    (`cross + E = C`) phrasing keeps everything subtraction-free here. -/
lemma cross_step_arith {d m cP cm cR : ℕ} (hm : m ≤ 2 ^ (d - 1)) (hm_pos : 0 < m)
    (h1 : cP + E_seq (2 ^ (d - 1)) = C_constant (2 ^ (d - 1)))
    (h2 : cm + E_seq m = C_constant m)
    (hrec : cR = cP + cm + ext_cube d m) :
    cR + E_seq (2 ^ (d - 1) + m) = C_constant (2 ^ (d - 1) + m) := by
  have hE := E_seq_sum_decomposition d m hm
  have hC := C_constant_recurrence' d m hm hm_pos
  omega

/-- **The driver.**  Strong induction on `R`: given the three interface lemmas,
    `HBCrossCollisions` holds for every `R ≥ 1` (at its canonical dimension
    `d = bit_length (R-1)`), assuming its three interface lemmas.

    Once `CrossBaseOne`, `CrossDimStable`, and `CrossRecurrence` are proven
    from the definitions, the `hb_cross_collisions` hypothesis threaded through
    `hamming_ball_eval` / `exists_optimal_embedding` /
    `arrangement_extraconnectivity_minimum` is discharged by this theorem. -/
theorem hb_cross_collisions_of_recurrence
    (hbase : CrossBaseOne n k) (hstab : CrossDimStable n k)
    (hrec : CrossRecurrence n k) :
    ∀ (R : ℕ), 1 ≤ R →
      ∀ (d : ℕ) (_hd : d = bit_length (R - 1))
        (hk : d ≤ k) (hnk : k + d ≤ n),
        HBCrossCollisions R n k d hk hnk := by
  intro R
  induction R using Nat.strong_induction_on with
  | _ R ih =>
    intro hR1 d hd hk hnk
    unfold HBCrossCollisions
    rcases Nat.lt_or_ge R 2 with hR2 | hR2
    · -- R = 1: d = bit_length 0 = Nat.size 0 = 0; base case + tiny computation.
      have hR : R = 1 := by omega
      subst hR
      have hd0 : d = 0 := by
        rw [hd, bit_length_eq_size]
        simp
      subst hd0
      have hb := hbase 0 hk hnk
      have hE1 : E_seq 1 = 0 := E_seq_one
      have hC1 : C_constant 1 = 0 := C_constant_one
      omega
    · -- R ≥ 2: split R = 2^(d-1) + m with 1 ≤ m ≤ 2^(d-1), recurse on both halves.
      have hdsize : d = Nat.size (R - 1) := by rw [hd, bit_length_eq_size]
      have hd1 : 1 ≤ d := by
        rw [hdsize]
        have : (0 : ℕ) < Nat.size (R - 1) := Nat.size_pos.mpr (by omega)
        omega
      -- 2^(d-1) ≤ R - 1 < 2^d  from the size characterization.
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
      -- Induction hypothesis at the power-of-two half P = 2^(d-1),
      -- whose canonical dimension is d - 1.
      have hdP : d - 1 = bit_length (2 ^ (d - 1) - 1) := by
        rw [bit_length_eq_size]
        rcases Nat.eq_or_lt_of_le hd1 with h1 | h1
        · -- d = 1: P = 1, size 0 = 0 = d - 1.
          rw [← h1]; simp
        · -- d ≥ 2: 2^(d-2) ≤ 2^(d-1) - 1 < 2^(d-1).
          have hup' : 2 ^ (d - 1) - 1 < 2 ^ (d - 1) := by omega
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
      -- Induction hypothesis at the reflected half m, at ITS canonical
      -- dimension d' = bit_length (m - 1) ≤ d - 1.
      have hdm_le : bit_length (m - 1) ≤ d - 1 := by
        rw [bit_length_eq_size]
        exact Nat.size_le.mpr (by omega)
      have hIH_m : HBCrossCollisions m n k (bit_length (m - 1))
          (by omega) (by omega) :=
        ih m (by omega) hm_pos (bit_length (m - 1)) rfl (by omega) (by omega)
      -- Lift both to ambient dimension d via CrossDimStable (iterated for m).
      -- The stability interface transports cross_collisions across ambient
      -- dimensions; combined with the recurrence and cross_step_arith the
      -- goal closes.
      have hstab_P :
          cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k d hk hnk) =
          cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k (d - 1)
            (by omega) (by omega)) :=
        hstab (2 ^ (d - 1)) d (by omega) hd1 hk hnk
      have hstab_m :
          cross_collisions (hamming_ball_subset m n k d hk hnk) =
          cross_collisions (hamming_ball_subset m n k (bit_length (m - 1))
            (by omega) (by omega)) := by
        -- Descend one ambient dimension at a time from d to bit_length (m-1),
        -- using m ≤ 2^(j-1) at every intermediate j > bit_length (m-1):
        -- m - 1 < 2^(bit_length (m-1)) ≤ 2^(j-1).
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
            show cross_collisions
                (hamming_ball_subset m n k (bit_length (m - 1) + j + 1) hkj hnkj) =
              cross_collisions (hamming_ball_subset m n k (bit_length (m - 1))
                (by omega) (by omega))
            exact hstep.trans this
        have := hkey (d - bit_length (m - 1)) (by omega)
          (by omega) (by omega)
        simpa [show bit_length (m - 1) + (d - bit_length (m - 1)) = d
          by omega] using this
      -- Assemble: recurrence + lifted IHs + arithmetic core.
      have hrec' := hrec R d m hd hR_eq hm_pos hm_le hk hnk
      unfold HBCrossCollisions at hIH_P hIH_m
      have h1 : cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k d hk hnk)
          + E_seq (2 ^ (d - 1)) = C_constant (2 ^ (d - 1)) := by
        rw [hstab_P]; exact hIH_P
      have h2 : cross_collisions (hamming_ball_subset m n k d hk hnk)
          + E_seq m = C_constant m := by
        rw [hstab_m]; exact hIH_m
      have hrec'' : cross_collisions (hamming_ball_subset (2 ^ (d - 1) + m) n k d hk hnk) =
          cross_collisions (hamming_ball_subset (2 ^ (d - 1)) n k d hk hnk) +
          cross_collisions (hamming_ball_subset m n k d hk hnk) + ext_cube d m := by
        rw [← hR_eq]; exact hrec'
      rw [hR_eq]
      exact cross_step_arith hm_le hm_pos h1 h2 hrec''

end RecurrenceDriver

section InterfaceProofs

variable {n k : ℕ}

/-- PROVEN: for `t ≤ 2^(d-1)`, the extra fresh coordinate `d-1` is never used by the
    Hamming ball of size `t` (bit `d-1` of every `i < t` is `0`), so the two
    `hamming_ball_subset` Finsets are literally equal, hence so are their
    `cross_collisions`. -/
theorem cross_dim_stable : CrossDimStable n k := by
  intro t d ht hd hk hnk
  have hk' : d - 1 ≤ k := by omega
  have hnk' : k + (d - 1) ≤ n := by omega
  have hset : hamming_ball_subset t n k d hk hnk =
      hamming_ball_subset t n k (d - 1) hk' hnk' := by
    unfold hamming_ball_subset
    apply Finset.image_congr
    intro i hi
    rw [Finset.mem_coe, Finset.mem_range] at hi
    have hi' : i < 2 ^ (d - 1) := lt_of_lt_of_le hi ht
    apply Subtype.ext
    funext p
    unfold embed_vertex embed_cube nat_to_cube
    by_cases hp1 : p.val < d - 1
    · have hp1' : p.val < d := by omega
      simp only [hp1, hp1', dif_pos]
    · by_cases hp2 : p.val < d
      · have hpeq : p.val = d - 1 := by omega
        have htb : i.testBit p.val = false := by
          rw [hpeq]; exact Nat.testBit_eq_false_of_lt hi'
        simp [hp1, hp2, htb]
      · simp [hp1, hp2]
  rw [hset]

/-- **The genuinely open, circular gap** — see the module docstring. Not
    required by `HBCrossCollisions` (proven unconditionally in
    `CrossTop.lean` by a different route); kept here as the honest record of
    what the superseded strong-induction route still needs. -/
theorem cross_recurrence : CrossRecurrence n k := by
  intro R d m hd hR hm_pos hm hk hnk
  sorry

/-- The final step of the superseded route: `HBCrossCollisions` from the three
    interface theorems. Dead code — `CrossTop.lean` proves the same
    conclusion unconditionally by a non-circular route and does not call
    this. Kept only because `hb_cross_collisions_of_recurrence` is otherwise
    an orphaned driver. -/
theorem hb_cross_collisions (R : ℕ) (hR1 : 1 ≤ R) (d : ℕ) (hd : d = bit_length (R - 1))
    (hk : d ≤ k) (hnk : k + d ≤ n) :
    HBCrossCollisions R n k d hk hnk :=
  hb_cross_collisions_of_recurrence cross_base_one cross_dim_stable cross_recurrence R hR1 d hd hk hnk

end InterfaceProofs

end Arrangement
