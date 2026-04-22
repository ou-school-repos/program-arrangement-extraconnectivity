import Lake
open Lake DSL

package «proofs» where
  leanOptions := #[⟨`autoImplicit, false⟩]

require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "master"

@[default_target]
lean_lib «Proofs» where
  srcDir := "."
  roots := #[`HypercubeEdges, `PredictorComplexity]
