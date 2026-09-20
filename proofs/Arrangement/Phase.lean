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
