import Lake
open Lake DSL

package «proofs» where
  leanOptions := #[⟨`autoImplicit, false⟩]

require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "master"

meta if get_config? env = some "dev" then
  require «doc-gen4» from git "https://github.com/leanprover/doc-gen4" @ "main"

@[default_target]
lean_lib «Proofs» where
  srcDir := "."
  roots := #[`HypercubeEdges]
