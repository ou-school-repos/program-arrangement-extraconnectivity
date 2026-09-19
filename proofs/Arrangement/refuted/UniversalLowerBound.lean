import Arrangement.ArrangementExtraconnectivity

/-!
# Refuted universal boundary lower bound

This proposition is retained only so the explicit Lean counterexample can
state its negation. It is false: the full Star in `A(9,6)` has size 19 and
external boundary 180, while the proposed formula requires at least 186.
The larger `A(10,8)` counterexample is also documented in the project notes.

This file is deliberately outside the active `lake_lib` roots, under
`Arrangement/refuted/`. It is not an axiom, theorem, or active conjecture.
-/

namespace Arrangement.refuted

/-- The former unrestricted lower-bound claim, retained for its refutation. -/
def UniversalLowerBound (R n k : ℕ) : Prop :=
  ∀ (V' : Finset (ArrVertex n k)), V'.card = R → k ≤ n →
    external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R

end Arrangement.refuted
