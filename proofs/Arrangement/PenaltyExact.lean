import Arrangement.ArrDefs
import Arrangement.ArrangementExtraconnectivity

/-!
# The true penalty theorem (replaces `sub_optimal_penalty` / `CollisionAdjustedBound`)

`CollisionAdjustedBound` states `external ≥ U·(n−k) − C_constant R`.  By the
exact identity below this is equivalent to `X(V') + D(V') ≤ C_constant R`,
which is the combined-waste bound REFUTED by the star-graph counterexample
(R=8 in A(15,7): 28 ≰ 12; docs/axiom-equivalence.md).  So the hypothesis of
`sub_optimal_penalty` is false for some (n,k,R) and the theorem, while
vacuously sound, should not be advertised.

The replacement needs no hypotheses at all.  From the already-proven
`total_coord_edges_eq` we get an EXACT unconditional identity, and the
comparative penalty of paper Theorem `them:penalty` (eq:penalty-expansion)
falls out by omega.  Both statements verified on 500 random subset pairs of
A(7,3) (scripts/penalty_check.py).

Status: every proof below is expected to close as written; the only inputs are
`total_coord_edges_eq`, `external_neighbors_le_total_coord`,
`sum_unique_roots_le_rk`, and `cross_collisions`'s definition — all in the
stable file.  After this lands:
  * delete (or archive under `unstable/`) `CollisionAdjustedBound` and the old
    `sub_optimal_penalty`;
  * update `lean-proof-status.md`: drop the Collision-Adjusted Bound row,
    change the Asymptotic Penalty row to `penalty_exact` / `penalty_ge`,
    status PROVEN (unconditional);
  * the paper's Theorem `them:penalty` can then cite a mechanized identity for
    eq:penalty-expansion (its O_R(1) framing already matches `penalty_exact`).
-/

namespace Arrangement

variable {n k : ℕ}

/-- **Exact boundary identity** (unconditional).  This is
    eq:boundary of the paper in additive ℕ form:
    `|∂V'| + X(V') + R·k = U(V')·(n−k) + U(V')`. -/
lemma boundary_identity (V' : Finset (ArrVertex n k)) (hnk : k ≤ n) :
    external_neighbors V' + cross_collisions V' + V'.card * k
      = sum_unique_roots V' * (n - k) + sum_unique_roots V' := by
  have htot := total_coord_edges_eq V' hnk
  have hle := external_neighbors_le_total_coord V'
  unfold cross_collisions
  omega

/-- **Exact comparative penalty** (unconditional).  For two subsets of the same
    cardinality with root surplus `ΔD = U(V₁) − U(V₂)`:

      |∂V₁| + X(V₁) = |∂V₂| + X(V₂) + ΔD·(n−k+1).

    This is the mechanized core of paper Theorem `them:penalty`
    (eq:penalty-expansion): with defects `D = R·k − U`, the same equation reads
    `|∂V₁| + (X₁ + D₁) = |∂V₂| + (X₂ + D₂) + ΔD·(n−k)` — see `penalty_defect`
    below.  No bound on X or D is needed or claimed. -/
theorem penalty_exact {R ΔD : ℕ} (V₁ V₂ : Finset (ArrVertex n k))
    (h₁ : V₁.card = R) (h₂ : V₂.card = R) (hnk : k ≤ n)
    (hU : sum_unique_roots V₁ = sum_unique_roots V₂ + ΔD) :
    external_neighbors V₁ + cross_collisions V₁
      = external_neighbors V₂ + cross_collisions V₂ + ΔD * (n - k + 1) := by
  have hid₁ := boundary_identity V₁ hnk
  have hid₂ := boundary_identity V₂ hnk
  rw [h₁] at hid₁
  rw [h₂] at hid₂
  rw [hU] at hid₁
  have hmul : (sum_unique_roots V₂ + ΔD) * (n - k)
      = sum_unique_roots V₂ * (n - k) + ΔD * (n - k) :=
    add_mul _ _ _
  have hmul' : ΔD * (n - k + 1) = ΔD * (n - k) + ΔD := by ring
  omega

/-- Comparative penalty, defect form, matching eq:penalty-expansion verbatim:
    `|∂V₁| + (X₁ + D₁) = |∂V₂| + (X₂ + D₂) + ΔD·(n−k)` where `D = R·k − U`.
    Uses `sum_unique_roots_le_rk` so the ℕ-subtractions in the defects are
    exact. -/
theorem penalty_defect {R ΔD : ℕ} (V₁ V₂ : Finset (ArrVertex n k))
    (h₁ : V₁.card = R) (h₂ : V₂.card = R) (hnk : k ≤ n)
    (hU : sum_unique_roots V₁ = sum_unique_roots V₂ + ΔD) :
    external_neighbors V₁ + (cross_collisions V₁ + (R * k - sum_unique_roots V₁))
      = external_neighbors V₂ + (cross_collisions V₂ + (R * k - sum_unique_roots V₂))
        + ΔD * (n - k) := by
  have hpe := penalty_exact V₁ V₂ h₁ h₂ hnk hU
  have hU₁ := sum_unique_roots_le_rk R V₁ h₁
  have hU₂ := sum_unique_roots_le_rk R V₂ h₂
  have hmul' : ΔD * (n - k + 1) = ΔD * (n - k) + ΔD := by ring
  omega

/-- One-sided reading: a subset with root surplus `ΔD` over a competitor pays at
    least `ΔD·(n−k+1) − X(V₁)` extra boundary — the honest, unconditional form
    of the old `sub_optimal_penalty` (which needed the refuted
    `CollisionAdjustedBound`).  Note the penalty is charged against V₁'s own
    cross-collisions; no dimension-independent cap on them is asserted. -/
theorem penalty_ge {R ΔD : ℕ} (V₁ V₂ : Finset (ArrVertex n k))
    (h₁ : V₁.card = R) (h₂ : V₂.card = R) (hnk : k ≤ n)
    (hU : sum_unique_roots V₁ = sum_unique_roots V₂ + ΔD) :
    external_neighbors V₁ + cross_collisions V₁
      ≥ external_neighbors V₂ + ΔD * (n - k + 1) := by
  have hpe := penalty_exact V₁ V₂ h₁ h₂ hnk hU
  omega

end Arrangement
