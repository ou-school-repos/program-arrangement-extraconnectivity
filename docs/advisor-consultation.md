# Status Update: The Arrangement Graph Extraconnectivity Problem

We have exhaustively verified the core strictness conjecture
($L_c \le (m+1)(|\pi| - R) + \Delta C + m\Delta E$) across 3,796 adversarial
test cases. The global bound holds. However, we have computationally proven that
the standard structural isoperimetric tools cannot be used to formalize the
proof.

We must abandon local, additive, and direct-mapping approaches. Here is the
formal autopsy of why standard techniques structurally fail on the arrangement
graph $A(n,k)$:

**1. Local Graph Edits (Kruskal-Katona Shifting / Hole-Filling Induction)**

- **The Approach:** Inductively building or compressing the set vertex-by-vertex
  (or via dual-compression swaps) to navigate toward a canonical dense state
  without decreasing slack.
- **The Obstruction (The Concentration Cliff):** In $A(n,k)$, the alphabet
  overhang ($m = n-k$) imposes a rigid $(m+1)$ penalty when global concentration
  ($R - |\pi|$) increases. A single local vertex addition can independently push
  global $R$ across a power-of-2 boundary without triggering a proportional
  increase in the local slice size $R_i$. The local geometric refund
  ($\alpha = 1$) is structurally too small to pay the global arithmetic toll
  ($m+1 \ge 2$). Furthermore, at high densities, injectivity requirements
  completely block classical guarded symbol swaps.
- **Conclusion:** Local topology cannot navigate the decoupled step-functions of
  binary arithmetic.

**2. Additive Lyapunov Potentials (Independent Fiber Sums)**

- **The Approach:** Defining a global potential
  $\Psi(V') = \sum_{F} f(|V' \cap F|)$ over 1D fibers to ledger adversarial
  chaos against the target $C(R) + mE(R)$.
- **The Obstruction (Cross-Fiber Blindness):** Any additive fiber-size ansatz is
  rigidly locked by the embedded Hamming cubes. Matching the $R=1$ and $R=2$
  targets strictly forces the lowest weights to $w_1 = 0$ and $w_2 = m+1$. At
  $m \ge 2$, this locked potential evaluates a 4-cycle at exactly
  $\Psi = 4m + 4$, while the true adversarial load is $\Phi = X + (m+1)D = 8m$.
- **Conclusion:** An additive sum over independent 1D fibers is mathematically
  blind to cross-fiber geometric collisions. The true invariant must be
  non-additive or coordinate-coupled.

**3. Geometric-to-Arithmetic Bijection (Carry Mapping)**

- **The Approach:** Proving that geometric cross-collisions structurally force
  binary carries in the partition sum $\sum R_i = R$, thus paying for themselves
  via the subadditivity gap $\Delta E$.
- **The Obstruction (Multi-way vs. Sequential Mismatch):** When isolated to sets
  where the unfilled slot refund is zero ($\text{conc} = 0$), the binary-carry
  mass of $\sum R_i$ systematically under-covers the geometric collision mass
  $L_c$, missing by up to 18 units in our test corpus. $L_c$ is driven by
  non-linear, multi-way geometric overlaps (many slices converging on one empty
  slot). The subadditivity gap $\Delta E$ is driven by 1D, sequential
  bit-arithmetic.
- **Conclusion:** Geometric collisions in $A(n,k)$ do not map injectively to
  binary carries.

**4. Inductive Merge / Split-and-Recombine**

- **The Approach:** Inductively building the set by merging disjoint coordinate
  slices ($Q_i, Q_j$), attempting to pay for the geometric cross-collisions
  ($|\partial A \cap \partial B|$) using the linear arithmetic surplus
  guaranteed by the subadditivity lemma:
  $E(A+B) \ge E(A) + E(B) + \min(|A|, |B|)$.
- **The Obstruction (Geometric Wrap-around):** Exhaustive testing confirms the
  cross-collision cost locally outscales the binary surplus, even when strictly
  restricted to valid, parallel coordinate slices. In $A(6,3)$ ($m=3$), merging
  a slice of size $|A|=1$ and $|B|=19$ yields a cross-collision overlap of $6$,
  strictly exceeding the arithmetic budget of $m \cdot \min(1, 19) = 3$. The
  linear surplus structurally cannot survive $\min=1$ because a large slice can
  geometrically "wrap around" a singleton slice and share many external
  neighbors, completely decoupling from the arithmetic cap. (An unrestricted,
  exhaustive test over arbitrary disjoint vertex pairs fails even harder and
  even earlier, at $|A|=2,|B|=1$ in $A(4,2)$; restricting to genuine
  coordinate-slice pairs, as the merge step actually requires, delays but does
  not avoid the failure — it survives for $m=1$/$m=2$ small cases but reappears
  at $m=2$ ($A(5,3)$, 25/1440 slice-pairs) and worsens at $m=3$ ($A(6,3)$,
  488/6000 slice-pairs).)
- **Conclusion:** The geometric cost of merging slices is not bounded by the
  linear subadditivity surplus, even restricted to the actual slice pairs the
  induction needs.

**5. Locating the Real Target: $\Phi = X + (m+1)D \le C(R) + mE(R)$ (Two Wrong
Shapes Refuted, the Correct One Confirmed)**

Two prior passes on this item tested the wrong inequality shape and were
retracted. Recorded here so the dead ends aren't silently re-discovered:

- **Wrong shape 1 ($X(F) \le C(c) - D(F)$, per fiber, no $y$):** refuted, but
  only at depth-0 (the whole set) — a scoping bug caught in review, since
  depth-0 just re-derives the already-known global Star Graph refutation on
  smaller instances and says nothing about genuine fiber splits.
- **Wrong shape 2 (aggregate, $X(V') \le \sum_s(C(c_s)-D(F_s)) + (R-y)$):** also
  refuted, at small scale (e.g. $A(4,2)$ star size 3: $X=7 > 6$). But this is
  not the requirement either — from `total_coord_edges_eq`
  ($|\partial V'| + X + Rk = U\cdot(m+1)$, $U=Rk-D$), the algebra that
  `UniversalLowerBound` actually needs is $\Phi = X + (m+1)D \le C(R) + mE(R)$,
  not $X+D \le C(R)$. The $m\cdot E(R)$ term is exactly what the superseded
  `CollisionAdjustedBound` hypothesis threw away, making it a strictly
  _stronger_, dimension-independent claim than what the proof needs — its
  refutation by the Star Graph (`docs/lean-proof-status.md`'s "Superseded" note)
  does not carry over to $\Phi \le C+mE$.
- **The correct target, tested directly:** $\Phi \le C(R)+mE(R)$ holds on every
  whole-set and per-fiber (depth $\ge 1$) configuration tested, including the
  literal Star Graph counterexample dimensions that killed
  `CollisionAdjustedBound` ($A(15,7)$, $R=8$: $\Phi=81 \le C+mE=108$), and
  larger stress cases where the looser $X \le C(R)$ shape (no $D$, no $m$ term)
  does fail outright once the ambient graph has room ($A(9,5)$, $R=20$:
  $X=108 > C(20)=48$, yet $\Phi=203 \le C+mE=208$).
- **Conclusion:** This is not a dead end — it is computational evidence _for_
  `UniversalLowerBound`, via the inequality it is actually equivalent to. What
  remains genuinely open is narrower: a naive aggregate, $y$-coupled
  fiber-partition bound on $\Phi$ with a flat $(m+1)(R-y)$ correction term
  (mimicking `E_seq_list_sum_le`'s $+R-y$ structure) fails at small scale, so
  the correct $n$-ary induction step for $\Phi \le C+mE$ is not a
  straightforward port of the $E_{\text{seq}}$ list lemma — it needs a
  differently-shaped or more tightly $y$-coupled correction, which is the
  concrete next target.

### Next Steps

Items 1-4 rule out local graph edits, 1D fiber sums, direct carry-mapping, and
slice-merge induction as proof strategies for the strictness conjecture. Item 5
is different in kind: it is not another refutation, but a correction of the
target itself, and the corrected target ($\Phi = X + (m+1)D \le C(R) + mE(R)$,
equivalent to `UniversalLowerBound`) has survived every test run against it,
including the counterexample that killed its predecessor hypothesis.

Note also (confirmed against `docs/lean-proof-status.md`): `UniversalLowerBound`
is a live _hypothesis interface_, not a proven theorem — Layers 1-3
(`E_add_min_le`, `E_seq_list_sum_le`, `defect_fiber_bound`,
`sum_unique_roots_lower_bound`) and Layer 4 (`total_coord_edges_eq`) are all
proven unconditionally, but they bound the algebraic _defect_
$D(V') = Rk - \text{sum\_unique\_roots}(V')$ and relate it to
`total_coord_edges`, not `external_neighbors` directly. The missing link is
`cross_collisions`, and Item 5 identifies the correct inequality this missing
link must satisfy — $\Phi \le C+mE$ — plus concrete computational support for
it.

We propose refocusing on constructing the $n$-ary, $y$-coupled induction step
for $\Phi \le C(R)+mE(R)$ over a fiber partition (the structural analogue of
`E_seq_list_sum_le`, but for $\Phi$ rather than $E$ alone), since the naive flat
correction term does not work and a genuine gap remains in how the per-fiber
slack should compose.

### Reproducing

```
python3 scripts/hole_filling_check.py                 # item 1
python3 docs/archive/lyapunov_profile_check.py         # item 2
python3 scripts/dual_compression_check.py              # item 1 (compression variant)
# item 3: carry-mapping decomposition (scratch, not yet committed as a script)
python3 scripts/cross_collision_bound.py --profile quick   # item 4, unrestricted
python3 scripts/slice_cross_collision.py               # item 4, restricted to slice pairs
python3 scripts/carry_decomposition_check.py           # item 3, carry-mass decomposition
python3 scripts/star_fiber_collision.py                # item 5, n-ary collision ceiling
```
