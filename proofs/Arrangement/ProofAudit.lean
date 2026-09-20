import Arrangement.EventualProfile

/-!
# Public theorem trust audit

This file checks the elaborated proof terms of the public capstone theorems.
Unlike a source scan, it follows imported declarations transitively; unlike a
build-log scan, it is independent of Lake's incremental-build behaviour.
-/

open Lean Elab Command

/-- Fail if a declaration's transitive proof term depends on Lean's `sorryAx`. -/
elab "assert_no_sorry " id:ident : command => do
  let names ← liftCoreM <| realizeGlobalConstWithInfos id
  for name in names do
    let axioms ← collectAxioms name
    if axioms.contains ``sorryAx then
      throwError "{name} depends on sorryAx"

assert_no_sorry Arrangement.hb_cross_collisions_closed
assert_no_sorry Arrangement.arrangement_boundary_sandwich
assert_no_sorry Arrangement.hamming_linear_le_profile_add_error
assert_no_sorry Arrangement.hamming_witness_roots
assert_no_sorry Arrangement.eventual_max_defect
assert_no_sorry Arrangement.eventual_minimizer_max_collision
assert_no_sorry Arrangement.eventual_profile_formula
assert_no_sorry Arrangement.eventual_hamming_exact_iff
assert_no_sorry external_neighbors_lower_bound_global
