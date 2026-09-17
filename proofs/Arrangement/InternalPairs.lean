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
  (Finset.univ : Finset (Fin k)).sum (fun p =>
    (V.image (fun v => drop_pos v p)).sum (fun r =>
      Nat.choose ((V.filter (fun v => drop_pos v p = r)).card) 2))

private def v01 : ArrVertex 4 2 :=
  ⟨fun i => Fin.cases 0 (fun _ => 1) i, by decide⟩

private def v21 : ArrVertex 4 2 :=
  ⟨fun i => Fin.cases 2 (fun _ => 1) i, by decide⟩

private def v31 : ArrVertex 4 2 :=
  ⟨fun i => Fin.cases 3 (fun _ => 1) i, by decide⟩

private def star_triple : Finset (ArrVertex 4 2) := {v01, v21, v31}

/-- The three vertices share one root with occupancy three, so K is 3. -/
example : fiber_pair_count star_triple = 3 := by
  native_decide

/-- Here X + D is 2, so it is not the internal pair count K. -/
example :
    cross_collisions star_triple +
        (star_triple.card * 2 - sum_unique_roots star_triple) = 2 := by
  native_decide

end Arrangement
