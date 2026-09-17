import Arrangement.ArrangementExtraconnectivity

/-!
# The embedding-gated hypothesis is not globally true

`RestrictedLowerBound R n k` is gated only by `can_embed_hypercube R n k`.
The full Star in `A(11,6)` (the closed neighbourhood of one vertex:
centre plus all `6 * 5 = 30` neighbours) has `R = 31`, satisfies the gate
(`bit_length 30 = 5`, `6 + 5 ≤ 11`, `5 ≤ 6`), and has external boundary
`450 < 476`.  Hence the gated hypothesis fails at `(31, 11, 6)` and cannot be
assumed for all parameters.  `A(11,6)` is the smallest full-Star witness inside
the embedding range: `m ≤ 4` Stars fail only above `2^m`, and `m = 5` first
fails at `j = 6`, forcing `k ≥ 6`.

Unlike `UniversalCounterexample.lean`'s `A(9,6)` witness (outside the gate,
since `bit_length 18 = 5 > 3`), this file refutes `RestrictedLowerBound`.
Evaluation uses `native_decide` (trusts `Lean.ofReduceBool`); expect a
runtime a few times that of the `A(9,6)` file, since `11^6 ≈ 1.8M` functions
are filtered to enumerate `ArrVertex 11 6`.
-/

namespace Arrangement

/-- Centre `(0,1,2,3,4,5)` of `A(11,6)`. -/
def star_center_11_6 : ArrVertex 11 6 :=
  ⟨fun i => ⟨i.val, by omega⟩, by decide⟩

/-- Full radius-one Star with all six branches: the closed neighbourhood. -/
def full_star_11_6 : Finset (ArrVertex 11 6) :=
  Finset.univ.filter (fun v => v = star_center_11_6 ∨ arr_adjacent star_center_11_6 v)

theorem full_star_11_6_card : full_star_11_6.card = 31 := by
  native_decide

theorem embeddable_31_11_6 : can_embed_hypercube 31 11 6 := by
  unfold can_embed_hypercube
  exact ⟨by native_decide, by native_decide⟩

/-- The gated lower bound is false at `(R, n, k) = (31, 11, 6)`. -/
theorem full_star_11_6_refutes_restricted_lower_bound :
    ¬ RestrictedLowerBound 31 11 6 := by
  intro h
  have hbound := h embeddable_31_11_6 full_star_11_6 full_star_11_6_card (by decide)
  have hviolate :
      ¬ (external_neighbors full_star_11_6
          ≥ (31 * 6 - E_seq 31) * (11 - 6) - C_constant 31) := by
    native_decide
  exact hviolate hbound

/-- The capstone hypothesis may only ever be supplied per instance. -/
theorem not_forall_restricted_lower_bound :
    ¬ ∀ R n k : ℕ, RestrictedLowerBound R n k :=
  fun h => full_star_11_6_refutes_restricted_lower_bound (h 31 11 6)

end Arrangement
