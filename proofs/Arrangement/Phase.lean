import Arrangement.PenaltyExact

/-!
# Pairwise phase comparisons

For two fixed-cardinality sets, the exact penalty identity makes the boundary
difference affine in the ambient slack `n - k`.  These lemmas formalize the
pairwise crossing rule.  A phase diagram for a family of shapes additionally
needs proofs of each shape's defect, collision count, and feasibility.
-/

namespace Arrangement

variable {n k : ℕ}

/-- If `V₂` has `ΔD` more defect than `V₁`, its boundary is no larger exactly
when the collision advantage of `V₂` pays for the defect gap at this slack.
This is the natural-number threshold form of `penalty_exact`. -/
theorem higher_defect_wins_iff {R ΔD : ℕ}
    (V₁ V₂ : Finset (ArrVertex n k))
    (h₁ : V₁.card = R) (h₂ : V₂.card = R) (hnk : k ≤ n)
    (hU : sum_unique_roots V₁ = sum_unique_roots V₂ + ΔD) :
    external_neighbors V₂ ≤ external_neighbors V₁ ↔
      cross_collisions V₁ ≤
        cross_collisions V₂ + ΔD * (n - k + 1) := by
  have hpenalty := penalty_exact V₁ V₂ h₁ h₂ hnk hU
  omega

/-- A defect gap beats a bounded collision advantage once its linear penalty
exceeds that advantage.  The collision cap may be supplied by a family
specific argument or by `cross_collision_bound_global`. -/
theorem higher_defect_strictly_wins_of_collision_cap {R ΔD collision_cap : ℕ}
    (V₁ V₂ : Finset (ArrVertex n k))
    (h₁ : V₁.card = R) (h₂ : V₂.card = R) (hnk : k ≤ n)
    (hU : sum_unique_roots V₁ = sum_unique_roots V₂ + ΔD)
    (hcollision : cross_collisions V₁ ≤ collision_cap)
    (hgap : collision_cap < ΔD * (n - k + 1)) :
    external_neighbors V₂ < external_neighbors V₁ := by
  have hpenalty := penalty_exact V₁ V₂ h₁ h₂ hnk hU
  omega

/-- Since every collision count is nonnegative, a maximum-defect witness
strictly beats any set at least one defect unit below it once the ambient slack
exceeds the global collision cap.  In contrast with the older rigidity bound,
this direct comparison has no `E_seq R` term. -/
theorem higher_defect_strictly_wins_global {R ΔD : ℕ}
    (V₁ V₂ : Finset (ArrVertex n k))
    (h₁ : V₁.card = R) (h₂ : V₂.card = R) (hnk : k ≤ n)
    (hU : sum_unique_roots V₁ = sum_unique_roots V₂ + ΔD)
    (hpositive : 1 ≤ ΔD)
    (hlarge : R * R - R < n - k + 1) :
    external_neighbors V₂ < external_neighbors V₁ := by
  have hcollision : cross_collisions V₁ ≤ R * R - R := by
    have hcollision := cross_collision_bound_factor_one V₁
    rw [h₁] at hcollision
    exact hcollision
  have hfactor : n - k + 1 ≤ ΔD * (n - k + 1) := by
    simpa only [one_mul] using Nat.mul_le_mul_right (n - k + 1) hpositive
  have hgap : R * R - R < ΔD * (n - k + 1) :=
    lt_of_lt_of_le hlarge hfactor
  exact higher_defect_strictly_wins_of_collision_cap V₁ V₂ h₁ h₂ hnk hU
    hcollision hgap

/-- At equal defect, the set with at least as many cross-collisions has no
larger external boundary. -/
theorem equal_defect_collision_order {R : ℕ}
    (V₁ V₂ : Finset (ArrVertex n k))
    (h₁ : V₁.card = R) (h₂ : V₂.card = R) (hnk : k ≤ n)
    (hU : sum_unique_roots V₁ = sum_unique_roots V₂) :
    external_neighbors V₂ ≤ external_neighbors V₁ ↔
      cross_collisions V₁ ≤ cross_collisions V₂ := by
  have hU' : sum_unique_roots V₁ = sum_unique_roots V₂ + 0 := by omega
  have hpenalty := penalty_exact V₁ V₂ h₁ h₂ hnk hU'
  omega

end Arrangement
