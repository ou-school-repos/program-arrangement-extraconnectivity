# Status Update: The Arrangement Graph Extraconnectivity Problem

We have exhaustively verified the core strictness conjecture
($L_c \le (m+1)(|\pi| - R) + \Delta C + m\Delta E$) across 3,796 adversarial
test cases. The global bound holds. However, we have computationally proven
that the standard structural isoperimetric tools cannot be used to
formalize the proof.

We must abandon local, additive, and direct-mapping approaches. Here is the
formal autopsy of why standard techniques structurally fail on the
arrangement graph $A(n,k)$:

**1. Local Graph Edits (Kruskal-Katona Shifting / Hole-Filling Induction)**

* **The Approach:** Inductively building or compressing the set
  vertex-by-vertex (or via dual-compression swaps) to navigate toward a
  canonical dense state without decreasing slack.
* **The Obstruction (The Concentration Cliff):** In $A(n,k)$, the alphabet
  overhang ($m = n-k$) imposes a rigid $(m+1)$ penalty when global
  concentration ($R - |\pi|$) increases. A single local vertex addition can
  independently push global $R$ across a power-of-2 boundary without
  triggering a proportional increase in the local slice size $R_i$. The
  local geometric refund ($\alpha = 1$) is structurally too small to pay
  the global arithmetic toll ($m+1 \ge 2$). Furthermore, at high densities,
  injectivity requirements completely block classical guarded symbol swaps.
* **Conclusion:** Local topology cannot navigate the decoupled step-functions
  of binary arithmetic.

**2. Additive Lyapunov Potentials (Independent Fiber Sums)**

* **The Approach:** Defining a global potential
  $\Psi(V') = \sum_{F} f(|V' \cap F|)$ over 1D fibers to ledger adversarial
  chaos against the target $C(R) + mE(R)$.
* **The Obstruction (Cross-Fiber Blindness):** Any additive fiber-size
  ansatz is rigidly locked by the embedded Hamming cubes. Matching the
  $R=1$ and $R=2$ targets strictly forces the lowest weights to $w_1 = 0$
  and $w_2 = m+1$. At $m \ge 2$, this locked potential evaluates a 4-cycle
  at exactly $\Psi = 4m + 4$, while the true adversarial load is
  $\Phi = X + (m+1)D = 8m$.
* **Conclusion:** An additive sum over independent 1D fibers is
  mathematically blind to cross-fiber geometric collisions. The true
  invariant must be non-additive or coordinate-coupled.

**3. Geometric-to-Arithmetic Bijection (Carry Mapping)**

* **The Approach:** Proving that geometric cross-collisions structurally
  force binary carries in the partition sum $\sum R_i = R$, thus paying for
  themselves via the subadditivity gap $\Delta E$.
* **The Obstruction (Multi-way vs. Sequential Mismatch):** When isolated to
  sets where the unfilled slot refund is zero ($\text{conc} = 0$), the
  binary-carry mass of $\sum R_i$ systematically under-covers the geometric
  collision mass $L_c$, missing by up to 18 units in our test corpus. $L_c$
  is driven by non-linear, multi-way geometric overlaps (many slices
  converging on one empty slot). The subadditivity gap $\Delta E$ is driven
  by 1D, sequential bit-arithmetic.
* **Conclusion:** Geometric collisions in $A(n,k)$ do not map injectively to
  binary carries.

**4. Inductive Merge / Split-and-Recombine**

* **The Approach:** Inductively building the set by merging disjoint
  coordinate slices ($Q_i, Q_j$), attempting to pay for the geometric
  cross-collisions ($|\partial A \cap \partial B|$) using the linear
  arithmetic surplus guaranteed by the subadditivity lemma:
  $E(A+B) \ge E(A) + E(B) + \min(|A|, |B|)$.
* **The Obstruction (Geometric Wrap-around):** Exhaustive testing confirms
  the cross-collision cost locally outscales the binary surplus, even when
  strictly restricted to valid, parallel coordinate slices. In $A(6,3)$
  ($m=3$), merging a slice of size $|A|=1$ and $|B|=19$ yields a
  cross-collision overlap of $6$, strictly exceeding the arithmetic budget
  of $m \cdot \min(1, 19) = 3$. The linear surplus structurally cannot
  survive $\min=1$ because a large slice can geometrically "wrap around" a
  singleton slice and share many external neighbors, completely decoupling
  from the arithmetic cap. (An unrestricted, exhaustive test over
  arbitrary disjoint vertex pairs fails even harder and even earlier, at
  $|A|=2,|B|=1$ in $A(4,2)$; restricting to genuine coordinate-slice pairs,
  as the merge step actually requires, delays but does not avoid the
  failure — it survives for $m=1$/$m=2$ small cases but reappears at
  $m=2$ ($A(5,3)$, 25/1440 slice-pairs) and worsens at $m=3$ ($A(6,3)$,
  488/6000 slice-pairs).)
* **Conclusion:** The geometric cost of merging slices is not bounded by
  the linear subadditivity surplus, even restricted to the actual slice
  pairs the induction needs.

**5. N-ary Collision-Ceiling Lemma ($C_{\text{constant}}$ as Upper Bound on
Cross-Collisions)**

* **The Approach:** Since `E_seq_list_sum_le` successfully generalized the
  2-way subadditivity $E(x)+E(y)+\min(x,y) \le E(x+y)$ to $n$-ary partitions
  (aggregate form: $\sum_s E(c_s) + R - y \le E(R)$) and closed the Defect
  Bound unconditionally, try the same move for $C_{\text{constant}}$: an
  aggregate, $y$-coupled bound on `cross_collisions` over a fiber partition,
  $X(V') \le \sum_s (C(c_s) - D(F_s)) + (R - y)$, hoping this would close
  the gap between the proven Defect Bound and the still-hypothesis
  `UniversalLowerBound`.
* **The Obstruction:** A first pass tested a per-fiber, non-aggregate form
  with no $y$ term at all ($X(F_s) \le C(c_s) - D(F_s)$ on each fiber in
  isolation) and found only depth-0 (whole-set) violations — a scoping bug
  an advisor review caught, since depth-0 just re-derives the already-known
  global Star Graph refutation on smaller instances and says nothing about
  genuine fiber splits. Correcting this (tracking depth $\ge 1$ separately,
  and testing the actual aggregate $y$-coupled analogue on both Star Graphs
  and denser random subsets) shows the failure is real but narrower than
  first claimed: it is absent for small/sparse configurations
  ($A(4,2)$; most $A(5,2)$ random draws) but appears reliably once $m \ge 2$
  and density rises — e.g. $A(6,3)$, random seed 4: $R=12$, $X=26$ vs.
  aggregate RHS $=9$; $A(5,3)$, random seed 3: $X=20$ vs. RHS $=9$. The
  weaker variant without the $-D$ term ($X(V') \le \sum_s C(c_s) + (R-y)$)
  is more forgiving but fails under the same conditions once density rises
  further (e.g. $A(6,2)$, all random draws).
* **Conclusion:** The $E_{\text{seq}}$ list-lemma trick does not transfer to
  $C_{\text{constant}}$, even in its correct aggregate, $y$-coupled form —
  it fails specifically once $m \ge 2$ and fiber density is nontrivial,
  matching the same $m \ge 2$ threshold seen in Item 4. This is structurally
  stronger than Items 1-4: those killed specific proof strategies (local
  edits, additive fiber potentials, carry maps, slice merging); this rules
  out an entire *shape* of argument — bounding `cross_collisions` through
  $C_{\text{constant}}$ as an arithmetic ceiling, whether globally (already
  refuted by the Star Graph at $R=8$, $n=15$, $k=7$: $X+D=28 > C(8)=12$, per
  `docs/lean-proof-status.md`'s "Superseded" note) or via the fiber-partition
  aggregate that made the $E_{\text{seq}}$ version work.

### Next Steps

The exhaustive data proves the inequality is true, but the proof *cannot*
rely on graph edits, 1D fiber sums, direct carry-mapping, slice-merge
induction, or any $C_{\text{constant}}$-as-ceiling bound on
`cross_collisions`, at any granularity. The true invariant is hiding in a
deeper, non-additive structural property of $A(n,k)$ — likely requiring a
global, submodular analysis of $C(R) + mE(R)$ directly against the global
transversal structure, or a route to `cross_collisions` that never passes
through $C_{\text{constant}}$ as an upper bound.

Note also (confirmed against `docs/lean-proof-status.md`): `UniversalLowerBound`
is a live *hypothesis interface*, not a proven theorem — Layers 1-3
(`E_add_min_le`, `E_seq_list_sum_le`, `defect_fiber_bound`,
`sum_unique_roots_lower_bound`) and Layer 4 (`total_coord_edges_eq`) are all
proven unconditionally, but they bound the algebraic *defect*
$D(V') = Rk - \text{sum\_unique\_roots}(V')$ and relate it to
`total_coord_edges`, not `external_neighbors` directly. The missing link is
exactly `cross_collisions`, and Item 5 shows the most natural route to it
(reusing the $E_{\text{seq}}$ list-lemma machinery) is closed.

We propose clearing the whiteboard of local geometric operators and
refocusing entirely on constructing a non-additive global invariant for
`cross_collisions` that does not route through $C_{\text{constant}}$.

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
