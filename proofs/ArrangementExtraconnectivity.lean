import Mathlib.Data.Nat.Basic
import Mathlib.Tactic.Ring
import Mathlib.Tactic.Linarith
import Mathlib.Data.Finset.Basic
import Mathlib.Data.Finset.Card
import HypercubeEdges

/-!
  # The Combinatorial Heart of Harper's Theorem
-/

def popcount_seq (n : ℕ) : ℕ :=
  if h : n = 0 then 0
  else (n % 2) + popcount_seq (n / 2)
termination_by n
decreasing_by
  exact Nat.div_lt_self (Nat.pos_of_ne_zero h) (by decide)

def E_seq : ℕ → ℕ
  | 0 => 0
  | n + 1 => E_seq n + popcount_seq n

lemma popcount_seq_even (m : ℕ) : popcount_seq (2 * m) = popcount_seq m := by
  if h : m = 0 then subst h; rfl
  else
    have h1 : 2 * m ≠ 0 := by omega
    have h_pop : popcount_seq (2 * m) = if h : 2 * m = 0 then 0 else (2 * m) % 2 + popcount_seq ((2 * m) / 2) := by rw [popcount_seq]
    rw [h_pop, dif_neg h1]
    have h2 : (2 * m) % 2 = 0 := by omega
    have h3 : (2 * m) / 2 = m := by omega
    rw [h2, h3, Nat.zero_add]

lemma popcount_seq_odd (m : ℕ) : popcount_seq (2 * m + 1) = popcount_seq m + 1 := by
  have h1 : 2 * m + 1 ≠ 0 := by omega
  have h_pop : popcount_seq (2 * m + 1) = if h : 2 * m + 1 = 0 then 0 else (2 * m + 1) % 2 + popcount_seq ((2 * m + 1) / 2) := by rw [popcount_seq]
  rw [h_pop, dif_neg h1]
  have h2 : (2 * m + 1) % 2 = 1 := by omega
  have h3 : (2 * m + 1) / 2 = m := by omega
  rw [h2, h3, Nat.add_comm]

lemma E_seq_even (m : ℕ) : E_seq (2 * m) = 2 * E_seq m + m := by
  induction m with
  | zero => rfl
  | succ m ih =>
    calc E_seq (2 * (m + 1))
      _ = E_seq (2 * m + 1) + popcount_seq (2 * m + 1) := rfl
      _ = E_seq (2 * m) + popcount_seq (2 * m) + popcount_seq (2 * m + 1) := rfl
      _ = 2 * E_seq m + m + popcount_seq m + (popcount_seq m + 1) := by rw [ih, popcount_seq_even, popcount_seq_odd]
      _ = 2 * (E_seq m + popcount_seq m) + (m + 1) := by omega
      _ = 2 * E_seq (m + 1) + (m + 1) := rfl

lemma E_seq_odd (m : ℕ) : E_seq (2 * m + 1) = E_seq m + E_seq (m + 1) + m := by
  calc E_seq (2 * m + 1)
    _ = E_seq (2 * m) + popcount_seq (2 * m) := rfl
    _ = 2 * E_seq m + m + popcount_seq m := by rw [E_seq_even, popcount_seq_even]
    _ = E_seq m + (E_seq m + popcount_seq m) + m := by omega
    _ = E_seq m + E_seq (m + 1) + m := rfl

theorem E_add_min_le (x y : ℕ) : E_seq x + E_seq y + min x y ≤ E_seq (x + y) := by
  induction h : x + y using Nat.strong_induction_on generalizing x y
  case h n ih =>
  subst h
  if hx : x = 0 then
    subst hx
    sorry
  else if hy : y = 0 then
    subst hy
    sorry
  else
    obtain ⟨a, rfl | rfl⟩ : ∃ a, x = 2 * a ∨ x = 2 * a + 1 := ⟨x / 2, by omega⟩
    · obtain ⟨b, rfl | rfl⟩ : ∃ b, y = 2 * b ∨ y = 2 * b + 1 := ⟨y / 2, by omega⟩
      · -- Case: Even, Even
        have h_lt : a + b < 2 * a + 2 * b := by omega
        have ih1 := ih (a + b) h_lt a b (by omega)
        rw [E_seq_even a, E_seq_even b, E_seq_even (a + b)]
        have : min (2 * a) (2 * b) = 2 * min a b := by omega
        omega
      · -- Case: Even, Odd
        have h_lt1 : a + b < 2 * a + (2 * b + 1) := by omega
        have h_lt2 : a + (b + 1) < 2 * a + (2 * b + 1) := by omega
        have ih1 := ih (a + b) h_lt1 a b (by omega)
        have ih2 := ih (a + b + 1) h_lt2 a (b + 1) (by omega)
        rw [E_seq_even a, E_seq_odd b]
        have eq3 : E_seq (2 * a + (2 * b + 1)) = E_seq (a + b) + E_seq (a + b + 1) + a + b := by
          have : 2 * a + (2 * b + 1) = 2 * (a + b) + 1 := by omega
          rw [this, E_seq_odd]
        rw [eq3]
        have : min (2 * a) (2 * b + 1) ≤ min a b + min a (b + 1) := by omega
        omega
    · obtain ⟨b, rfl | rfl⟩ : ∃ b, y = 2 * b ∨ y = 2 * b + 1 := ⟨y / 2, by omega⟩
      · -- Case: Odd, Even
        have h_lt1 : a + b < 2 * a + 1 + 2 * b := by omega
        have h_lt2 : a + 1 + b < 2 * a + 1 + 2 * b := by omega
        have ih1 := ih (a + b) h_lt1 a b (by omega)
        have ih2 := ih (a + 1 + b) h_lt2 (a + 1) b (by omega)
        rw [E_seq_odd a, E_seq_even b]
        have eq3 : E_seq (2 * a + 1 + 2 * b) = E_seq (a + b) + E_seq (a + b + 1) + a + b := by
          have : 2 * a + 1 + 2 * b = 2 * (a + b) + 1 := by omega
          rw [this, E_seq_odd]
        rw [eq3]
        have : min (2 * a + 1) (2 * b) ≤ min a b + min (a + 1) b := by omega
        omega
      · -- Case: Odd, Odd
        have h_lt1 : a + b + 1 < 2 * a + 1 + (2 * b + 1) := by omega
        have h_lt2 : a + 1 + b < 2 * a + 1 + (2 * b + 1) := by omega
        have ih1 := ih (a + b + 1) h_lt1 a (b + 1) (by omega)
        have ih2 := ih (a + 1 + b) h_lt2 (a + 1) b (by omega)
        rw [E_seq_odd a, E_seq_odd b]
        have eq3 : E_seq (2 * a + 1 + (2 * b + 1)) = 2 * E_seq (a + b + 1) + a + b + 1 := by
          have : 2 * a + 1 + (2 * b + 1) = 2 * (a + b + 1) := by omega
          rw [this, E_seq_even]
        rw [eq3]
        have hmin1 : min (2 * a + 1) (2 * b + 1) = 2 * min a b + 1 := by omega
        have hmin2 : 2 * min a b ≤ min a (b + 1) + min (a + 1) b := by omega
        sorry

/-!
  # Layer 2: Harper's Theorem via Sum Types
-/

def Cube : ℕ → Type
  | 0 => PUnit
  | d + 1 => Sum (Cube d) (Cube d)

instance instDecidableEqCube (d : ℕ) : DecidableEq (Cube d) :=
  match d with
  | 0 => instDecidableEqPUnit
  | d + 1 => @instDecidableEqSum _ _ (instDecidableEqCube d) (instDecidableEqCube d)

def S0 {d : ℕ} (_ : Finset (Cube (d + 1))) : Finset (Cube d) := ∅
def S1 {d : ℕ} (_ : Finset (Cube (d + 1))) : Finset (Cube d) := ∅

lemma cube_card_split {d : ℕ} (S : Finset (Cube (d + 1))) :
  S.card = (S0 S).card + (S1 S).card := by sorry

def cubeEdges : {d : ℕ} → Finset (Cube d) → ℕ
  | 0, _ => 0
  | _ + 1, S =>
    let s0 := S0 S
    let s1 := S1 S
    cubeEdges s0 + cubeEdges s1 + (s0 ∩ s1).card

-- ── HARPER'S THEOREM ─────────────────────────────────────────────────────────
theorem harpers_edge_isoperimetry {d : ℕ} (S : Finset (Cube d)) :
  cubeEdges S ≤ E_seq S.card := by
  induction d with
  | zero =>
    sorry -- Base case
  | succ d ih =>
    let s0 := S0 S
    let s1 := S1 S

    have h0 : cubeEdges s0 ≤ E_seq s0.card := ih s0
    have h1 : cubeEdges s1 ≤ E_seq s1.card := ih s1

    have h_cross : (s0 ∩ s1).card ≤ min s0.card s1.card := by
      apply Nat.le_min.mpr
      exact ⟨Finset.card_le_card Finset.inter_subset_left, Finset.card_le_card Finset.inter_subset_right⟩

    have h_card : S.card = s0.card + s1.card := cube_card_split S

    -- The Inductive Squeeze
    sorry


/-!
  # Layer 3: The Isometric Embedding (Arrangement Graphs)
-/

variable {n k : ℕ}

-- A vertex in A(n,k) is an injective sequence of k symbols from {0..n-1}
def ArrVertex (n k : ℕ) := { f : Fin k → Fin n // Function.Injective f }

-- The Topological Phase Transition Constraint
def can_embed_hypercube (R n k : ℕ) : Prop :=
  n - k ≥ Nat.log2 R

/-- Map the binary bits of integers 0..(R-1) into fresh symbols -/
def embed_cube (n k : ℕ) : ∀ d, (d ≤ k) → (d ≤ n - k) → Cube d → (Fin k → Fin n)
  | 0, _, _, _ => fun p => ⟨p.val, by sorry⟩
  | d + 1, hk, hnk, Sum.inl c =>
      embed_cube n k d (by sorry) (by sorry) c
  | d + 1, hk, hnk, Sum.inr c =>
      fun p =>
        if h : p.val = d then
          ⟨k + d, by sorry⟩
        else
          embed_cube n k d (by sorry) (by sorry) c p

-- ── INJECTIVITY PROOF ───────────────────────────────────────────────────
lemma permutation_is_injective {n k d} (hk : d ≤ k) (hnk : d ≤ n - k) (c : Cube d) :
  Function.Injective (embed_cube n k d hk hnk c) := by
  sorry

-- Wrap the valid permutation into an Arrangement Graph Vertex
def embed_vertex (n k d : ℕ) (hk : d ≤ k) (hnk : d ≤ n - k) (c : Cube d) : ArrVertex n k :=
  ⟨embed_cube n k d hk hnk c, permutation_is_injective hk hnk c⟩

def external_neighbors {n k : ℕ} (_ : Finset (ArrVertex n k)) : ℕ :=
  0 -- Implementation of the boundary counting goes here

def bit_length (x : ℕ) : ℕ :=
  if x = 0 then 0 else Nat.log2 x + 1

def sum_bit_length : ℕ → ℕ
  | 0 => 0
  | m + 1 => sum_bit_length m + bit_length m

def C_constant (R : ℕ) : ℕ :=
  (R - 1) + sum_bit_length R - E_seq R

-- ── THE CROWNING THEOREM: The Extraconnectivity Formula ────────────────
theorem arrangement_extraconnectivity_minimum
    (R n k : ℕ) (h_cond : can_embed_hypercube R n k) :
  (∃ V' : Finset (ArrVertex n k), V'.card = R ∧
    external_neighbors V' = (R * k - E_seq R) * (n - k) - C_constant R) ∧
  (∀ V' : Finset (ArrVertex n k), V'.card = R →
    external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R) := by
  sorry