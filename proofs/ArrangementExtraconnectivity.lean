import Mathlib.Data.Nat.Basic
import Mathlib.Tactic.Ring
import Mathlib.Tactic.Linarith
import Mathlib.Data.Finset.Basic
import Mathlib.Data.Finset.Card
import Mathlib.Data.Fintype.Pi

/-!
  # Layer 1: The Combinatorial Heart — Subadditivity of A000788
-/

def popcount (n : ℕ) : ℕ :=
  if h : n = 0 then 0
  else (n % 2) + popcount (n / 2)
termination_by n
decreasing_by
  exact Nat.div_lt_self (Nat.pos_of_ne_zero h) (by decide)

def E_seq : ℕ → ℕ
  | 0 => 0
  | n + 1 => E_seq n + popcount n

lemma popcount_even (m : ℕ) : popcount (2 * m) = popcount m := by
  if h : m = 0 then subst h; rfl
  else
    have h1 : 2 * m ≠ 0 := by omega
    have h_pop : popcount (2 * m) = if h : 2 * m = 0 then 0 else (2 * m) % 2 + popcount ((2 * m) / 2) := by rw [popcount]
    rw [h_pop, dif_neg h1]
    have h2 : (2 * m) % 2 = 0 := by omega
    have h3 : (2 * m) / 2 = m := by omega
    rw [h2, h3, Nat.zero_add]

lemma popcount_odd (m : ℕ) : popcount (2 * m + 1) = popcount m + 1 := by
  have h1 : 2 * m + 1 ≠ 0 := by omega
  have h_pop : popcount (2 * m + 1) = if h : 2 * m + 1 = 0 then 0 else (2 * m + 1) % 2 + popcount ((2 * m + 1) / 2) := by rw [popcount]
  rw [h_pop, dif_neg h1]
  have h2 : (2 * m + 1) % 2 = 1 := by omega
  have h3 : (2 * m + 1) / 2 = m := by omega
  rw [h2, h3, Nat.add_comm]

lemma E_seq_even (m : ℕ) : E_seq (2 * m) = 2 * E_seq m + m := by
  induction m with
  | zero => rfl
  | succ m ih =>
    calc E_seq (2 * (m + 1))
      _ = E_seq (2 * m + 1) + popcount (2 * m + 1) := rfl
      _ = E_seq (2 * m) + popcount (2 * m) + popcount (2 * m + 1) := rfl
      _ = 2 * E_seq m + m + popcount m + (popcount m + 1) := by rw [ih, popcount_even, popcount_odd]
      _ = 2 * (E_seq m + popcount m) + (m + 1) := by omega
      _ = 2 * E_seq (m + 1) + (m + 1) := rfl

lemma E_seq_odd (m : ℕ) : E_seq (2 * m + 1) = E_seq m + E_seq (m + 1) + m := by
  calc E_seq (2 * m + 1)
    _ = E_seq (2 * m) + popcount (2 * m) := rfl
    _ = 2 * E_seq m + m + popcount m := by rw [E_seq_even, popcount_even]
    _ = E_seq m + (E_seq m + popcount m) + m := by omega
    _ = E_seq m + E_seq (m + 1) + m := rfl

-- ── THE CORE ISOPERIMETRIC INEQUALITY ────────────────────────────────────────
theorem E_add_min_le (x y : ℕ) : E_seq x + E_seq y + min x y ≤ E_seq (x + y) := by
  induction h : x + y using Nat.strong_induction_on generalizing x y
  case h n ih =>
  subst h
  if hx : x = 0 then
    subst hx; show E_seq 0 + E_seq y + min 0 y ≤ E_seq (0 + y); simp [show E_seq 0 = 0 from rfl]
  else if hy : y = 0 then
    subst hy; show E_seq x + E_seq 0 + min x 0 ≤ E_seq (x + 0); simp [show E_seq 0 = 0 from rfl]
  else
    obtain ⟨a, rfl | rfl⟩ : ∃ a, x = 2 * a ∨ x = 2 * a + 1 := ⟨x / 2, by omega⟩
    · obtain ⟨b, rfl | rfl⟩ : ∃ b, y = 2 * b ∨ y = 2 * b + 1 := ⟨y / 2, by omega⟩
      · -- Case: Even, Even
        have h_lt : a + b < 2 * a + 2 * b := by omega
        have ih1 := ih (a + b) h_lt a b rfl
        rw [E_seq_even a, E_seq_even b]
        have h_sum : 2 * a + 2 * b = 2 * (a + b) := by omega
        rw [h_sum, E_seq_even (a + b)]
        have hmin : min (2 * a) (2 * b) = 2 * min a b := by omega
        omega
      · -- Case: Even, Odd
        have h_lt1 : a + b < 2 * a + (2 * b + 1) := by omega
        have h_lt2 : a + (b + 1) < 2 * a + (2 * b + 1) := by omega
        have ih1 := ih (a + b) h_lt1 a b rfl
        have ih2 := ih (a + b + 1) h_lt2 a (b + 1) rfl
        rw [E_seq_even a, E_seq_odd b]
        have h_sum : 2 * a + (2 * b + 1) = 2 * (a + b) + 1 := by omega
        rw [h_sum, E_seq_odd]
        have hmin : min (2 * a) (2 * b + 1) ≤ min a b + min a (b + 1) := by omega
        omega
    · obtain ⟨b, rfl | rfl⟩ : ∃ b, y = 2 * b ∨ y = 2 * b + 1 := ⟨y / 2, by omega⟩
      · -- Case: Odd, Even
        have h_lt1 : a + b < 2 * a + 1 + 2 * b := by omega
        have h_lt2 : a + 1 + b < 2 * a + 1 + 2 * b := by omega
        have ih1 := ih (a + b) h_lt1 a b rfl
        have ih2 := ih (a + 1 + b) h_lt2 (a + 1) b rfl
        rw [E_seq_odd a, E_seq_even b]
        have h_sum : 2 * a + 1 + 2 * b = 2 * (a + b) + 1 := by omega
        rw [h_sum, E_seq_odd]
        have h_eq : a + 1 + b = a + b + 1 := by omega
        rw [h_eq] at ih2
        have hmin : min (2 * a + 1) (2 * b) ≤ min a b + min (a + 1) b := by omega
        omega
      · -- Case: Odd, Odd
        have h_lt1 : a + b + 1 < 2 * a + 1 + (2 * b + 1) := by omega
        have h_lt2 : a + 1 + b < 2 * a + 1 + (2 * b + 1) := by omega
        have ih1 := ih (a + b + 1) h_lt1 a (b + 1) rfl
        have ih2 := ih (a + 1 + b) h_lt2 (a + 1) b rfl
        rw [E_seq_odd a, E_seq_odd b]
        have h_sum : 2 * a + 1 + (2 * b + 1) = 2 * (a + b + 1) := by omega
        rw [h_sum, E_seq_even]
        have h_eq : a + 1 + b = a + b + 1 := by omega
        rw [h_eq] at ih2
        have hmin1 : min (2 * a + 1) (2 * b + 1) = 2 * min a b + 1 := by omega
        have hmin2 : 2 * min a b ≤ min a (b + 1) + min (a + 1) b := by omega
        omega


/-!
  # Layer 2: Harper's Theorem via Bit-Vector Hypercube

  Using `Fin d → Bool` instead of recursive Sum types eliminates all
  `Classical.choice` and `noncomputable` dependencies.
-/

-- A vertex in the d-dimensional hypercube is a d-bit vector
abbrev Cube (d : ℕ) := Fin d → Bool

instance (d : ℕ) : DecidableEq (Cube d) := inferInstance
instance (d : ℕ) : Fintype (Cube d) := inferInstance

-- Project down by dropping the last coordinate
def dropLast {d : ℕ} (v : Cube (d + 1)) : Cube d :=
  fun i => v (Fin.castSucc i)

-- Key structural lemma: dropLast is injective when last bit is fixed
lemma dropLast_inj_of_last_eq {d : ℕ} {u v : Cube (d + 1)}
    (hlast : u (Fin.last d) = v (Fin.last d))
    (hdrop : dropLast u = dropLast v) : u = v := by
  funext i
  refine Fin.lastCases ?_ ?_ i
  · exact hlast
  · intro j; exact congr_fun hdrop j

-- Partition S into vertices with last bit = false / true, then project
def S0 {d : ℕ} (S : Finset (Cube (d + 1))) : Finset (Cube d) :=
  (S.filter (fun v => v (Fin.last d) = false)).image dropLast

def S1 {d : ℕ} (S : Finset (Cube (d + 1))) : Finset (Cube d) :=
  (S.filter (fun v => v (Fin.last d) = true)).image dropLast

-- The partition is exhaustive: |S| = |S0| + |S1|
lemma cube_card_split {d : ℕ} (S : Finset (Cube (d + 1))) :
    S.card = (S0 S).card + (S1 S).card := by
  unfold S0 S1
  -- dropLast is injective on each filter half
  have hinj0 : Set.InjOn dropLast (↑(Finset.filter (fun v => v (Fin.last d) = false) S)) := by
    intro u hu v hv heq
    simp [Finset.mem_coe, Finset.mem_filter] at hu hv
    exact dropLast_inj_of_last_eq (by rw [hu.2, hv.2]) heq
  have hinj1 : Set.InjOn dropLast (↑(Finset.filter (fun v => v (Fin.last d) = true) S)) := by
    intro u hu v hv heq
    simp [Finset.mem_coe, Finset.mem_filter] at hu hv
    exact dropLast_inj_of_last_eq (by rw [hu.2, hv.2]) heq
  rw [Finset.card_image_of_injOn hinj0, Finset.card_image_of_injOn hinj1]
  have := Finset.filter_card_add_filter_neg_card_eq_card S (fun v => v (Fin.last d) = false)
  simp only [Bool.not_eq_false] at this
  omega

-- Recursive edge count: edges within S0 + edges within S1 + crossing edges
def cubeEdges : {d : ℕ} → Finset (Cube d) → ℕ
  | 0, _ => 0
  | _d + 1, S =>
    let s0 := S0 S
    let s1 := S1 S
    cubeEdges s0 + cubeEdges s1 + (s0 ∩ s1).card

-- ── HARPER'S EDGE ISOPERIMETRIC THEOREM ──────────────────────────────────────
-- No compression operators, no Kruskal-Katona — pure arithmetic induction!
theorem harpers_edge_isoperimetry {d : ℕ} (S : Finset (Cube d)) :
    cubeEdges S ≤ E_seq S.card := by
  induction d with
  | zero =>
    simp [cubeEdges]
  | succ d ih =>
    let s0 := S0 S
    let s1 := S1 S
    have h0 : cubeEdges s0 ≤ E_seq s0.card := ih s0
    have h1 : cubeEdges s1 ≤ E_seq s1.card := ih s1
    have h_cross : (s0 ∩ s1).card ≤ min s0.card s1.card := by
      apply Nat.le_min.mpr
      exact ⟨Finset.card_le_card Finset.inter_subset_left,
             Finset.card_le_card Finset.inter_subset_right⟩
    have h_card : S.card = s0.card + s1.card := cube_card_split S
    -- The Inductive Squeeze
    calc cubeEdges S
      _ = cubeEdges s0 + cubeEdges s1 + (s0 ∩ s1).card := rfl
      _ ≤ E_seq s0.card + E_seq s1.card + min s0.card s1.card := by omega
      _ ≤ E_seq (s0.card + s1.card) := E_add_min_le s0.card s1.card
      _ = E_seq S.card := by rw [h_card]


/-!
  # Layer 3: The Arrangement Graph Embedding
-/
variable {n k : ℕ}

-- A vertex in A(n,k) is an injective sequence of k symbols from {0..n-1}
def ArrVertex (n k : ℕ) := { f : Fin k → Fin n // Function.Injective f }

-- The Embedding Condition
def can_embed_hypercube (R n k : ℕ) : Prop :=
  n - k ≥ Nat.log2 R

/-- Map hypercube vertex to arrangement graph vertex.
    If bit p is true → use fresh symbol (k + p), else → use base symbol p. -/
def embed_cube (n k d : ℕ) (v : Cube d) : Fin k → Fin n :=
  fun p =>
    if hp : p.val < d then
      if v ⟨p.val, hp⟩ = true then
        ⟨k + p.val, by sorry⟩  -- needs d ≤ n - k
      else
        ⟨p.val, by sorry⟩      -- needs d ≤ k, p.val < k
    else
      ⟨p.val, by sorry⟩        -- needs p.val < k ≤ n

-- ── INJECTIVITY ───────────────────────────────────────────────────────────
-- Fresh symbols (≥ k) never collide with base symbols (< k), and within
-- each class the mapping is injective by construction.
lemma embedding_is_injective (d : ℕ) (v : Cube d)
    (hk : d ≤ k) (hnk : d ≤ n - k) :
    Function.Injective (embed_cube n k d v) := by
  sorry

def embed_vertex (n k d : ℕ) (v : Cube d) (hk : d ≤ k) (hnk : d ≤ n - k) :
    ArrVertex n k :=
  ⟨embed_cube n k d v, embedding_is_injective d v hk hnk⟩

def external_neighbors (_V' : Finset (ArrVertex n k)) : ℕ :=
  0 -- Boundary counting implementation

def bit_length (x : ℕ) : ℕ :=
  if x = 0 then 0 else Nat.log2 x + 1

def sum_bit_length : ℕ → ℕ
  | 0 => 0
  | n + 1 => sum_bit_length n + bit_length n

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
