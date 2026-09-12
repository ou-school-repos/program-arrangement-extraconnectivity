import Arrangement.CrossTop

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
assert_no_sorry Arrangement.arrangement_extraconnectivity_minimum
assert_no_sorry Arrangement.globally_optimal_growth_strategy
