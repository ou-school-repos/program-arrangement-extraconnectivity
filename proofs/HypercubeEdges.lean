import Mathlib.Data.Nat.Basic
import Mathlib.Tactic.Ring
import Mathlib.Tactic.Linarith

/-!
  # Hypercube Edge Isoperimetry in Arrangement Graphs

  The optimal connected subgraph of size R = 2^d in A(n,k) that minimizes
  the external neighborhood is a Boolean Hypercube.

  We define the number of vertices `V d` and internal edges `E d`
  of a d-dimensional boolean hypercube embedded in A(n,k), and formally
  prove that this is mathematically identical to the cumulative popcount
  sequence (OEIS A000788) at powers of 2.
-/

-- ── Part 1: Hypercube Geometry ───────────────────────────────────────────

-- The number of vertices in a d-dimensional hypercube is 2^d
def V (d : ℕ) : ℕ := 2^d

-- The number of internal edges is defined recursively:
def E : ℕ → ℕ
| 0 => 0
| d + 1 => 2 * E d + V d

-- Theorem: The closed-form number of internal edges in a d-cube is d * 2^(d-1).
-- To avoid division/subtraction on natural numbers, we prove 2 * E(d) = d * 2^d.
theorem hypercube_edges_form (d : ℕ) : E d * 2 = d * 2^d := by
  induction d with
  | zero => rfl
  | succ d ih =>
    show (2 * E d + V d) * 2 = (d + 1) * 2 ^ (d + 1)
    unfold V
    have h : (2 * E d + 2 ^ d) * 2 = 2 * (E d * 2) + 2 ^ (d + 1) := by ring
    rw [h, ih]
    ring

-- ── Part 2: OEIS A000788 — Cumulative Popcount ───────────────────────────

-- Popcount: number of 1-bits in binary representation
def popcount : ℕ → ℕ
| 0 => 0
| n + 1 => (n + 1) % 2 + popcount ((n + 1) / 2)

-- A000788(n) = popcount(0) + popcount(1) + ... + popcount(n-1)
def A000788 : ℕ → ℕ
| 0 => 0
| n + 1 => A000788 n + popcount n

-- Lemma: popcount(2N) = popcount(N)
lemma popcount_even (N : ℕ) : popcount (2 * N) = popcount N := by
  cases N with
  | zero => rfl
  | succ n =>
    have h1 : 2 * (n + 1) = (2 * n + 1) + 1 := by omega
    rw [h1]
    unfold popcount
    have h2 : (2 * n + 2) % 2 = 0 := by omega
    have h3 : (2 * n + 2) / 2 = n + 1 := by omega
    rw [h2, h3]
    exact Nat.zero_add _

-- Lemma: popcount(2N + 1) = popcount(N) + 1
lemma popcount_odd (N : ℕ) : popcount (2 * N + 1) = popcount N + 1 := by
  unfold popcount
  have h1 : (2 * N + 1) % 2 = 1 := by omega
  have h2 : (2 * N + 1) / 2 = N := by omega
  rw [h1, h2]

-- Theorem: A000788(2N) = 2 * A000788(N) + N
lemma A000788_2N (N : ℕ) : A000788 (2 * N) = 2 * A000788 N + N := by
  induction N with
  | zero => rfl
  | succ n ih =>
    have h1 : 2 * (n + 1) = 2 * n + 2 := by omega
    rw [h1]
    have h2 : A000788 (2 * n + 2) = A000788 (2 * n) + popcount (2 * n) + popcount (2 * n + 1) := rfl
    rw [h2, ih, popcount_even n, popcount_odd n]
    omega

-- ── Part 3: The Ultimate Equivalence ─────────────────────────────────────

-- Theorem: A000788(2^d) exactly matches the hypercube edge coefficient d * 2^(d-1)
theorem A000788_power2 (d : ℕ) : A000788 (2^d) * 2 = d * 2^d := by
  induction d with
  | zero => rfl
  | succ d ih =>
    have h1 : 2^(d + 1) = 2 * 2^d := by ring
    rw [h1, A000788_2N (2^d)]
    calc (2 * A000788 (2^d) + 2^d) * 2
      _ = 2 * (A000788 (2^d) * 2) + 2 * 2^d := by ring
      _ = 2 * (d * 2^d) + 2 * 2^d           := by rw [ih]
      _ = (d + 1) * 2^(d + 1)               := by ring

-- Final Conclusion: The cumulative popcount at 2^d is EXACTLY the number
-- of internal edges E(d) in a d-dimensional hypercube.
theorem A000788_eq_E (d : ℕ) : A000788 (2^d) = E d := by
  have h1 := A000788_power2 d
  have h2 := hypercube_edges_form d
  omega
