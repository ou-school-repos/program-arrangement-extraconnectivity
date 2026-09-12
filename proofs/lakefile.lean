import Lake
open Lake DSL

package «proofs» where
  leanOptions := #[⟨`autoImplicit, false⟩]

require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "master"

@[default_target]
lean_lib «Proofs» where
  srcDir := "."
  roots := #[
    `Arrangement.ArrDefs,
    `Arrangement.HypercubeEdges,
    `Arrangement.PredictorComplexity,
    `Arrangement.ArrangementExtraconnectivity,
    `Arrangement.PenaltyExact,
    `Arrangement.unstable.IsoperimetricPartialPermutation,
    `Arrangement.unstable.ArrangementGraphUtils,
    `Arrangement.unstable.SupportProjection,
    `Arrangement.unstable.CrossCollisionsResearch,
    `Arrangement.unstable.CrossRecurrenceDriver,
    `Arrangement.CrossTop
  ]
