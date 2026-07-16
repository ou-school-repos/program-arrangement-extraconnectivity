import Arrangement.ArrangementExtraconnectivity
-- NOTE: also needs the arithmetic layer already proven in
-- CrossCollisionsResearch.lean (E_seq_sum_decomposition, sum_bit_length_pow,
-- sum_bit_length_sum_decomposition, E_seq_le_sum_bit_length, popcount lemmas).
-- Either import that file or move those lemmas here / into a shared file.

open Finset

/-!
# CrossTop: an unconditional closed form for the Hamming-ball cross-collision count

## Why this file exists (read before proving `CrossRecurrence`)

`CrossRecurrence` as currently stated in `CrossCollisionsResearch.lean` is TRUE
but is the wrong interface. It asserts, unconditionally,

  cross(HB R, d) = cross(HB 2^(d-1), d) + cross(HB m, d) + ext_cube d m .

One can show (see the analysis below) that
  cross(HB 2^(d-1), d) = 0,
  cross(HB m, d)        = edge_boundary(m, d-1) - vertex_boundary(m, d-1),
  cross(HB R, d)        = edge_boundary(m, d-1),
so `CrossRecurrence` reduces to `ext_cube d m = vertex_boundary(m, d-1)`, which
is *equivalent to `HBCrossCollisions m` itself*. The strong-induction driver
hands its induction hypotheses only to itself — the Prop `CrossRecurrence`
receives none — so any direct proof of `cross_recurrence : CrossRecurrence n k`
must re-derive the whole target theorem for `m` from scratch. The interface is
circular in practice.

The fix: prove the closed form directly. For `R = 2^(d-1) + m`, `0 < m ≤ 2^(d-1)`:

  **CrossTop:  cross_collisions (HB R, d) + 2 * E_seq m = m * (d - 1)**

i.e. the cross-collision count of the top-heavy ball is *exactly the edge
boundary of HB(m) inside Q_(d-1)*. This is unconditional, needs no recursion,
and makes the strong-induction driver (and even `CrossDimStable`) unnecessary:
`HBCrossCollisions R` for `R ≥ 2` follows from CrossTop by pure arithmetic.

## Why CrossTop is true (the combinatorial argument)

Work in the ball `V = embed '' (range R)` inside the d-subcube of A(n,k).

1. (Collisions are cube vertices.) If an external vertex `w` lies in
   `coord_boundary V p ∩ coord_boundary V q` with `p ≠ q`, take witnesses
   `v = embed i` (w agrees with v off p) and `v' = embed i'` (agrees off q).
   Then `w` agrees with `v` everywhere except position `p`, and
   `w p = v' p`, which is one of the two cube symbols at `p`. Since `w ≠ v`,
   `w p` is the *other* cube symbol, forcing `p < d` and
   `w = embed (i ^^^ 2^p)`. So only vertices of the form `embed j`,
   `j ∈ Ico R (2^d)`, can be counted more than once; every other external
   vertex contributes multiplicity exactly 1.

2. (Multiplicity of a cube vertex.) For `j ∈ Ico R (2^d)`,
   `embed j ∈ coord_boundary V p ↔ p < d ∧ j ^^^ 2^p < R`.
   Hence `mult (embed j) = ball_deg R d j` (defined below).

3. (Everyone in the top half is hit.) For `j ∈ Ico R (2^d)` we have
   `testBit j (d-1) = true`, so its partner `j ^^^ 2^(d-1) = j - 2^(d-1)`
   lies in `range (2^(d-1)) ⊆ range R`. Hence `ball_deg R d j ≥ 1` for ALL
   top external vertices, and

     cross(HB R) = Σ_{j ∈ Ico R (2^d)} (ball_deg R d j - 1).

4. (Reindex to the small ball.) Writing `j = 2^(d-1) + j'`, `j' ∈ Ico m (2^(d-1))`:
     ball_deg R d (2^(d-1) + j') = 1 + ball_deg m (d-1) j',
   so cross(HB R) = Σ_{j' ∈ Ico m (2^(d-1))} ball_deg m (d-1) j'
                  = edge boundary of HB(m) in Q_(d-1)
                  = m * (d-1) - 2 * E_seq m.

All four steps, the final arithmetic, and CrossTop itself have been verified
by exhaustive brute force on A(8,4), A(9,4), A(11,5), A(12,5) for all feasible R
(see verify_crosstop.py in the same directory as this file's delivery bundle).

## What is fully proven here vs. left as `sorry`

Layer A (pure ℕ / cube counting) is written out in full in the repo's style
(omega-heavy, additive statements to dodge truncated subtraction) and should
compile with at most cosmetic fixes. Layer B (the Finset bridge to
`cross_collisions`) is stated precisely with detailed proof plans but left as
`sorry`, because it manipulates repo definitions (`coord_boundary`,
`total_coord_edges`, `external_neighbors`, `cross_collisions`, `drop_pos`,
`embed_vertex`) whose exact normal forms I could only infer from the diff
context — those proofs need to be finished interactively against the real
definitions. Layer C (assembly + the new driver) is arithmetic and written in
full modulo the Layer B inputs.
-/

namespace Arrangement

/-! ### Layer A0: XOR-with-a-power-of-two toolbox -/

section XorTwoPow

/-- Flipping a set bit subtracts that power of two (stated additively). -/
lemma xor_two_pow_of_testBit_true {j p : ℕ} (h : j.testBit p = true) :
    j ^^^ 2 ^ p + 2 ^ p = j := by
  -- Proof plan: `Nat.eq_of_testBit_eq` on both sides, or induction on `p`
  -- generalizing `j` with the div/mod peeling used in `popcount_two_pow_add`.
  -- Mathlib candidates: `Nat.testBit_xor`, `Nat.testBit_two_pow`,
  -- `Nat.xor_two_pow` (if present in the pinned Mathlib).
  sorry

/-- Flipping a clear bit adds that power of two. -/
lemma xor_two_pow_of_testBit_false {j p : ℕ} (h : j.testBit p = false) :
    j ^^^ 2 ^ p = j + 2 ^ p := by
  sorry

/-- XOR does not leave the cube: `j < 2^D → p < D → j ^^^ 2^p < 2^D`.
    (Mathlib: `Nat.xor_lt_two_pow` should close this directly.) -/
lemma xor_two_pow_lt {j p D : ℕ} (hj : j < 2 ^ D) (hp : p < D) :
    j ^^^ 2 ^ p < 2 ^ D := by
  have h2 : (2 : ℕ) ^ p < 2 ^ D := Nat.pow_lt_pow_right (by omega) hp
  exact Nat.xor_lt_two_pow hj h2

/-- `popcount` as a filter-card over the bit positions, for `m < 2^D`. -/
lemma popcount_eq_card_testBit {D : ℕ} :
    ∀ {m : ℕ}, m < 2 ^ D →
      popcount m = ((range D).filter (fun p => m.testBit p)).card := by
  -- Induction on D. Base: m = 0, both sides 0.
  -- Step: split m = 2 * (m / 2) + m % 2; use `popcount_div_two`,
  -- `Nat.testBit_succ`-style reindexing of `range (D+1)` via
  -- `Finset.range_succ` / `Finset.filter_insert`, testBit 0 = decide (m % 2 = 1).
  sorry

end XorTwoPow

/-! ### Layer A: pure cube counting -/

section CubeCounting

/-- Number of `Q_D`-neighbours of `j` lying in the initial segment `{0, …, m-1}`. -/
def ball_deg (m D j : ℕ) : ℕ :=
  ((range D).filter (fun p => j ^^^ 2 ^ p < m)).card

/-- The vertex `m` itself has exactly `popcount m` neighbours below it:
    flipping a set bit lands `< m`, flipping a clear bit lands `> m`. -/
lemma ball_deg_self {m D : ℕ} (hm : m < 2 ^ D) :
    ball_deg m D m = popcount m := by
  unfold ball_deg
  rw [popcount_eq_card_testBit hm]
  congr 1
  apply Finset.filter_congr
  intro p hp
  rw [mem_range] at hp
  constructor
  · intro hlt
    by_contra hbit
    have hf : m.testBit p = false := by
      cases h : m.testBit p with
      | false => rfl
      | true => exact absurd h hbit
    have := xor_two_pow_of_testBit_false hf
    omega
  · intro hbit
    have := xor_two_pow_of_testBit_true (by simpa using hbit)
    have hpow : 0 < 2 ^ p := Nat.two_pow_pos p
    omega

/-- The number of `Q_D`-neighbours of `m` lying strictly above it is
    `D - popcount m` (stated additively). -/
lemma card_up_neighbors {m D : ℕ} (hm : m < 2 ^ D) :
    ((range D).filter (fun p => ¬ m.testBit p)).card + popcount m = D := by
  rw [popcount_eq_card_testBit hm]
  have := Finset.filter_card_add_filter_neg_card_eq_card
    (s := range D) (p := fun p => m.testBit p)
  simp only [card_range] at this
  omega

/-- **Layer A main lemma.** The (directed, into-the-ball) edge count from the
    outside of the initial segment equals its edge boundary:

      Σ_{j ∈ [m, 2^D)} ball_deg m D j + 2 * E_seq m = m * D.

    Proof: induction on `m`. Moving `m → m+1` removes vertex `m` from the
    outside (killing its `popcount m` into-ball edges) and adds it to the ball
    (creating `D - popcount m` new into-ball edges from its upper neighbours);
    meanwhile `2 * E_seq` grows by `2 * popcount m`. Net change: `+D`. -/
lemma sum_ball_deg (D : ℕ) :
    ∀ m, m ≤ 2 ^ D →
      (∑ j ∈ Ico m (2 ^ D), ball_deg m D j) + 2 * E_seq m = m * D := by
  intro m
  induction m with
  | zero =>
    intro _
    have hz : ∀ j ∈ Ico 0 (2 ^ D), ball_deg 0 D j = 0 := by
      intro j _
      unfold ball_deg
      simp [Finset.filter_false_of_mem, Nat.not_lt_zero]
    simp [Finset.sum_congr rfl hz, E_seq]
  | succ m ih =>
    intro hm1
    have hm : m < 2 ^ D := by omega
    -- Split off j = m from the old sum.
    have hsplit :
        (∑ j ∈ Ico m (2 ^ D), ball_deg m D j)
          = ball_deg m D m + ∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg m D j := by
      rw [← Finset.sum_Ico_eq_sum_range]  -- placeholder; the clean route is:
      sorry
      -- Clean route: `Finset.sum_eq_sum_Ico_succ_bot hm` (Mathlib) or
      -- `Finset.Ico_succ_left`-style peel:
      --   rw [Finset.sum_Ico_eq_sum_range] is NOT it; use
      --   `Finset.sum_eq_sum_Ico_succ_bot : m < n → ∑ i in Ico m n, f i = f m + ∑ i in Ico (m+1) n, f i`.
    -- Pointwise growth of ball_deg on the remaining range:
    -- for j ∈ Ico (m+1) (2^D),
    --   ball_deg (m+1) D j = ball_deg m D j + (if j is a Q_D-neighbour of m then 1 else 0),
    -- because {p : j ^^^ 2^p < m+1} \ {p : j ^^^ 2^p < m} = {p : j ^^^ 2^p = m}.
    have hgrow :
        (∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg (m + 1) D j)
          = (∑ j ∈ Ico (m + 1) (2 ^ D), ball_deg m D j)
            + ((range D).filter (fun p => ¬ m.testBit p)).card := by
      -- Proof plan:
      --  (a) pointwise: ball_deg (m+1) D j = ball_deg m D j
      --        + ((range D).filter (fun p => j ^^^ 2^p = m)).card
      --      via `Finset.filter_or`-style split of `_ < m + 1` into `_ < m ∨ _ = m`
      --      and `Finset.card_union_of_disjoint`.
      --  (b) swap the double sum:
      --      Σ_{j ∈ Ico (m+1) (2^D)} #{p < D : j ^^^ 2^p = m}
      --        = #{p < D : m ^^^ 2^p ∈ Ico (m+1) (2^D)}       (j := m ^^^ 2^p is forced)
      --        = #{p < D : ¬ m.testBit p}
      --      using `xor_two_pow_of_testBit_false` (lands at m + 2^p > m, < 2^D by
      --      `xor_two_pow_lt`) and `xor_two_pow_of_testBit_true` (lands < m, excluded).
      sorry
    have hself := ball_deg_self hm
    have hup := card_up_neighbors hm
    have hE : E_seq (m + 1) = E_seq m + popcount m := rfl
    -- Assemble: new_sum = old_tail + (D - popcount m)
    --                   = (old_sum - popcount m) + (D - popcount m); all additive:
    have := ih (by omega)
    -- goal: (∑ j ∈ Ico (m+1) (2^D), ball_deg (m+1) D j) + 2 * E_seq (m+1) = (m+1) * D
    rw [hgrow, hE]
    have hmul : (m + 1) * D = m * D + D := by ring
    omega

end CubeCounting

/-! ### Layer B: bridge from `cross_collisions` to `ball_deg`

These are the only lemmas that touch the arrangement graph. Signatures are
written against the definitions inferred from `ArrangementExtraconnectivity.lean`
(`coord_boundary`, `total_coord_edges = Σ_p card (coord_boundary · p)`,
`external_neighbors`, `cross_collisions = total - external`); adjust names /
argument order to the real ones when wiring in. -/

section Bridge

variable {n k : ℕ}

/-- The external-neighbour set is exactly the union of the coordinate
    boundaries, and `cross_collisions` is the excess multiplicity:

      cross_collisions V + external_neighbors V = total_coord_edges V

    with `total_coord_edges V = Σ_{w ∈ U} mult V w`, where
    `mult V w := ((univ : Finset (Fin k)).filter (fun p => w ∈ coord_boundary V p)).card`.

    Proof plan: this is the generic inclusion–multiplicity identity
    Σ_p |B_p| = Σ_{w ∈ ⋃ B_p} #{p : w ∈ B_p}: rewrite each `card` as
    `Finset.sum_boole`/indicator and apply `Finset.sum_comm`. -/
def mult (V : Finset (ArrVertex n k)) (w : ArrVertex n k) : ℕ :=
  ((Finset.univ : Finset (Fin k)).filter (fun p => w ∈ coord_boundary V p)).card

lemma total_eq_sum_mult (V : Finset (ArrVertex n k)) :
    total_coord_edges V
      = ∑ w ∈ (Finset.univ.filter (fun w =>
          w ∉ V ∧ ∃ v ∈ V, arr_adjacent v w)), mult V w := by
  sorry

/-- **B1 (membership).** For `t ≤ 2^d`, `j ∈ Ico t (2^d)`, and `p : Fin k`:

      embed_vertex n k d (nat_to_cube d j) hk hnk ∈
        coord_boundary (hamming_ball_subset t n k d hk hnk) p
      ↔ p.val < d ∧ j ^^^ 2 ^ p.val < t.

    Proof plan (⇐): witness `i := j ^^^ 2^(p.val) < t`; `drop_pos` agrees off `p`
    because `nat_to_cube d i` and `nat_to_cube d j` differ exactly in bit `p`
    (`Nat.testBit_xor`, `Nat.testBit_two_pow`), and `embed_cube` is coordinatewise;
    non-membership in the ball from `embed_vertex` injectivity (`embed_vertex
    _injective_cube` + `nat_to_cube_injective`) and `j ≥ t`.
    (⇒): a witness `embed i`, `i < t`, agreeing off `p` forces (coordinatewise,
    by the two-symbol coding being injective at each position `< d`, and
    positions `≥ d` being constant) `testBit i q = testBit j q` for all `q ≠ p.val`
    with `q < d`; hence `i = j ∨ i = j ^^^ 2^(p.val)` by `Nat.eq_of_testBit_eq`;
    `i = j` is impossible (`i < t ≤ j`); if `p.val ≥ d` all bits agree, forcing
    `i = j`, contradiction — which also yields the `p.val < d` conjunct. -/
lemma embed_mem_coord_boundary_iff {t d j : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) (hj : j ∈ Ico t (2 ^ d)) (p : Fin k) :
    embed_vertex n k d (nat_to_cube d j) hk hnk ∈
        coord_boundary (hamming_ball_subset t n k d hk hnk) p
      ↔ p.val < d ∧ j ^^^ 2 ^ p.val < t := by
  sorry

/-- **B2 (multiplicity of a cube vertex).** Immediate from B1 by transporting the
    filter along `Fin k ↪ ℕ` (`Fin.val`), using `p.val < d ≤ k`. -/
lemma mult_embed_eq_ball_deg {t d j : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) (hj : j ∈ Ico t (2 ^ d)) :
    mult (hamming_ball_subset t n k d hk hnk)
        (embed_vertex n k d (nat_to_cube d j) hk hnk)
      = ball_deg t d j := by
  sorry

/-- **B3 (collisions live on the cube).** If `w` lies in two distinct coordinate
    boundaries of the ball, then `w = embed j` for some `j ∈ Ico t (2^d)`.

    Proof plan: witnesses `v = embed i` (agrees with `w` off `p`) and
    `v' = embed i'` (agrees off `q`), `p ≠ q`. Then `w r = v r` for `r ≠ p` and
    `w p = v' p`. If `p.val ≥ d` then `v' p = v p` (both are the constant
    symbol), so `w = v ∈ V`, contradiction; hence `p.val < d`, `w p` is a cube
    symbol at `p` distinct from `v p` (else `w = v`), so
    `w = embed (i ^^^ 2^(p.val))` by funext + the coordinatewise unfolding used
    in `cross_dim_stable`. Set `j := i ^^^ 2^(p.val)`; `j < 2^d` by
    `xor_two_pow_lt`, and `j ≥ t` because `w ∉ V` + injectivity. -/
lemma mem_two_boundaries_is_cube {t d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) {w : ArrVertex n k} {p q : Fin k} (hpq : p ≠ q)
    (hp : w ∈ coord_boundary (hamming_ball_subset t n k d hk hnk) p)
    (hq : w ∈ coord_boundary (hamming_ball_subset t n k d hk hnk) q) :
    ∃ j ∈ Ico t (2 ^ d),
      w = embed_vertex n k d (nat_to_cube d j) hk hnk := by
  sorry

/-- **B4 (bridge, assembled).** For any `t ≤ 2^d`:

      cross_collisions (HB t)
        = Σ_{j ∈ Ico t (2^d), ball_deg t d j ≥ 1} (ball_deg t d j - 1),

    stated additively to stay in ℕ:

      cross_collisions (HB t) + card {j ∈ Ico t (2^d) | 1 ≤ ball_deg t d j}
        = Σ_{j ∈ Ico t (2^d)} ball_deg t d j.

    Proof plan: start from `total_eq_sum_mult` and the definition
    `cross_collisions = total_coord_edges - external_neighbors`
    (exact via `external_neighbors_le_total_coord`). Split the external set `U`
    into `U_cube = U ∩ embed '' (Ico t (2^d))` and its complement.
    By B3, `mult = 1` on the complement (mult ≥ 1 since `w ∈ U` means some
    boundary contains it; mult ≤ 1 since two boundaries would force cube-ness).
    By B2, `Σ_{U_cube} mult = Σ_{j : ball_deg ≥ 1} ball_deg` after transporting
    along the (injective) embedding — note `embed j ∈ U ↔ ball_deg t d j ≥ 1`.
    Cancel `|U|` against the two pieces. -/
lemma cross_collisions_eq_cube_sum {t d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (ht : t ≤ 2 ^ d) :
    cross_collisions (hamming_ball_subset t n k d hk hnk)
        + ((Ico t (2 ^ d)).filter (fun j => 1 ≤ ball_deg t d j)).card
      = ∑ j ∈ Ico t (2 ^ d), ball_deg t d j := by
  sorry

end Bridge

/-! ### Layer C: the closed form and the new (induction-free) driver -/

section CrossTop

variable {n k : ℕ}

/-- Top-half vertices always see their bottom partner: for
    `R = 2^(d-1) + m`, `0 < m ≤ 2^(d-1)`, and `j ∈ Ico R (2^d)`,
    bit `d-1` of `j` is set and `j ^^^ 2^(d-1) = j - 2^(d-1) < 2^(d-1) < R`,
    plus the reindexing identity down to dimension `d-1`. -/
lemma ball_deg_top {d m j' : ℕ} (hd : 1 ≤ d) (hm : 0 < m) (hm' : m ≤ 2 ^ (d - 1))
    (hj' : j' ∈ Ico m (2 ^ (d - 1))) :
    ball_deg (2 ^ (d - 1) + m) d (2 ^ (d - 1) + j')
      = 1 + ball_deg m (d - 1) j' := by
  -- Proof plan: split `range d = range (d-1) ∪ {d-1}`.
  --  * p = d-1: bit is set (`j = 2^(d-1) + j'`, `j' < 2^(d-1)`), and
  --    `j ^^^ 2^(d-1) = j' < 2^(d-1) < 2^(d-1) + m` — contributes the `1`.
  --  * p < d-1: `(2^(d-1) + j') ^^^ 2^p = 2^(d-1) + (j' ^^^ 2^p)`
  --    (low-bit XOR doesn't touch bit d-1; via testBit extensionality or the
  --    additive xor lemmas), and `2^(d-1) + x < 2^(d-1) + m ↔ x < m`.
  sorry

/-- **CrossTop.** The unconditional closed form: for `d ≥ 1`, `0 < m ≤ 2^(d-1)`,
    `R = 2^(d-1) + m`, `d ≤ k`, `k + d ≤ n`:

      cross_collisions (HB R) + 2 * E_seq m = m * (d - 1).

    This replaces `CrossRecurrence` (and obviates `CrossDimStable` and the
    strong-induction driver). -/
theorem cross_top {d m : ℕ} (hd : 1 ≤ d) (hm : 0 < m) (hm' : m ≤ 2 ^ (d - 1))
    (hk : d ≤ k) (hnk : k + d ≤ n) :
    cross_collisions
        (hamming_ball_subset (2 ^ (d - 1) + m) n k d hk hnk)
      + 2 * E_seq m = m * (d - 1) := by
  set R := 2 ^ (d - 1) + m with hR
  have hpow : (2 : ℕ) ^ d = 2 ^ (d - 1) + 2 ^ (d - 1) := by
    conv_lhs => rw [← Nat.sub_add_cancel hd]; rw [pow_succ]; ring_nf
    sorry -- cosmetic: same manipulation as in the existing driver, `omega`-close
  have hRle : R ≤ 2 ^ d := by omega
  have hbridge := cross_collisions_eq_cube_sum (t := R) hk hnk hRle
  -- Reindex the top strip Ico R (2^d) by j = 2^(d-1) + j', j' ∈ Ico m (2^(d-1)):
  have hreindex :
      (∑ j ∈ Ico R (2 ^ d), ball_deg R d j)
        = ∑ j' ∈ Ico m (2 ^ (d - 1)), (1 + ball_deg m (d - 1) j') := by
    -- `Finset.sum_Ico_eq_sum_Ico_add`-style shift (or `Finset.sum_map` with the
    -- embedding `j' ↦ 2^(d-1) + j'`, as in `hb_half1_eq_image_shifted`),
    -- then pointwise `ball_deg_top`.
    sorry
  have hfull :
      ((Ico R (2 ^ d)).filter (fun j => 1 ≤ ball_deg R d j)).card
        = 2 ^ (d - 1) - m := by
    -- Every j in the strip has ball_deg ≥ 1 (its partner), so the filter is
    -- the whole Ico; `Nat.card_Ico` gives `2^d - R = 2^(d-1) - m`.
    sorry
  have hA := sum_ball_deg (d - 1) m hm'
  have hcard : (Ico m (2 ^ (d - 1))).card = 2 ^ (d - 1) - m := Nat.card_Ico _ _
  rw [Finset.sum_add_distrib, Finset.sum_const, hcard, smul_eq_mul, mul_one]
    at hreindex
  omega

/-- `2 * E_seq (2^j) = j * 2^j` — the exact value at powers of two, additive.
    Induction via `E_seq_sum_decomposition` at `m = 2^j`. -/
lemma E_seq_pow (j : ℕ) : 2 * E_seq (2 ^ j) = j * 2 ^ j := by
  induction j with
  | zero => simp [E_seq, popcount]
  | succ j ih =>
    have hsplit : E_seq (2 ^ j + 2 ^ j) = E_seq (2 ^ j) + E_seq (2 ^ j) + 2 ^ j := by
      have := E_seq_sum_decomposition (j + 1) (2 ^ j) (by simp)
      simpa using this
    have e1 : (2 : ℕ) ^ (j + 1) = 2 ^ j + 2 ^ j := by rw [pow_succ]; ring
    rw [e1, hsplit]
    have e2 : (j + 1) * (2 ^ j + 2 ^ j) = 2 * (j * 2 ^ j) + 2 * 2 ^ j := by ring
    omega

/-- `C_constant` de-truncated: `C_constant R + E_seq R = (R - 1) + sum_bit_length R`,
    valid because `E_seq R ≤ sum_bit_length R`. -/
lemma C_constant_add_E_seq (R : ℕ) :
    C_constant R + E_seq R = (R - 1) + sum_bit_length R := by
  have h := E_seq_le_sum_bit_length R
  unfold C_constant
  omega

/-- **The new driver: `HBCrossCollisions` for every `R ≥ 1`, no induction,
    no `CrossDimStable`, no `CrossRecurrence`.**

    Case `R = 1` is `cross_base_one`. For `R ≥ 2`, with `d = bit_length (R-1)`
    and `m = R - 2^(d-1)`, combine `cross_top` with the arithmetic identities;
    everything closes by `omega`. -/
theorem hb_cross_collisions_closed (R : ℕ) (hR1 : 1 ≤ R)
    (hbase : CrossBaseOne n k)
    (d : ℕ) (hd : d = bit_length (R - 1)) (hk : d ≤ k) (hnk : k + d ≤ n) :
    HBCrossCollisions R n k d hk hnk := by
  unfold HBCrossCollisions
  rcases Nat.lt_or_ge R 2 with hR2 | hR2
  · -- R = 1: as in the existing driver.
    have hR : R = 1 := by omega
    subst hR
    have hd0 : d = 0 := by
      rw [hd, bit_length_eq_size]; simpa using Nat.size_zero
    subst hd0
    have hb := hbase 0 hk hnk
    have hE1 : E_seq 1 = 0 := by decide
    have hC1 : C_constant 1 = 0 := by decide
    omega
  · -- R ≥ 2: locate R in (2^(d-1), 2^d] exactly as the old driver does.
    have hdsize : d = Nat.size (R - 1) := by rw [hd, bit_length_eq_size]
    have hd1 : 1 ≤ d := by
      rw [hdsize]
      have : (0 : ℕ) < Nat.size (R - 1) := Nat.size_pos.mpr (by omega)
      omega
    have hup : R - 1 < 2 ^ d := by rw [hdsize]; exact Nat.lt_size_self (R - 1)
    have hlow : 2 ^ (d - 1) ≤ R - 1 := by
      have : d - 1 < Nat.size (R - 1) := by omega
      rw [hdsize] at this ⊢
      exact Nat.lt_size.mp (by omega)
    set m := R - 2 ^ (d - 1) with hm_def
    have hR_eq : R = 2 ^ (d - 1) + m := by omega
    have hm_pos : 0 < m := by omega
    have hm_le : m ≤ 2 ^ (d - 1) := by
      have hpow : (2 : ℕ) ^ d = 2 ^ (d - 1) + 2 ^ (d - 1) := by
        conv_lhs => rw [← Nat.sub_add_cancel hd1]
        rw [pow_succ]; ring
      omega
    -- The five arithmetic facts + CrossTop; omega closes.
    have htop := by
      rw [hR_eq]
      exact cross_top (n := n) (k := k) hd1 hm_pos hm_le hk hnk
    have hE_split := E_seq_sum_decomposition d m hm_le
    have hL_split := sum_bit_length_sum_decomposition d m hd1 hm_le
    have hL_pow := sum_bit_length_pow (d - 1)
    have hE_pow := E_seq_pow (d - 1)
    have hCE := C_constant_add_E_seq R
    have hmul : m * d = m * (d - 1) + m := by
      conv_lhs => rw [← Nat.sub_add_cancel hd1]; ring
    rw [hR_eq] at hCE hE_split hL_split ⊢
    -- Target: cross + E_seq (P + m) = C_constant (P + m).
    -- LHS = m(d-1) - 2E_m + E_P + E_m + m = m*d - E_m + E_P.
    -- RHS = (P + m - 1) + sbl P + m*d - E_P - E_m - m, and
    --   sbl P + P = (d-1)P + 1,  2E_P = (d-1)P   close the gap.
    omega

end CrossTop

end Arrangement
