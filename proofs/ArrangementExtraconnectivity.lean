import Mathlib.Data.Nat.Basic
import Mathlib.Tactic.Ring
import Mathlib.Tactic.Linarith
import Mathlib.Data.Finset.Basic
import Mathlib.Data.Finset.Card
import Mathlib.Data.Fintype.Pi
import Mathlib.Algebra.BigOperators.Group.Finset.Basic
import Mathlib.Data.Fintype.Basic

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
  let sf := S.filter (fun v => v (Fin.last d) = false)
  let st := S.filter (fun v => v (Fin.last d) = true)
  change S.card = (sf.image dropLast).card + (st.image dropLast).card
  have hinj0 : Set.InjOn dropLast (sf : Set (Cube (d + 1))) := by
    intro u hu v hv heq
    have ⟨_, hu2⟩ := Finset.mem_filter.mp (Finset.mem_coe.mp hu)
    have ⟨_, hv2⟩ := Finset.mem_filter.mp (Finset.mem_coe.mp hv)
    exact dropLast_inj_of_last_eq (by rw [hu2, hv2]) heq
  have hinj1 : Set.InjOn dropLast (st : Set (Cube (d + 1))) := by
    intro u hu v hv heq
    have ⟨_, hu2⟩ := Finset.mem_filter.mp (Finset.mem_coe.mp hu)
    have ⟨_, hv2⟩ := Finset.mem_filter.mp (Finset.mem_coe.mp hv)
    exact dropLast_inj_of_last_eq (by rw [hu2, hv2]) heq
  rw [Finset.card_image_of_injOn hinj0, Finset.card_image_of_injOn hinj1]
  have h1 := S.card_filter_add_card_filter_not (fun v => v (Fin.last d) = false)
  have h2 : (S.filter (fun a => ¬a (Fin.last d) = false)) = st := by
    ext v; constructor
    · intro hv; rw [Finset.mem_filter] at hv ⊢
      exact ⟨hv.1, by rcases Bool.eq_false_or_eq_true (v (Fin.last d)) with h | h <;> simp_all⟩
    · intro hv; rw [Finset.mem_filter] at hv ⊢
      exact ⟨hv.1, by rw [hv.2]; decide⟩
  rw [h2] at h1; linarith

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

-- Provide Fintype and DecidableEq for ArrVertex (injective functions)
instance {n k : ℕ} : DecidableEq (ArrVertex n k) := by
  unfold ArrVertex; infer_instance

instance {n k : ℕ} : Fintype (ArrVertex n k) := by
  unfold ArrVertex; infer_instance

-- The Embedding Condition
def can_embed_hypercube (R n k : ℕ) : Prop :=
  n - k ≥ Nat.log2 R

/-- Map hypercube vertex to arrangement graph vertex.
    If bit p is true → use fresh symbol (k + p), else → use base symbol p. -/
def embed_cube (n k d : ℕ) (_hk : d ≤ k) (hnk : k + d ≤ n) (v : Cube d) : Fin k → Fin n :=
  fun p =>
    if hp : p.val < d then
      if v ⟨p.val, hp⟩ = true then
        ⟨k + p.val, by omega⟩
      else
        ⟨p.val, by omega⟩
    else
      ⟨p.val, by omega⟩

-- ── INJECTIVITY ───────────────────────────────────────────────────────────
-- Fresh symbols (≥ k) never collide with base symbols (< k), and within
-- each class the mapping is injective by construction.
lemma embedding_is_injective (d : ℕ) (v : Cube d)
    (hk : d ≤ k) (hnk : k + d ≤ n) :
    Function.Injective (embed_cube n k d hk hnk v) := by
  intro p1 p2 heq
  ext
  simp only [embed_cube] at heq
  -- Extract the Fin.val equality from the Fin equality
  have hval := Fin.val_eq_of_eq heq
  simp at hval
  -- Case split on whether each position is in the flipped range
  by_cases h1 : p1.val < d <;> by_cases h2 : p2.val < d <;> simp [h1, h2] at hval
  · -- Both in range: split on bit values
    by_cases hv1 : v ⟨p1.val, h1⟩ = true <;> by_cases hv2 : v ⟨p2.val, h2⟩ = true <;>
      simp [hv1, hv2] at hval <;> omega
  · -- p1 in range, p2 out: fresh vs base collision impossible
    by_cases hv1 : v ⟨p1.val, h1⟩ = true <;> simp [hv1] at hval <;> omega
  · -- p1 out, p2 in range: same
    by_cases hv2 : v ⟨p2.val, h2⟩ = true <;> simp [hv2] at hval <;> omega
  · -- Both out of range: identity
    omega

def embed_vertex (n k d : ℕ) (v : Cube d) (hk : d ≤ k) (hnk : k + d ≤ n) :
    ArrVertex n k :=
  ⟨embed_cube n k d hk hnk v, embedding_is_injective d v hk hnk⟩

-- ── ADJACENCY ─────────────────────────────────────────────────────────────

/-- Two vertices in A(n,k) are adjacent if they differ in exactly one position. -/
def arr_adjacent {n k : ℕ} (u v : ArrVertex n k) : Prop :=
  (Finset.univ.filter (fun p : Fin k => u.val p ≠ v.val p)).card = 1

instance {n k : ℕ} (u v : ArrVertex n k) : Decidable (arr_adjacent u v) := by
  unfold arr_adjacent; infer_instance

-- ── EXTERNAL NEIGHBORS (computable) ───────────────────────────────────────

/-- Computable definition of the external boundary.
    Counts vertices outside V' that are adjacent to at least one member of V'. -/
def external_neighbors {n k : ℕ} (V' : Finset (ArrVertex n k)) : ℕ :=
  (Finset.univ.filter (fun v => v ∉ V' ∧ ∃ u ∈ V', arr_adjacent u v)).card

-- ── FORMULA COMPONENTS ────────────────────────────────────────────────────

def bit_length (x : ℕ) : ℕ :=
  if x = 0 then 0 else Nat.log2 x + 1

def sum_bit_length : ℕ → ℕ
  | 0 => 0
  | n + 1 => sum_bit_length n + bit_length n

def C_constant (R : ℕ) : ℕ :=
  (R - 1) + sum_bit_length R - E_seq R

-- ── THE DEGREE & COLLISION BRIDGE (LAYER 3.5) ─────────────────────────────

/-- Drop coordinate `p` from an arrangement vertex to get a (k-1)-sequence root -/
def drop_pos {n k : ℕ} (v : ArrVertex n k) (p : Fin k) : {x : Fin k // x ≠ p} → Fin n :=
  fun q => v.val q.val

/-- The number of unique roots when projecting V' along coordinate `p`.
    This exactly formalizes `anon_coeff` from the C++ predictor! -/
def unique_roots {n k : ℕ} (p : Fin k) (V' : Finset (ArrVertex n k)) : ℕ :=
  (V'.image (fun v => drop_pos v p)).card

/-- Sum of unique_roots across all k dimensions -/
def sum_unique_roots {n k : ℕ} (V' : Finset (ArrVertex n k)) : ℕ :=
  (Finset.univ : Finset (Fin k)).val.map (fun p => unique_roots p V') |>.sum

-- ── THE GENERIC ALGEBRAIC SQUEEZE ─────────────────────────────────────────
-- Generalizes E_add_min_le from binary splits to arbitrary partitions.
-- This is the algebraic engine that powers the Defect-based proof of Bridge 2.
--
-- KEY INSIGHT (Triangle Anomaly):
-- The naive path (sum edges_at over dimensions, apply Harper) is WRONG because
-- Harper's theorem bounds edges in *hypercubes*, not arrangement graph cliques.
-- Example: R=3 in A(n,1), the triangle K_3 has 3 edges > E_seq(3) = 2.
--
-- Instead, we prove D(V') ≤ E_seq(|V'|) where D(V') = |V'|·k - sum_unique_roots
-- is the "Defect". This bound holds even for cliques (the triangle has defect
-- 3·1 - 1 = 2 ≤ E_seq(3) = 2).

lemma foldr_max_le_sum (l : List ℕ) : l.foldr max 0 ≤ l.sum := by
  induction l with
  | nil => rfl
  | cons a t ih =>
    change max a (t.foldr max 0) ≤ a + t.sum
    omega

lemma E_seq_add_bound (a E_t S_t Mt y : ℕ)
    (h1 : E_t + S_t - Mt ≤ E_seq S_t)
    (h2 : a ≤ y)
    (h3 : Mt ≤ y)
    (h4 : Mt ≤ S_t) :
    E_seq a + E_t + (a + S_t) - y ≤ E_seq (a + S_t) := by
  have step3 : E_seq a + E_t + (a + S_t) - y ≤
    E_seq a + (E_t + S_t - Mt) + min a S_t := by omega
  have step4 : E_seq a + (E_t + S_t - Mt) + min a S_t ≤
    E_seq a + E_seq S_t + min a S_t := by omega
  have step5 := E_add_min_le a S_t
  omega

/--
  The generalized subadditivity of A000788 for any partition.
  If y ≥ max(partition sizes), the defect bound holds.
-/
lemma E_seq_list_sum_le (l : List ℕ) (y : ℕ) (hy : l.foldr max 0 ≤ y) :
    (l.map E_seq).sum + l.sum - y ≤ E_seq l.sum := by
  induction l generalizing y with
  | nil =>
    simp only [List.foldr, List.map, List.sum_nil] at hy ⊢
    omega
  | cons a t ih =>
    simp only [List.foldr, List.map, List.sum_cons] at hy ⊢
    let Mt := t.foldr max 0
    have h1 : (t.map E_seq).sum + t.sum - Mt ≤ E_seq t.sum := ih Mt (by rfl)
    have h2 : a ≤ y := by omega
    have h3 : Mt ≤ y := by omega
    have h4 : Mt ≤ t.sum := foldr_max_le_sum t
    exact E_seq_add_bound a (t.map E_seq).sum t.sum Mt y h1 h2 h3 h4

-- ── FIBER PARTITION INFRASTRUCTURE ─────────────────────────────────────────

/-- Fiber: vertices in V' with symbol s at position p -/
private def fiber {n k : ℕ} (V' : Finset (ArrVertex n k)) (p : Fin k) (s : Fin n) :
    Finset (ArrVertex n k) :=
  V'.filter (fun v => v.val p = s)

/--
  The key fiber decomposition lemma (captures all Finset plumbing).

  For any V' with |V'| ≥ 2, there exists:
  - A list `l` of fiber sizes (one per active symbol at some coordinate p)
  - A count `y` = unique_roots at position p
  such that:
  - l.sum = |V'| (partition is exhaustive)
  - y ≥ max(l) (projection is injective on fibers)
  - Each fiber is strictly smaller than V'
  - The Defect decomposes: D(V') ≤ sum E_seq(c_i) + R - y
    (using IH: D(F_i) ≤ E_seq(c_i) for each fiber)

  The proof requires:
  1. Finding a coordinate p where vertices disagree (exists since R ≥ 2)
  2. Partitioning V' by v.val p into fibers F_s
  3. Showing drop_pos is injective on fibers → unique_roots(p, F_s) = c_s
  4. Showing roots at q ≠ p are disjoint across fibers (key: drop_pos at q
     includes position p, so different symbols at p → different roots)
  5. Composing: sum_unique_roots V' = y + sum_s (sum_unique_roots F_s - c_s)
-/
private lemma defect_fiber_bound {n k : ℕ} (V' : Finset (ArrVertex n k))
    (hR : V'.card ≥ 2)
    (ih : ∀ (W : Finset (ArrVertex n k)), W.card < V'.card →
          sum_unique_roots W ≥ W.card * k - E_seq W.card) :
    ∃ (l : List ℕ) (y : ℕ),
      l.sum = V'.card ∧
      l.foldr max 0 ≤ y ∧
      (∀ c ∈ l, c < V'.card) ∧
      V'.card * k - sum_unique_roots V' ≤ (l.map E_seq).sum + l.sum - y := by
  -- 1. Find a coordinate p where two vertices disagree
  have h1 : 1 < V'.card := by omega
  obtain ⟨u, hu, v, hv, huv⟩ := Finset.one_lt_card.mp h1
  have hval : u.val ≠ v.val := fun h => huv (Subtype.ext h)
  have hdiff : ∃ p : Fin k, u.val p ≠ v.val p := Function.ne_iff.mp hval
  obtain ⟨p, hp⟩ := hdiff
  -- 2. Define the partition variables
  let active_syms := V'.image (fun w => w.val p)
  refine ⟨active_syms.toList.map (fun s => (fiber V' p s).card),
          unique_roots p V', ?_, ?_, ?_, ?_⟩
  · -- Subgoal 1: l.sum = V'.card (Exhaustiveness)
    -- Bridge List.sum to Finset.sum, then use fiberwise partition identity
    rw [Finset.sum_map_toList]
    -- Now: active_syms.sum (fun s => (fiber V' p s).card) = V'.card
    symm
    exact Finset.card_eq_sum_card_fiberwise (fun w hw => Finset.mem_image_of_mem _ hw)
  · -- Subgoal 2: l.foldr max 0 ≤ unique_roots p V' (Injective Projection)
    -- Each fiber size ≤ unique_roots p V', so max ≤ unique_roots p V'.
    have h_le : ∀ s ∈ active_syms, (fiber V' p s).card ≤ unique_roots p V' := by
      intro s _
      unfold unique_roots
      -- |fiber| ≤ |V'.image drop_pos| via injective mapping
      exact Finset.card_le_card_of_injOn (fun w => drop_pos w p)
        (fun w hw => Finset.mem_image_of_mem _ (Finset.mem_filter.mp hw).1)
        (fun a ha b hb hab => by
          have ha' := (Finset.mem_filter.mp ha).2
          have hb' := (Finset.mem_filter.mp hb).2
          exact Subtype.ext (funext fun q => by
            by_cases hq : q = p
            · subst hq; rw [ha', hb']
            · exact congr_fun hab ⟨q, hq⟩))
    -- Lift to: foldr max 0 (map ...) ≤ unique_roots p V'
    suffices ∀ (l : List ℕ) (b : ℕ), (∀ x ∈ l, x ≤ b) → l.foldr max 0 ≤ b by
      exact this _ _ (by
        intro x hx; simp only [List.mem_map] at hx
        obtain ⟨s, hs, rfl⟩ := hx
        exact h_le s (active_syms.mem_toList.mp hs))
    intro l
    induction l with
    | nil => intro _ _; simp
    | cons a t iht =>
      intro b hl
      simp only [List.foldr_cons]
      exact Nat.max_le.mpr ⟨hl a (@List.mem_cons_self _ a t),
        iht b (fun x hx => hl x (List.mem_cons_of_mem a hx))⟩
  · -- Subgoal 3: ∀ c ∈ l, c < V'.card (Strict Decrease)
    intro c hc
    simp only [List.mem_map, Finset.mem_toList] at hc
    obtain ⟨s, hs, rfl⟩ := hc
    -- c = (fiber V' p s).card, need this < V'.card
    -- fiber V' p s ⊂ V' because ∃ w ∈ V' with w.val p ≠ s
    have : fiber V' p s ⊂ V' := by
      constructor
      · intro w hw; simp [fiber] at hw; exact hw.1
      · -- V' has an element NOT in this fiber
        -- Either u.val p ≠ s or v.val p ≠ s (since u.val p ≠ v.val p)
        simp only [Finset.not_subset]
        by_cases hus : u.val p = s
        · -- u is in fiber s, so v is not (since v.val p ≠ u.val p = s)
          refine ⟨v, hv, ?_⟩
          simp [fiber]; intro _; exact fun hvs => hp (hus ▸ hvs ▸ rfl)
        · refine ⟨u, hu, ?_⟩
          simp [fiber]; intro _; exact hus
    exact Finset.card_lt_card this
  · -- Subgoal 4: The Algebraic Composition (Defect Decomposition + IH)
    -- For q ≠ p, drop_pos retains coordinate p. Roots from different fibers are
    -- STRICTLY DISJOINT at q. Apply ih to each fiber, sum over active_syms.
    sorry

-- ── BRIDGE LEMMA 2: The Defect Bound (proven by strong induction) ─────────

lemma sum_unique_roots_lower_bound {n k : ℕ}
    (R : ℕ) (V' : Finset (ArrVertex n k)) (hR : V'.card = R) :
    sum_unique_roots V' ≥ R * k - E_seq R := by
  -- We prove: ∀ R, ∀ V' with |V'| = R, defect ≤ E_seq R
  -- by strong induction on R
  revert V'
  induction R using Nat.strongRecOn with
  | ind R ih =>
  intro V' hR
  by_cases hR2 : R ≤ 1
  · -- Base case: R ≤ 1
    by_cases h0 : R = 0
    · -- R = 0: goal is sum_unique_roots V' ≥ 0, trivially true
      subst h0; omega
    · -- R = 1: goal is sum_unique_roots V' ≥ k - E_seq 1 = k
      have hR1 : R = 1 := by omega
      subst hR1
      -- V' is non-empty (card = 1)
      have hne : V'.Nonempty := Finset.card_pos.mp (by omega)
      -- Each unique_roots p V' ≥ 1 (non-empty image has positive card)
      have h_each : ∀ p : Fin k, unique_roots p V' ≥ 1 := by
        intro p; unfold unique_roots
        exact Finset.card_pos.mpr (Finset.image_nonempty.mpr hne)
      show sum_unique_roots V' ≥ 1 * k - E_seq 1
      have h_bound : sum_unique_roots V' ≥ k := by
        unfold sum_unique_roots
        suffices ∀ (m : Multiset (Fin k)),
            (∀ p ∈ m, unique_roots p V' ≥ 1) →
            (m.map (fun p => unique_roots p V')).sum ≥ m.card by
          have huniv : (Finset.univ : Finset (Fin k)).val.card = k := by
            simp [Finset.card_univ, Fintype.card_fin]
          have := this Finset.univ.val (fun p _ => h_each p)
          omega
        intro m hm
        induction m using Multiset.induction with
        | empty => simp
        | cons a s his =>
          simp only [Multiset.map_cons, Multiset.sum_cons, Multiset.card_cons]
          have ha := hm a (Multiset.mem_cons_self a s)
          have hs := his (fun p hp => hm p (Multiset.mem_cons_of_mem hp))
          omega
      omega
  · -- Inductive case: R ≥ 2
    have hR2' : V'.card ≥ 2 := by omega
    -- Pass the IH into defect_fiber_bound
    have ih_typed : ∀ (W : Finset (ArrVertex n k)), W.card < V'.card →
        sum_unique_roots W ≥ W.card * k - E_seq W.card := by
      intro W hW
      exact ih W.card (by omega) W rfl
    obtain ⟨l, y, hsum, hmax, hlt, hdefect⟩ := defect_fiber_bound V' hR2' ih_typed
    have h_alg := E_seq_list_sum_le l y hmax
    rw [hsum, hR] at h_alg
    rw [hsum, hR] at hdefect
    omega

/--
  BRIDGE LEMMA 3: The Collision Formula
  Each unique root can be extended by (n-k) fresh symbols to form distinct
  external neighbors. The constant C_constant(R) exactly bounds the maximum
  overlaps from reused active symbols.
-/
lemma external_neighbors_bound {n k : ℕ}
    (R : ℕ) (V' : Finset (ArrVertex n k)) (hR : V'.card = R) :
    external_neighbors V' ≥
      sum_unique_roots V' * (n - k) - C_constant R := by
  sorry

-- ── THE CROWNING THEOREM DECOMPOSED ────────────────────────────────────────

-- Part 1: Existence of the Optimal Cut (Constructive Upper Bound)
lemma exists_optimal_embedding (R n k : ℕ) (h_cond : can_embed_hypercube R n k) :
    ∃ V' : Finset (ArrVertex n k), V'.card = R ∧
      external_neighbors V' = (R * k - E_seq R) * (n - k) - C_constant R := by
  -- Provide the `embed_vertex` Hamming ball construction.
  -- The embedding condition n-k ≥ log2(R) ensures no intra-alphabet collisions.
  sorry

-- Part 2: Universal Lower Bound (Squeezing via Bridge Lemmas)
lemma lower_bound_all_embeddings (R n k : ℕ)
    (V' : Finset (ArrVertex n k)) (hR : V'.card = R) :
    external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R := by
  have h1 := sum_unique_roots_lower_bound R V' hR
  have h2 := external_neighbors_bound R V' hR
  -- h1: R*k - E_seq R ≤ sum_unique_roots V'
  -- h2: sum_unique_roots V' * (n-k) - C_constant R ≤ external_neighbors V'
  -- Multiply h1 by (n-k) to get the key step omega can't do alone
  have h3 := Nat.mul_le_mul_right (n - k) h1
  -- h3: (R*k - E_seq R) * (n-k) ≤ sum_unique_roots V' * (n-k)
  omega

-- The final Capstone: composition of the two halves
theorem arrangement_extraconnectivity_minimum
    (R n k : ℕ) (h_cond : can_embed_hypercube R n k) :
  (∃ V' : Finset (ArrVertex n k), V'.card = R ∧
    external_neighbors V' = (R * k - E_seq R) * (n - k) - C_constant R) ∧
  (∀ V' : Finset (ArrVertex n k), V'.card = R →
    external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R) :=
  ⟨exists_optimal_embedding R n k h_cond,
   fun V' hR => lower_bound_all_embeddings R n k V' hR⟩
