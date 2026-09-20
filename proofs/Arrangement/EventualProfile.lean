import Arrangement.Capstone
import Arrangement.Phase

/-!
# The eventual regime: maximum defect, then maximum collisions

Status: UNCHECKED (written without a Lean toolchain). Place under
`Arrangement/unstable/` until it builds. One lemma, `hamming_witness_roots`, is
left as `sorry` with a proof plan; everything else reduces to existing lemmas.

Setting: volume `R`, graph `A(n,k)`, slack `m = n - k`, Hamming gate open.

Main results
* `eventual_max_defect`: if `2(R²-R) < m+1`, every profile minimizer has
  defect exactly `E(R)` (no `E(R)` term in the threshold; this uses
  `higher_defect_strictly_wins_global`).
* `eventual_profile_formula`: in that regime
  `Φ(R) + R·k + X* = (R·k - E(R))·(m+1)`, where `X*` is the collision excess of
  any minimizer, which is the maximum over all maximum-defect sets.
* `eventual_hamming_exact_iff`: in that regime, `Φ(R) = H(R,n,k)` iff no
  maximum-defect `R`-set has more collisions than the Hamming ball
  (collision maximality).

Together: Hamming optimality for large slack is *equivalent* to a finite,
`n`-independent extremal statement about maximum-defect sets.
-/

namespace Arrangement

variable {n k : ℕ}

/-- The Hamming witness attains the maximum defect: its root count is
    `R*k - E(R)`.

    Proof plan: from `hamming_witness`, `external_neighbors W = H`; from
    `hb_cross_collisions_closed`, `cross_collisions W + E_seq R = C_constant R`
    (restate `HBCrossCollisions` in this form); substitute both into
    `total_coord_edges_eq` / `external_neighbors_decomp` and cancel the positive
    factor `m+1` (`Nat.eq_of_mul_eq_mul_right`). -/
lemma hamming_witness_roots (R : ℕ) (h_cond : can_embed_hypercube R n k) :
    ∃ W : Finset (ArrVertex n k), W.card = R ∧
      external_neighbors W = hamming_profile R n k ∧
      sum_unique_roots W = R * k - E_seq R := by
  sorry

/-- **Eventual maximum defect.** Once `2(R²-R) < m+1`, every set that attains
    the profile has defect exactly `E(R)`. -/
theorem eventual_max_defect (R : ℕ) (h_cond : can_embed_hypercube R n k)
    (hlarge : 2 * (R * R - R) < n - k + 1)
    (V : Finset (ArrVertex n k)) (hV : V.card = R)
    (hmin : external_neighbors V = boundary_profile R n k) :
    sum_unique_roots V = R * k - E_seq R := by
  have hnk : k ≤ n := by
    obtain ⟨h1, _⟩ := h_cond
    omega
  obtain ⟨W, hW, _, hWroots⟩ := hamming_witness_roots R h_cond
  -- defect bound: U(V) ≥ R*k - E(R) = U(W)
  have hdef := sum_unique_roots_lower_bound R V hV
  by_contra hne
  -- then V has strictly more roots than W: ΔD := U(V) - U(W) ≥ 1
  have hU : sum_unique_roots V = sum_unique_roots W +
      (sum_unique_roots V - sum_unique_roots W) := by omega
  have hpos : 1 ≤ sum_unique_roots V - sum_unique_roots W := by omega
  have hwin := higher_defect_strictly_wins_global V W hV hW hnk hU hpos hlarge
  -- W beats V, contradicting minimality of V
  have hle := boundary_profile_le (R := R) W hW
  omega

/-- **Eventual profile formula.** In the eventual regime the profile is the
    Hamming slope term minus the collision excess of a minimizer. -/
theorem eventual_profile_formula (R : ℕ) (h_cond : can_embed_hypercube R n k)
    (hlarge : 2 * (R * R - R) < n - k + 1)
    (V : Finset (ArrVertex n k)) (hV : V.card = R)
    (hmin : external_neighbors V = boundary_profile R n k) :
    boundary_profile R n k + R * k + cross_collisions V =
      (R * k - E_seq R) * (n - k + 1) := by
  have hnk : k ≤ n := by
    obtain ⟨h1, _⟩ := h_cond
    omega
  have hroots := eventual_max_defect R h_cond hlarge V hV hmin
  have htotal := total_coord_edges_eq V hnk
  have hdecomp := external_neighbors_decomp V (external_neighbors_le_total_coord V)
  -- ext + X + R*k = U*(m+1) with U = R*k - E(R)
  rw [hV] at htotal
  rw [← hmin, ← hroots]
  have hexp : sum_unique_roots V * (n - k + 1) =
      sum_unique_roots V * (n - k) + sum_unique_roots V := by ring
  unfold cross_collisions
  omega

/-- **Hamming exactness ⇔ collision maximality**, in the eventual regime.
    The right-hand side is a statement about maximum-defect `R`-sets only. -/
theorem eventual_hamming_exact_iff (R : ℕ) (h_cond : can_embed_hypercube R n k)
    (hlarge : 2 * (R * R - R) < n - k + 1) :
    boundary_profile R n k = hamming_profile R n k ↔
      ∀ V : Finset (ArrVertex n k), V.card = R →
        sum_unique_roots V = R * k - E_seq R →
        ∀ W : Finset (ArrVertex n k), W.card = R →
          external_neighbors W = hamming_profile R n k →
          sum_unique_roots W = R * k - E_seq R →
          cross_collisions V ≤ cross_collisions W := by
  have hnk : k ≤ n := by
    obtain ⟨h1, _⟩ := h_cond
    omega
  constructor
  · -- if Φ = H, a max-defect V with more collisions than W would beat H
    intro hPhi V hV hVroots W hW hWext hWroots
    have hord := equal_defect_collision_order W V hW hV hnk (by rw [hVroots, hWroots])
    by_contra hgt
    push Not at hgt
    -- equal defect and strictly more collisions ⇒ strictly smaller boundary
    have hpen := penalty_exact W V hW hV hnk (by rw [hVroots, hWroots]; simp)
    have hle := boundary_profile_le (R := R) V hV
    omega
  · -- if collision maximality holds, the minimizer (max defect by
    -- `eventual_max_defect`) has no more collisions than the Hamming ball
    intro hmax
    obtain ⟨W, hW, hWext, hWroots⟩ := hamming_witness_roots R h_cond
    obtain ⟨V, hV, hVmin⟩ := boundary_profile_attained (R := R) ⟨W, hW⟩
    have hVroots := eventual_max_defect R h_cond hlarge V hV hVmin
    have hX := hmax V hV hVroots W hW hWext hWroots
    have hord := (equal_defect_collision_order V W hV hW hnk
      (by rw [hVroots, hWroots])).mpr hX
    have hle := boundary_profile_le (R := R) W hW
    omega

end Arrangement
