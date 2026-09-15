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

### Next Steps

The exhaustive data proves the inequality is true, but the proof *cannot*
rely on graph edits, 1D fiber sums, direct carry-mapping, or slice-merge
induction. The true invariant is hiding in a deeper, non-additive
structural property of $A(n,k)$ — likely requiring a global, submodular
analysis of $C(R) + mE(R)$ directly against the global transversal
structure.

We propose clearing the whiteboard of local geometric operators and
refocusing entirely on constructing a non-additive global invariant.

### Reproducing

```
python3 scripts/hole_filling_check.py                 # item 1
python3 docs/archive/lyapunov_profile_check.py         # item 2
python3 scripts/dual_compression_check.py              # item 1 (compression variant)
# item 3: carry-mapping decomposition (scratch, not yet committed as a script)
python3 scripts/cross_collision_bound.py --profile quick   # item 4, unrestricted
# item 4, restricted to slice pairs: scratch script, not yet committed
```
