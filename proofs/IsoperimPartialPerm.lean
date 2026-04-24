/-
  IsoperimPartialPerm.lean
  ========================
  Isoperimetric Inequality for Partial Permutations (A(n,k))

  This module provides the sequence compression machinery needed to
  remove the two axioms in ArrangementExtraconnectivity.lean:
    1. external_neighbors_collision_bound
    2. hamming_ball_eval

  STATUS: SCAFFOLD — contains definitions and proof obligations as sorry.
  See docs/collision-axiom-roadmap.md for the full formalization roadmap.

  ARCHITECTURE:
  - Section 1: Sequence Shift (local, single-vertex operation)
  - Section 2: Set-Wise Compression (global, conditional on V')
  - Section 3: Compression preserves injectivity and cardinality
  - Section 4: Compression does not increase external boundary
  - Section 5: Colex ordering for ArrVertex and convergence
  - Section 6: The Isoperimetric Theorem (replaces both axioms)
-/

import Mathlib.Data.Nat.Basic
import Mathlib.Data.Finset.Basic
import Mathlib.Data.Finset.Card
import Mathlib.Data.Fintype.Pi
import Mathlib.Data.Fintype.Basic

set_option autoImplicit false

-- Re-use core types from the main file
-- (In a production build, these would be imported from a shared module)

/-- A vertex in A(n,k): an injective sequence of k symbols from {0..n-1} -/
def ArrVertex' (n k : ℕ) := { f : Fin k → Fin n // Function.Injective f }

/-!
  # Section 1: The Sequence Shift (Local Operation)

  Given two symbols a < b, shift a single vertex by replacing b with a
  at the position where b occurs (if a is not already used).
-/

/-- Shift a single vertex: replace symbol b with symbol a if valid.
    Returns the shifted vertex if:
    1. v uses symbol b at some position
    2. v does NOT use symbol a anywhere
    Otherwise returns v unchanged. -/
noncomputable def seq_shift {n k : ℕ} (a b : Fin n)
    (v : ArrVertex' n k) : ArrVertex' n k :=
  -- Check if v uses b but not a
  let uses_b := ∃ p : Fin k, v.val p = b
  let uses_a := ∃ p : Fin k, v.val p = a
  if _h : uses_b ∧ ¬uses_a then
    -- Replace b with a, preserving injectivity
    ⟨fun p => if v.val p = b then a else v.val p, by
      intro p1 p2 heq
      sorry -- Injectivity proof: case split on whether p1, p2 hit b
    ⟩
  else
    v

/-!
  # Section 2: Set-Wise Compression (Global Operation)

  The conditional compression: apply seq_shift to each vertex in V',
  but only if the target is not already occupied in V'.
  This is the arrangement graph analog of Mathlib's UV-compression.
-/

/-- Compress a subset V' by shifting symbol b → a, with collision guard.
    A vertex v is shifted only if seq_shift(v) ∉ V'. -/
noncomputable def seq_compress {n k : ℕ} (a b : Fin n)
    (V' : Finset (ArrVertex' n k)) : Finset (ArrVertex' n k) :=
  sorry -- The conditional set-wise map

/-!
  # Section 3: Compression Invariants
-/

/-- Compression preserves subset cardinality -/
theorem compress_card {n k : ℕ} (a b : Fin n) (V' : Finset (ArrVertex' n k)) :
    (seq_compress a b V').card = V'.card :=
  sorry

/-!
  # Section 4: Compression Does Not Increase Boundary

  The core extremal lemma: compressing V' toward smaller symbols
  can only increase root collisions, which reduces external neighbors.
-/

-- (external_neighbors and related definitions would be imported
--  from the main module in a production build)

/-!
  # Section 5: Colex Ordering and Convergence

  Define the colexicographic ordering on ArrVertex that enumerates
  the Hamming Ball construction. Prove that repeated compression
  converges to the Hamming Ball initial segment.
-/

/-!
  # Section 6: The Isoperimetric Theorem

  Composing Sections 3-5 replaces both axioms:
  - external_neighbors_collision_bound (the universal lower bound)
  - hamming_ball_eval (the Hamming Ball achieves the bound)
-/
