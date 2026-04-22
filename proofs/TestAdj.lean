import Mathlib.Data.Finset.Basic
import Mathlib.Combinatorics.SimpleGraph.Basic
import Mathlib.Data.Nat.Log
import Mathlib.Algebra.BigOperators.Group.Finset.Basic
import Mathlib.Combinatorics.SetFamily.KruskalKatona
import HypercubeEdges

open Classical

variable {d : ℕ}

def CubeVertex (d : ℕ) := Fin d → Bool
def CubeAdj (u v : CubeVertex d) : Prop := ∃! i, u i ≠ v i

def cubeEquivFinset (d : ℕ) : CubeVertex d ≃ Finset (Fin d) where
  toFun v := Finset.filter (fun i => v i) Finset.univ
  invFun s i := i ∈ s
  left_inv v := funext (fun i => by simp)
  right_inv s := by ext i; simp

lemma cubeAdj_iff_symmDiff_card_one (u v : CubeVertex d) :
  CubeAdj u v ↔ (cubeEquivFinset d u ∆ cubeEquivFinset d v).card = 1 := by
  sorry
