/-
  UniversalCounterexample.lean
  ============================
  Mechanized refutation of `Arrangement.refuted.UniversalLowerBound` (defined
  alongside the refutation files), and
  `docs/proof-sketch-weighted-potential.md`'s "Full-Star Failure Landscape"
  section, for the mathematical context).

  The paper's Proposition 5.3 remark exhibits the refuting witness in
  `A(10,8)` at `R=17`. That parameter cell's vertex Fintype has
  `10!/(10-8)! = 1,814,400` elements; `#eval Fintype.card (ArrVertex 10 8)`
  alone does not finish within 90s on this machine (the `ArrVertex`
  `Fintype` instance enumerates all injective functions with no special
  optimization), so mechanizing that exact witness is not currently
  tractable here.

  Instead this file mechanizes the smaller, mathematically equivalent
  full-Star failure at `A(9,6)`, `m=3`, `j=6` branches, `R=19` --
  identified by `scripts/occupancy_sweep.py` as a full-Star failure case
  in the same family. `A(9,6)` has only `9!/3! = 60,480` vertices, which
  `native_decide` handles in seconds. This is a genuine, independently
  verified counterexample to `UniversalLowerBound`, not a restatement of
  the paper's witness -- see the Python cross-check below.

  Python cross-check (scripts/occupancy_sweep.py's underlying formulas,
  run by hand for this cell): the full Star in A(9,6) (center plus all 18
  vertices obtained by replacing one of the 6 coordinates with 6, 7, or 8)
  has R=19, external boundary 180, while
  `(R*k - E_seq R)*(n-k) - C_constant R = (19*6-37)*3-45 = 186`.
  180 < 186, so `UniversalLowerBound 19 9 6` is false.
-/

import Arrangement.ArrDefs
import Arrangement.ArrangementExtraconnectivity
import Arrangement.refuted.UniversalLowerBound

set_option autoImplicit false

namespace Arrangement

/-- The center of the full Star: the identity embedding `Fin 6 ↪ Fin 9`. -/
def star_center_9_6 : ArrVertex 9 6 :=
  ⟨fun i => ⟨i.val, by omega⟩, by decide⟩

def leaf_0_6 : ArrVertex 9 6 := ⟨fun i => if i.val = 0 then ⟨6, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_0_7 : ArrVertex 9 6 := ⟨fun i => if i.val = 0 then ⟨7, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_0_8 : ArrVertex 9 6 := ⟨fun i => if i.val = 0 then ⟨8, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_1_6 : ArrVertex 9 6 := ⟨fun i => if i.val = 1 then ⟨6, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_1_7 : ArrVertex 9 6 := ⟨fun i => if i.val = 1 then ⟨7, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_1_8 : ArrVertex 9 6 := ⟨fun i => if i.val = 1 then ⟨8, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_2_6 : ArrVertex 9 6 := ⟨fun i => if i.val = 2 then ⟨6, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_2_7 : ArrVertex 9 6 := ⟨fun i => if i.val = 2 then ⟨7, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_2_8 : ArrVertex 9 6 := ⟨fun i => if i.val = 2 then ⟨8, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_3_6 : ArrVertex 9 6 := ⟨fun i => if i.val = 3 then ⟨6, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_3_7 : ArrVertex 9 6 := ⟨fun i => if i.val = 3 then ⟨7, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_3_8 : ArrVertex 9 6 := ⟨fun i => if i.val = 3 then ⟨8, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_4_6 : ArrVertex 9 6 := ⟨fun i => if i.val = 4 then ⟨6, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_4_7 : ArrVertex 9 6 := ⟨fun i => if i.val = 4 then ⟨7, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_4_8 : ArrVertex 9 6 := ⟨fun i => if i.val = 4 then ⟨8, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_5_6 : ArrVertex 9 6 := ⟨fun i => if i.val = 5 then ⟨6, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_5_7 : ArrVertex 9 6 := ⟨fun i => if i.val = 5 then ⟨7, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩
def leaf_5_8 : ArrVertex 9 6 := ⟨fun i => if i.val = 5 then ⟨8, by decide⟩ else ⟨i.val, by omega⟩, by decide⟩

/-- The full Star in `A(9,6)` at `m=3`, all 6 branches complete: the center
    plus all 18 single-coordinate replacements by symbol 6, 7, or 8. -/
def full_star_9_6 : Finset (ArrVertex 9 6) :=
  {star_center_9_6,
   leaf_0_6, leaf_0_7, leaf_0_8,
   leaf_1_6, leaf_1_7, leaf_1_8,
   leaf_2_6, leaf_2_7, leaf_2_8,
   leaf_3_6, leaf_3_7, leaf_3_8,
   leaf_4_6, leaf_4_7, leaf_4_8,
   leaf_5_6, leaf_5_7, leaf_5_8}

theorem full_star_9_6_card : full_star_9_6.card = 19 := by decide

/-- **Mechanized refutation of `UniversalLowerBound`.** The full Star in
    `A(9,6)` at `R=19` has external boundary 180, strictly less than the
    formula's required 186 -- so no instance of `UniversalLowerBound` can
    be proved for `R=19, n=9, k=6`. This is a smaller sibling of the
    paper's `A(10,8), R=17` witness (Proposition 5.3), independently
    verified here by the Lean kernel/compiler rather than by the Python
    script or C++ search alone. -/
theorem full_star_9_6_refutes_universal_lower_bound :
    ¬ refuted.UniversalLowerBound 19 9 6 := by
  intro h
  have hbound := h full_star_9_6 full_star_9_6_card (by decide)
  have hviolate :
      ¬ (external_neighbors full_star_9_6
          ≥ (19 * 6 - E_seq 19) * (9 - 6) - C_constant 19) := by
    native_decide
  exact hviolate hbound

end Arrangement
