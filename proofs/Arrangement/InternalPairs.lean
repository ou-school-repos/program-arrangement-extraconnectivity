import Arrangement.PenaltyExact

/-!
# Internal fiber pairs versus external cross-collisions

`fiber_pair_count` is the genuine internal quantity

  `Σᵣ choose (|S ∩ r|) 2`.

It is useful for exploratory edge-density arguments, but it is deliberately
kept separate from `cross_collisions`.  The latter counts multiplicity excess
at external boundary vertices.  They are not interchangeable: two adjacent
vertices in `A(4,2)` already give a counterexample to the equality
`fiber_pair_count = cross_collisions + defect`.
-/

namespace Arrangement

variable {n k : ℕ}

/-- Sum of internal vertex-pairs lying in a common coordinate root. -/
def fiber_pair_count (V : Finset (ArrVertex n k)) : ℕ :=
  ∑ p : Fin k, (∑ r in V.image (fun v => drop_pos v p),
    Nat.choose ((V.filter (fun v => drop_pos v p = r)).card) 2)

private def v01 : ArrVertex 4 2 :=
  ⟨fun i => Fin.cases 0 (fun _ => 1) i, by decide⟩

private def v02 : ArrVertex 4 2 :=
  ⟨fun i => Fin.cases 0 (fun _ => 2) i, by decide⟩

private def adjacent_pair : Finset (ArrVertex 4 2) := {v01, v02}

/-- The pair shares exactly one coordinate root. -/
example : fiber_pair_count adjacent_pair = 1 := by
  native_decide

/-- Its external collision excess plus defect is `m = 2`, not `1`. -/
example :
    cross_collisions adjacent_pair +
        (adjacent_pair.card * 2 - sum_unique_roots adjacent_pair) = 2 := by
  native_decide

end Arrangement
