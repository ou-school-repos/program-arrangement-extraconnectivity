/-
  ArrDefs.lean
  ============
  Shared definitions for Arrangement Graph A(n,k).

  This module defines the core types and operations used by other files.
-/

import Mathlib.Data.Nat.Basic
import Mathlib.Data.Finset.Basic
import Mathlib.Data.Finset.Card
import Mathlib.Data.Fintype.Pi
import Mathlib.Data.Fintype.Basic

set_option autoImplicit false

variable {n k : ℕ}

-- A vertex in A(n,k) is an injective sequence of k symbols from {0..n-1}
def ArrVertex (n k : ℕ) := { f : Fin k → Fin n // Function.Injective f }

-- Provide Fintype and DecidableEq for ArrVertex (injective functions)
instance {n k : ℕ} : DecidableEq (ArrVertex n k) := by
  unfold ArrVertex; infer_instance

instance {n k : ℕ} : Fintype (ArrVertex n k) := by
  unfold ArrVertex; infer_instance

-- ADJACENCY

/-- Two vertices in A(n,k) are adjacent if they differ in exactly one position. -/
def arr_adjacent {n k : ℕ} (u v : ArrVertex n k) : Prop :=
  (Finset.univ.filter (fun p : Fin k => u.val p ≠ v.val p)).card = 1

instance {n k : ℕ} (u v : ArrVertex n k) : Decidable (arr_adjacent u v) := by
  unfold arr_adjacent; infer_instance

-- EXTERNAL NEIGHBORS (computable)

/-- Computable definition of the external boundary.
    Counts vertices outside V' that are adjacent to at least one member of V'. -/
def external_neighbors {n k : ℕ} (V' : Finset (ArrVertex n k)) : ℕ :=
  (Finset.univ.filter (fun v => v ∉ V' ∧ ∃ u ∈ V', arr_adjacent u v)).card
