import Arrangement.ArrangementExtraconnectivity

/-!
# Archived factor-one collision interfaces

These statements are not needed by the active development, which uses the
proved factor-two global charging bound. `CrossCollisionBound` is a plausible
and believed-true sharper estimate, but is not proved here.

Do not add the ordered-overlap estimate with right side `R * (R - 1)`: it is
false. In `A(5,3)`, the two vertices `(0,1,2)` and `(3,4,2)` have two common
neighbors in each orientation of coordinates `{0,1}`, so the ordered overlap
is 4 while `R * (R - 1) = 2`. The factor-two bound in the active module is
consistent with this example and is proved there.
-/

namespace Arrangement.unused

/-- The sharper factor-one cross-collision estimate, currently unproved. -/
def CrossCollisionBound {n k : ℕ} (V' : Finset (ArrVertex n k)) : Prop :=
  cross_collisions V' ≤ V'.card * (V'.card - 1)

/-- The unordered Bonferroni estimate proposed as one route to the factor-one
    collision bound. -/
def CoordinateBonferroni {n k : ℕ} (V' : Finset (ArrVertex n k)) : Prop :=
  total_coord_edges V' ≤ external_neighbors V' +
    (Finset.univ : Finset (Fin k)).sum (fun p =>
      ((Finset.univ : Finset (Fin k)).filter (fun q => p < q)).sum (fun q =>
        (coord_boundary V' p ∩ coord_boundary V' q).card))

/-- The unordered coordinate-pair charging estimate proposed as the second
    ingredient. -/
def CoordinatePairCharging {n k : ℕ} (V' : Finset (ArrVertex n k)) : Prop :=
  (Finset.univ : Finset (Fin k)).sum (fun p =>
      ((Finset.univ : Finset (Fin k)).filter (fun q => p < q)).sum (fun q =>
        (coord_boundary V' p ∩ coord_boundary V' q).card)) ≤
    V'.card * (V'.card - 1)

/-- Conjectural combination of unordered Bonferroni and pair charging. -/
def CrossCollisionChargingConjecture {n k : ℕ}
    (V' : Finset (ArrVertex n k)) : Prop :=
  CoordinateBonferroni V' ∧ CoordinatePairCharging V'

/-- The proposed interface implication is elementary; its two hypotheses
    remain unproved in general. -/
theorem cross_collisions_of_charging {n k : ℕ}
    (V' : Finset (ArrVertex n k))
    (h : CrossCollisionChargingConjecture V') :
    CrossCollisionBound V' := by
  unfold CrossCollisionChargingConjecture CoordinateBonferroni
    CoordinatePairCharging CrossCollisionBound cross_collisions at *
  omega

end Arrangement.unused
