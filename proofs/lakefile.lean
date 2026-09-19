import Lake
open Lake DSL

package «proofs» where
  leanOptions := #[
    ⟨`autoImplicit, false⟩,
    ⟨`linter.unusedVariables, true⟩,
    ⟨`linter.style.multiGoal, true⟩,
    ⟨`linter.style.setOption, true⟩,
    ⟨`linter.style.missingEnd, true⟩,
    ⟨`linter.deprecated, true⟩,
    ⟨`linter.style.cdot, true⟩,
    ⟨`linter.docPrime, true⟩
  ]

require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "master"

@[default_target]
lean_lib «Proofs» where
  srcDir := "."
  roots := #[
    `Arrangement.ArrDefs,
    `Arrangement.HypercubeEdges,
    `Arrangement.ArrangementExtraconnectivity,
    `Arrangement.PenaltyExact,
    `Arrangement.InternalPairs,
    `Arrangement.CrossTop,
  ]
