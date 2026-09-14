/-
  CapacityBoundExamples.lean
  ==========================
  Mechanized witnesses for the two "channels" identified in
  `docs/proof-sketch-weighted-potential.md` (Strategy 3b, "The open
  problem, restated as a capacity/pigeonhole aggregation bound") and
  `paper/sections/12_appendix_alternative_strategies.tex`.

  This file does NOT attempt the open aggregation bound itself. It
  formalizes and mechanically checks the two *concrete* examples that
  were hand-verified in Python during that research session, upgrading
  them from "re-checked twice by script" to "checked by the Lean
  kernel/compiler." Nothing here is a step toward closing the bound;
  it is a durable record of the two facts the open question has to be
  consistent with.

  Channel 1 (distance-1, root-sharing): two vertices sharing a root at
  the partition coordinate p contribute m+1-|{u_p,v_p}| shared external
  neighbors — nonzero, and linear in m. Witnessed in A(6,3): m=3,
  {(0,1,2)} and {(3,1,2)} share exactly 2 = m-1 external neighbors.

  Channel 2 (distance-2, midpoint): for u,v differing at p and one
  other coordinate q, the two candidate cross-vertices are gated by
  *independent* conditions (β≠u_q, α≠v_q), not a single combined one.
  Witnessed in A(5,3): u=(0,1,3), v=(1,2,3) has β=1=u_q (first
  candidate invalid) but α=0≠v_q=2 (second valid) — exactly 1 shared
  neighbor, not 0 or 2.
-/

import Arrangement.ArrDefs

set_option autoImplicit false

/-- The external neighbors shared by two (not necessarily disjoint)
    subsets of `A(n,k)`: vertices outside both `Fa` and `Fb` that are
    adjacent to some member of `Fa` *and* to some member of `Fb`.
    This is `∂Fa ∩ ∂Fb` under the same adjacency `external_neighbors`
    already uses, restricted to the two-set case the recombination
    step in the doc/appendix is about. -/
def shared_boundary {n k : ℕ} (Fa Fb : Finset (ArrVertex n k)) :
    Finset (ArrVertex n k) :=
  Finset.univ.filter (fun w =>
    w ∉ Fa ∧ w ∉ Fb ∧
    (∃ u ∈ Fa, arr_adjacent u w) ∧ (∃ v ∈ Fb, arr_adjacent v w))

section Channel1

/-- `A(6,3)`: u = (0,1,2). -/
def c1_u : ArrVertex 6 3 :=
  ⟨fun i => if i.val = 0 then 0 else if i.val = 1 then 1 else 2, by decide⟩

/-- `A(6,3)`: v = (3,1,2), agreeing with `u` everywhere except
    coordinate 0 — a distance-1 (adjacent) pair sharing a root there. -/
def c1_v : ArrVertex 6 3 :=
  ⟨fun i => if i.val = 0 then 3 else if i.val = 1 then 1 else 2, by decide⟩

example : arr_adjacent c1_u c1_v := by decide

/-- Channel 1, mechanically checked: a distance-1 pair sharing a root
    contributes exactly `m - 1 = 2` shared external neighbors in
    `A(6,3)` (`m = n - k = 3`), matching the closed form
    `m + 1 - |{u_p, v_p}| = 4 - 2 = 2` derived by hand in the doc. -/
example :
    (shared_boundary ({c1_u} : Finset (ArrVertex 6 3)) {c1_v}).card = 2 := by
  native_decide

end Channel1

section Channel2

/-- `A(5,3)`: u = (0,1,3). -/
def c2_u : ArrVertex 5 3 :=
  ⟨fun i => if i.val = 0 then 0 else if i.val = 1 then 1 else 3, by decide⟩

/-- `A(5,3)`: v = (1,2,3), differing from `u` at coordinates 0 and 1
    (a distance-2 pair) with β = v_0 = 1 = u_1 = u_q, so the first
    candidate cross-vertex is invalid, while α = u_0 = 0 ≠ v_1 = 2, so
    the second is valid — the two gates firing independently. -/
def c2_v : ArrVertex 5 3 :=
  ⟨fun i => if i.val = 0 then 1 else if i.val = 1 then 2 else 3, by decide⟩

example :
    (Finset.univ.filter
      (fun p : Fin 3 => c2_u.val p ≠ c2_v.val p)).card = 2 := by decide

/-- Channel 2, mechanically checked: this distance-2 pair contributes
    exactly 1 shared external neighbor, not 0 or 2 — confirming the two
    existence conditions (`β ≠ u_q`, `α ≠ v_q`) gate independently
    rather than as one combined check. -/
example :
    (shared_boundary ({c2_u} : Finset (ArrVertex 5 3)) {c2_v}).card = 1 := by
  native_decide

end Channel2
