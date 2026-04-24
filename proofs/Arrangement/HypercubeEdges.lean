import Mathlib.Data.Nat.Basic
import Mathlib.Tactic.Ring
import Mathlib.Tactic.Linarith

def V (d : ℕ) : ℕ := 2^d

def E : ℕ → ℕ
| 0 => 0
| d + 1 => 2 * E d + V d

theorem hypercube_edges_form (d : ℕ) : E d * 2 = d * 2^d := by
  induction d with
  | zero => rfl
  | succ d ih =>
    show (2 * E d + V d) * 2 = (d + 1) * 2 ^ (d + 1)
    unfold V
    have h : (2 * E d + 2 ^ d) * 2 = 2 * (E d * 2) + 2 ^ (d + 1) := by ring
    rw [h, ih]
    ring

def popcount : ℕ → ℕ
| 0 => 0
| n + 1 => (n + 1) % 2 + popcount ((n + 1) / 2)

lemma popcount_succ (n : ℕ) : popcount (n + 1) = (n + 1) % 2 + popcount ((n + 1) / 2) := by
  rw [popcount]

def A000788 : ℕ → ℕ
| 0 => 0
| n + 1 => A000788 n + popcount n

lemma A000788_succ (n : ℕ) : A000788 (n + 1) = A000788 n + popcount n := by
  rw [A000788]

lemma popcount_even (N : ℕ) : popcount (2 * N) = popcount N := by
  cases N with
  | zero => rfl
  | succ n =>
    have h1 : 2 * (n + 1) = 2 * n + 1 + 1 := by omega
    rw [h1, popcount_succ (2 * n + 1)]
    have h2 : (2 * n + 1 + 1) % 2 = 0 := by omega
    have h3 : (2 * n + 1 + 1) / 2 = n + 1 := by omega
    rw [h2, h3, Nat.zero_add]

lemma popcount_odd (N : ℕ) : popcount (2 * N + 1) = popcount N + 1 := by
  rw [popcount_succ (2 * N)]
  have h2 : (2 * N + 1) % 2 = 1 := by omega
  have h3 : (2 * N + 1) / 2 = N := by omega
  rw [h2, h3]
  omega

lemma A000788_2N (N : ℕ) : A000788 (2 * N) = 2 * A000788 N + N := by
  induction N with
  | zero => rfl
  | succ n ih =>
    have h1 : 2 * (n + 1) = 2 * n + 1 + 1 := by omega
    rw [h1]
    rw [A000788_succ (2 * n + 1), A000788_succ (2 * n)]
    rw [ih, popcount_even n, popcount_odd n]
    rw [A000788_succ n]
    omega

theorem A000788_power2 (d : ℕ) : A000788 (2^d) * 2 = d * 2^d := by
  induction d with
  | zero =>
    have h : 2^0 = 1 := rfl
    rw [h]
    rw [A000788_succ 0]
    unfold popcount
    unfold A000788
    rfl
  | succ d ih =>
    have h1 : 2^(d + 1) = 2 * 2^d := by ring
    rw [h1, A000788_2N (2^d)]
    have h2 : 2 * (d * 2^d) + 2 * 2^d = (d + 1) * (2 * 2^d) := by ring
    calc (2 * A000788 (2^d) + 2^d) * 2
      _ = 2 * (A000788 (2^d) * 2) + 2 * 2^d := by ring
      _ = 2 * (d * 2^d) + 2 * 2^d           := by rw [ih]
      _ = (d + 1) * (2 * 2^d)               := by rw [h2]

theorem A000788_eq_E (d : ℕ) : A000788 (2^d) = E d := by
  have h1 := A000788_power2 d
  have h2 := hypercube_edges_form d
  omega
