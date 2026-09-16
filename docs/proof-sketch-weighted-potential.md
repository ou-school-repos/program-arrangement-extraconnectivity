# Proof Sketch: Weighted Fiber-Overlap Inequality

## Unrestricted target refuted (2026-09-16)

The full Star in A(10,8), with center `(0,1,2,3,4,5,6,7)` and all replacements
of one coordinate by 8 or 9, has R=17, m=2, D=16 and X=56. Here E(17)=33,
sbl(17)=54, C(17)=37, so P=103 while Phi=104. Direct enumeration of distinct
external neighbors gives 168, below the claimed lower bound 169. This refutes
the unrestricted target below, not merely a proof strategy.

The full-Star formulas give Phi = m(m-1)j(j-1)/2 + m(m+1)j. For fixed m>1 this
grows quadratically in j, whereas P(1+jm) grows as O(j log j). Thus unrestricted
complete-Star inequalities cannot hold for all dimensions. No nonnegative
singleton-normalized transition potential can repair the false bound in this
regime.

Restricting to m >= ceil(log2 R) is a possible revised conjecture, not an
established safe range. The witness violates that restriction. It beats the
numerical Hamming-ball target; comparison with an actually embedded Hamming ball
requires separate embedding hypotheses.

The sweep script includes small-R rows outside that proposed range, including
A(5,4), R=3,4 and A(5,3), R=5. Earlier small-pool passes therefore cannot be
explained solely by the proposed restriction. In particular, the verified
A(4,3), R<=5 pool includes m=1, R=5. Those bounded passes do not reach this R=17
obstruction. The script's scheduled rows and cap-dependent skips are not
themselves execution logs.

## Historical target (false without restrictions)

For all V' ⊆ A(n,k) with |V'| = R and k ≤ n:

\[ X(V') + (m+1) D(V') \le C(R) + m \cdot E(R) \]

where m = n−k, X = cross-collisions, D = defect, C = C_constant, E = E_seq.

Equivalently (via the fiber identity |∂V| = Rkm − X − (m+1)D):

\[ |\partial V'| \ge (Rk − E(R)) \cdot m − C(R) \]

The Hamming ball achieves equality in both forms.

## Why Standard Compression Fails

The guarded symbol compression `compressSet V' a b` shifts symbol b → a across
all vertices, guarding against collisions. In A(4,2), shifting 3 → 1 in {[4,3],
[1,3]} produces {[4,1], [1,3]} and raises the boundary from 5 to 7. The weighted
potential X + (m+1)D falls from 3 to 1.

The failure is structural: in A(n,k), the injectivity constraint means moving a
vertex from one fiber to another can destroy an existing fiber member's
uniqueness, creating new collisions at a different coordinate. The boundary
increase is a global consequence of a local move that ignores the
multi-coordinate fiber structure.

This counterexample is verified and documented in
`IsoperimetricPartialPermutation.lean` and `universal-lower-bound-work.md`.

## Proposed Approach: Coordinate-Root Compression via H(k,n)

### Step 1: Lift to the Hamming Graph

A(n,k) embeds naturally in the Hamming graph H(k,n) = K_n □ ... □ K_n (k
copies). Vertices of H(k,n) are all k-tuples over {0,...,n−1} (no injectivity
constraint). The arrangement graph is the induced subgraph on injective tuples.

The fiber structure extends: at coordinate p, the root is the (k−1)-tuple
obtained by deleting position p. Two vertices share a root iff they differ at
exactly position p.

### Step 2: Known Vertex-Isoperimetric Result for H(k,n)

**For the binary case (n=2):** Harper's Edge Isoperimetric Theorem
\cite{harper1966optimal} states that among all R-element subsets of the
d-dimensional hypercube Q_d, the initial segment of colex (binary lexicographic)
order minimizes the edge boundary. This is proven in Lean as
`harpers_edge_isoperimetry` in `ArrangementGraphUtils.lean`. By the handshake
lemma, minimizing the edge boundary is equivalent to maximizing internal edges,
which is equivalent to maximizing E_seq(R).

**For the n-ary case (n>2):** The correct reference is Bollobás and Leader
\cite{bollobas1991vertex}, who proved that for the n-ary Hamming graph H(k,n),
the initial segment of colex order minimizes the vertex boundary |∂S| = |N(S) \
S|. This is a genuinely harder result than Harper's theorem — it requires a
compression argument specific to n-ary alphabets, not a one-line corollary of
Kruskal-Katona. The Kruskal-Katona theorem itself applies to uniform set systems
(binary layers of the hypercube), not to n-ary Hamming graphs directly.

**The vertex-isoperimetric profile of H(k,n)** is known: for each R, the minimum
vertex boundary is achieved by a colex-initial segment, and the minimum value is
determined by the "profile function" of H(k,n). This is the result we would need
to transfer to A(n,k).

### Step 3: The Transfer Problem (This Is the Hard Part)

The natural approach is to show that the vertex-isoperimetric inequality for
H(k,n) implies the weighted inequality for A(n,k). However, this transfer is not
straightforward — it is the core open problem, not a minor technical step.

The difficulty: for V' ⊆ A(n,k), the fiber structure in A(n,k) is a
**restriction** of the fiber structure in H(k,n), but the restriction is not
benign. In H(k,n), each root at position p has fiber size (n−1)!/(n−k+1)!. In
A(n,k), each root has the same full fiber size (since the root is a (k−1)-tuple
of distinct symbols, extending it with a fresh symbol at position p gives n−k+1
choices). So the fiber sizes are actually identical — the difference is in which
vertices of those fibers are **present in V'**.

The injectivity constraint creates a coupling between coordinates that does not
exist in H(k,n): moving a vertex at position p changes its root at every other
position q ≠ p. This "coordinate tangling" is the source of the X/D interaction
that the weighted potential must resolve. A clean transfer inequality would need
to quantify how much the injectivity constraint can degrade the boundary below
the H(k,n) optimum, and show that this degradation is exactly compensated by the
(m+1)D term. No such inequality is currently known.

**What the Cheng et al. literature provides:** Cheng, Lipták, and Tian
\cite{cheng2022extraconnectivity} computed small-g extraconnectivity cases (g
≤ 6) by exhaustive computer search. Their ad-hoc topological constructions turn
out to be Hamming balls. They do NOT prove a general vertex-isoperimetric
theorem for A(n,k) — their contribution is computational, not structural. The
general inequality we need is not in their paper.

### Step 4: The Weighted Potential and Root Compression

Since vertex compression in A(n,k) fails (Step 1), and transfer from H(k,n) is
open (Step 3), a direct approach on the root structure of A(n,k) may be more
tractable.

Define a **root compression** at position p: given two roots r₁ `<colex` r₂, if
the r₂-fiber contains more V'-members than the r₁-fiber, move one vertex from
the r₂-fiber to the r₁-fiber by changing its symbol at position p.

**Properties of root compression:**

1. **Preserves |V'|**: we move a vertex, not copy it.

2. **Preserves injectivity**: the moved vertex v has symbol b at position p. We
   change it to symbol a, where a is determined by the target root r₁. Since r₁
   is a (k−1)-tuple of distinct symbols, the extension symbol a (the unique
   value at position p in any vertex with root r₁) is one of the n−k+1 symbols
   not appearing in r₁. Since v currently uses k distinct symbols and r₁ uses
   k−1 distinct symbols, and n − k + 1 ≥ 1, the symbol a is available and
   distinct from all symbols in r₁. The resulting vertex has k distinct symbols,
   so injectivity is preserved.

3. **Does not decrease X** (needs proof): moving a vertex into a fiber that
   already has more members should not reduce the total collision count. This is
   plausible because the moved vertex joins a larger fiber, creating at least as
   many new collisions as it destroys in the old fiber. However, the moved
   vertex's roots at other positions q ≠ p also change, which could affect X at
   those positions. The net effect on X is not obvious and requires analysis.

4. **Does not decrease D** (needs proof): D = Rk − |unique roots|. The moved
   vertex may create a new shared root at position p (increasing D) but may also
   eliminate a shared root at position q ≠ p (decreasing D). The net effect on D
   is not obvious.

The claim — **not yet proved** — is that the net effect on X + (m+1)D is
non-negative. If true, iterating root compressions at every coordinate converges
to a configuration where roots are in colex order with maximal fiber occupancy,
which should be the Hamming ball.

### Step 5: Why This Is Hard (Multi-Coordinate Interaction)

The root compression concentrates vertices in fewer fibers at position p, which
locally increases X and D at that coordinate. But the moved vertex changes its
symbol at position p, which changes its root at every other position q ≠ p. This
can:

- Create new shared roots at some positions q (increasing X and D there)
- Destroy shared roots at other positions q (decreasing X and D there)

The Hamming ball avoids this problem because it is "aligned" — all vertices
share the same first d coordinates, so the fiber structure is nested across
positions. A general configuration is not aligned, and root compression at one
position can disrupt the structure at others.

The key obstacle is that X and D are **global** quantities (summed over all
coordinates), but root compression is a **local** operation (at one coordinate).
The multi-coordinate interaction means the local benefit at position p can be
offset by damage at other positions.

### Partial-Layer Case

When R does not exactly fill a colex-initial segment of a "layer" (vertices with
the same number of 1-bits in the binary case, or the same weight class in the
n-ary case), the Hamming ball for size R is a partial layer: it fills complete
layers below and takes an initial segment of the next layer. Standard
isoperimetric proofs (Harper, Bollobás–Leader) handle this via a tie-breaking
argument: within each layer, colex order is used, and the partial layer is an
initial segment of colex within its layer.

For the weighted inequality, the partial-layer case matters because the Hamming
ball's fiber structure changes qualitatively at layer boundaries. At R = 2^d (a
full hypercube), the Hamming ball has X = 0 and D = E(R) maximal. At R = 2^d + 1
(one vertex into the next layer), X jumps to a positive value and D drops. The
weighted potential X + (m+1)D must track these discontinuities.

The exhaustive evidence suggests the inequality remains tight at layer
boundaries (e.g., A(7,2) R=4 = 2^2 is tight, A(7,2) R=5 is slack 1). A proof
would need to handle the partial-layer transition explicitly, showing that the
weighted potential does not drop below the bound during the transition.

### Possible Resolution Strategies

1. **Coordinate-by-coordinate with repair**: compress at position p, then repair
   the damage at positions q ≠ p by further compressions. Show that the total
   effect across all coordinates is non-negative for the weighted potential.

2. **Global Lyapunov function**: define a potential that is monotone under root
   compression at every coordinate simultaneously (e.g., lexicographic order on
   the multiset of fiber sizes across all coordinates). Show that the Hamming
   ball is the unique maximum. The challenge is finding a Lyapunov function that
   actually works — the natural candidates (fiber-size multisets) may not be
   monotone due to the multi-coordinate interaction.

3. **Induction on R with partial-layer tracking**: for R = 1, trivial. For R >
   1, remove a vertex and use the induction hypothesis. Show that adding the
   vertex back preserves the bound, tracking the partial-layer transition
   explicitly.

4. **Direct counting**: decompose X(V') + (m+1)D(V') into a sum over coordinates
   and roots, and bound each term. This avoids compression entirely but requires
   a new combinatorial identity.

5. **Contrapositive approach**: instead of showing the Hamming ball maximizes
   X + (m+1)D, show that any configuration with X + (m+1)D > C(R) + m·E(R) would
   violate some known constraint (e.g., the defect bound D ≤ E(R), or the fiber
   identity itself).

## Whiteboard Sketches for Strategies 2 and 3 (2026-09-12)

Root compression (Step 4) has stalled on the coordinate-tangling obstacle (Step
5): a local move at position p unavoidably scrambles roots at every q ≠ p, and
no argument has bounded that scatter damage tightly enough to recover
monotonicity. Rather than keep pushing on that specific operator, here are
informal starting sketches for Strategies 2 and 3 from the list above. **Neither
is worked out to the point of being checkable, let alone a proof — both are
starting points for further whiteboard work, recorded here so the next pass
doesn't restart from nothing.**

### Strategy 3 sketch: Marginal induction with partial-layer tracking

This reuses the exact skeleton already used to prove `Theorem~\ref{thm:defect}`
(`D(V') ≤ E_seq(R)`, proven by strong induction on R via
`sum_unique_roots_lower_bound` and the subadditivity lemma `E_add_min_le`),
extended to carry the joint quantity Φ = X + (m+1)D instead of D alone.

Proceed by strong induction on R. Strip a vertex v from V', apply the inductive
hypothesis to V' \ {v} (size R−1), then bound the marginal change Δ Φ = ΔX +
(m+1)ΔD when v is added back.

1. **Marginal defect.** v brings k roots. Let s = the number of these roots
   already populated by V' \ {v} (shared roots), so k−s are fresh. By
   definition, ΔD = s.

2. **Marginal collisions.** v contributes at most (k−s)(n−k) new external edges
   from its fresh roots (0 from shared roots, since those don't expose new
   boundary). Some of these edges may land on vertices already in the external
   boundary of V' \ {v}; each such overlap adds 1 to ΔX instead of exposing a
   genuinely new boundary vertex.

3. **Target bound for the inductive step.** To close the induction, we'd need

   ```text
   ΔX + (m+1)s ≤ ΔC(R) + m·Δ E_seq(R)
   ```

   where Δ E_seq(R) = popcount(R−1) (from the defining recurrence of E_seq) and
   ΔC(R) is the corresponding marginal change in C_constant.

4. **Partial result on ΔX (2026-09-12, derived and hand-checked, not yet
   Lean-formalized).** Write `total_coord_edges` for the quantity
   `Σ_w bd_mult V' w` over external w. Since each of the U used roots at a
   coordinate contributes (m+1) minus its own occupancy, and occupancies sum to
   Rk, `total_coord_edges = U(m+1) − Rk`; combined with the proved fiber
   identity `|∂V'| = Um − D − X` (using U = Rk − D), this gives
   `total_coord_edges = |∂V'| + X`, i.e. **X = total_coord_edges − |∂V'|**.

   Adding v with s shared roots: ΔU = k−s, ΔR = 1, so
   `Δ(total_coord_edges) = (k−s)(m+1) − k = (k−s)m − s`. Separately, Δ|∂V'| ≥
   −[s>0] (the only way the boundary can shrink is v itself dropping out of it,
   which requires s>0, and by at most 1 — every other effect of adding v can
   only add new external vertices). Substituting:

   ```text
   ΔX = Δ(total_coord_edges) − Δ|∂V'| ≤ (k−s)m         (all s, since the
                                                          s=0 and s>0 cases
                                                          both reduce to this)
   ```

   This matches the maximal ΔX = km observed in the s=0 double-clique stress
   test in A(6,2) (R: 8→9, k=2, m=4, ΔX=8=km exactly), so the bound is tight,
   not just an upper estimate.

   A second, dimension-independent cap: only vertices of V'\{v} at Hamming
   distance exactly 2 from v can border one of v's fresh-root fibers, and each
   such vertex differs from v in exactly 2 coordinates, so it can border at most
   2 fresh fibers. With N₂ = #{u ∈ V'\{v} : d(u,v)=2} ≤ R−1, this gives ΔX ≤
   2(R−1) independent of k and m. **The "exactly 2" claim above needs a more
   careful check** — a distance-2 vertex's second differing coordinate could in
   principle coincide with a _shared_ root's coordinate rather than a fresh one,
   which would need ruling out before this half is treated as proved.

   **What remains.** The bound ΔX ≤ (k−s)m only closes the induction if
   `(k−s)m ≤ Δ C(R) + m·Δ E_seq(R) − (m+1)s` for all valid s, i.e. if
   `(k−s)m + (m+1)s ≤ ΔC(R) + m·popcount(R−1)`, i.e.
   `km + s ≤ ΔC(R) + m·popcount(R−1)`. This must hold uniformly in s up to k,
   which is the actual remaining obstacle — the ΔX bound above is now solid, but
   plugging it into the target inequality has not yet been checked against
   ΔC(R)'s actual formula.

   **The (k−s)m bound is checked and fails as a standalone closer.** Plugging
   R=1→2 into A(10,5) (k=5, m=5, s=k−1=4 since the two connected vertices share
   k−1 roots): LHS = km+s = 29, RHS = ΔC(2)+m·popcount(1) = 1+5 = 6. 29 ≤ 6 is
   false, so `(k−s)m` alone cannot close the induction at small R with large k,
   m.

   **Proven exact fact for this case: ΔX = 0 for any two adjacent vertices, in
   general (not just this example).** If external w borders v via coordinate p
   and borders u via a different coordinate q, then w agrees with u everywhere
   except q, forcing w_p = u_p; but w disagrees with v at p, forcing u_p ≠ v_p.
   Symmetrically, w agrees with v everywhere except p, forcing w_q = v_q, and w
   disagrees with u at q, forcing v_q ≠ u_q. So u and v differ at both p and q —
   contradicting that adjacent vertices differ at exactly one coordinate. Hence
   p = q, i.e. any two adjacent vertices contribute to X only through a shared
   coordinate, never a cross-coordinate collision: **ΔX = 0 whenever v is
   adjacent to the vertex it's added next to**, independent of n, k, R. This is
   a genuine, reusable lemma (worth Lean-formalizing on its own), not just an
   artifact of the R=2 example.

   **Conjectured second cap (not yet proved as a general trade-off).**
   Separately, ΔX ≤ 2·N₂ where N₂ = #{u ∈ V'\{v} : d(u,v)=2} ≤ R−1 (see above);
   this "Source Cap" is small when V'\{v} is sparse near v. The working
   conjecture is that ΔX is always bounded by `min((k−s)m, 2N₂)`, and that
   whichever cap is small enough to be the binding one always keeps
   `ΔX + (m+1)s ≤ ΔC(R) + m·popcount(R−1)` satisfied — i.e., that a
   configuration cannot simultaneously have large (k−s)m (small s, so cheap ΔD)
   _and_ large N₂ (many distance-2 neighbors near v, which should itself require
   existing structure that already costs D_old/X_old budget). **This trade-off
   has not been proved.** Only two data points support it so far — the sparse
   R=2 case (Source Cap ≈ 0 saves it) and the dense R=9 clique-trap (Volume Cap
   is tight but RHS has banked enough slack from D_old to absorb it). No
   mid-range example (moderate R, k, m, with neither cap trivially small) has
   been checked; that is the natural next stress test before treating the
   crossover as anything more than a naming convention for two observed cases.

This approach's main appeal is that steps 1–2 reuse verified machinery directly;
the ΔX bound in step 4 is now a checked derivation with one fully proved special
case (ΔX=0 at adjacency) and one still-conjectural general trade-off, but
closing the induction for all R, s still requires either proving that trade-off
or finding a counterexample to it.

### Strategy 2 sketch: Global Lyapunov function on fiber multisets (refuted by LP)

1. **State space.** Let 𝓕(V') be the multiset of nonzero fiber sizes across all
   k coordinates: 𝓕(V') = ⋃ₚ {|`F_p,r`| : r ∈ U_p(V')}.

2. **Candidate potential.** Since D(V') = Rk − |𝓕(V')|, a natural candidate is
   Φ(V') = Σₚ Σᵣ f(|`F_p,r`|) for a convex f (e.g. f = E_seq or f(c) = C(c,2)).
   Convexity would make concentrating vertices into fewer, larger fibers — which
   the Hamming ball maximizes — strictly increase Φ.

3. **Single-operation accounting.** For one root-compression move (vertex v
   shifted from a smaller to a larger fiber at coordinate p): the _local_ gain
   at p is positive by convexity of f. The _collateral_ effect at every other
   coordinate q ≠ p, where v's root also changes, could go either way; worst
   case v lands in k−1 brand-new singleton fibers, each contributing f(1) at
   those coordinates.

4. **What would need to be shown.** (a) Φ actually lower-bounds X(V') +
   (m+1)D(V') in a form matching the target inequality — this link has not been
   established, only motivated informally via D; (b) the local gain at p
   strictly dominates the worst-case collateral loss summed over q ≠ p, for
   every possible compression move. Neither (a) nor (b) has been attempted
   formally.

**LP refutation (2026-09-15).** The additive convex potential Ψ_f(V') = Σ_s
N_s(V') w_s, where N_s counts coordinate roots with exactly s members, was
tested via a normalized feasibility LP (`scripts/lyapunov_profile_check.py`).
Constraints: (i) Ψ(H_R) = C(R)+mE(R) for every embedded Hamming cube that fits,
(ii) Φ(V') ≤ Ψ(V') ≤ C(R)+mE(R) on the full adversarial corpus, (iii) discrete
convexity of w. Results:

- A(5,3) and A(6,3): the Hamming-ball equalities alone are algebraically
  inconsistent — R=1 forces w_1=0, R=2 forces w_2=3 (or 4), but R=3 then
  requires 2w_2 = 7 (or 9), which contradicts.
- A(4,3): equalities admit (w_1,w_2) = (0,2), but the corpus set {012, 023}
  gives Φ=1, Ψ=0, violating the lower cap.

**Structural 4-cycle kill (no computation needed).** Even without the LP, a
purely algebraic argument refutes the entire additive family. Anchor the
weights: R=1 forces k·w_1 ≤ C(1)+mE(1)=0, so w_1=0. R=2 forces w_2 ≥ Φ(R=2)=m+1
and w_2 ≤ C(2)+mE(2)=m+1, so w_2=m+1 exactly. Now consider a 4-cycle (square of
4 vertices) in A(n,k): its profile has N_2=4, all other active fibers size 1.
The locked potential evaluates Ψ = 4w_2 = 4(m+1). But the adversarial load is Φ
= X + (m+1)D = 4(m-1) + 4(m+1) = 8m. The sandwich Φ ≤ Ψ requires 8m ≤ 4m+4, i.e.
m ≤ 1. For m ≥ 2 (the only interesting range), every 4-cycle is a
counterexample. The root cause: a purely additive function over individual
fibers is blind to cross-fiber collisions — two fibers of size 2 contribute the
same w_2 whether they are parallel (X=0) or locked in a 4-cycle (X=4m-4). The
potential must know about fiber intersections to pay for X.

**Status:** The _additive independent 1D fiber sum_ potential is **refuted**.
The embedded Hamming cubes mathematically lock the minimum weights, rendering an
independent sum blind to cross-fiber geometric collisions (demonstrated by the
LP refutation and the 4-cycle counterexample above).

### Live: Recursive profile-state potential G(state) (finite certificates, 2026-09-16)

This is separate from the refuted additive ansatz. For a set `V`, let `state(V)`
be the sorted multiset of its per-coordinate sorted fiber-size profiles. At a
coordinate split, let `gap` be the arithmetic surplus `P(R) - sum P(|F_s|)`
minus the geometric overhead `Phi(V) - sum Phi(F_s)`. A proposed recursive
correction is a nonnegative, profile-only integer function `G` satisfying, for
at least one coordinate of every non-singleton set,

```text
G(state(V)) <= gap + sum G(state(F_s)).
```

`src/profile_telescope_milp.cpp` checks this finite feasibility problem with
OR-Tools CP-SAT. Its `exact-menu` mode is exact within the stated range: it
merges concrete sets only when they have both the same parent state and the same
complete menu of `(gap, child-state)` split options.

Exact-menu feasibility was obtained for all subsets in `A(4,2)` through `R=5`
(1,585 subsets), `A(5,2)` through `R=5` (21,699 subsets), and `A(4,3)` through
`R=7` (536,154 subsets). A minimized run on all `A(4,3)` subsets through `R=8`
(1,271,625 subsets and 6,022 distinct menus) returned `sum G = 0`: every set in
that finite cell has a coordinate whose raw gap is nonnegative.

That zero-correction outcome is not universal. Capacity-valid Star sets with
every available raw coordinate gap negative occur in `A(7,4)` at `R=11`, with
gaps `(-1,-1,-1,-1)`; in `A(8,4)` at `R=17`, with `(-2,-2,-2,-2)`; and in
`A(8,5)` at `R=15`, with `(-2,-3,-3,-3,-3)`. Thus choosing the best raw
coordinate is false in general. Whether a nonzero recursive profile correction
works for all parameters, and what closed form it has, remains open. No Lean
theorem or axiom status changes follow from these finite certificates.

### Exact coordinate-split accounting (2026-09-16)

The defect framework now proves two independent accounting lemmas:

```text
DeltaD(V,p) = |V| - |U_p(V)|
DeltaX(V,p) = #{w in external_boundary(V): mu_V(w) >= 2,
                                          w in directional_boundary_p(V)}
```

Multiplicity counts coordinate directions, not adjacent vertices. A vertex
internal to a sibling fiber can be reached from another child only along the
split coordinate, so it contributes no child collision excess. The proof
explicitly accounts for these child-external, parent-internal vertices.

`scripts/verify_split_identities.py` independently computes the increments by
parent-minus-child subtraction and checks the displayed formulas for every
coordinate, including trivial splits:

| Pool           | Non-singleton sets | Coordinate checks | Mismatches | All-negative sets |
| -------------- | -----------------: | ----------------: | ---------: | ----------------: |
| A(4,3), R <= 5 |             55,430 |           166,290 |          0 |                 0 |
| A(5,2), R <= 6 |             60,439 |           120,878 |          0 |                 0 |

Reproduce with
`python3 scripts/verify_split_identities.py --n 4 --k 3 --max-r 5` and the
corresponding `--n 5 --k 2 --max-r 6` invocation. These finite checks
corroborate the implementation; the universal identities are justified by the
paper proofs. The pools do not include the larger all-negative Star witnesses
recorded above.

A split-tree inequality using exact terminal slack is not a new proof:
`sum DeltaPhi = Phi(V) - sum Phi(leaves)` and
`sum DeltaP = P(|V|) - sum P(|leaves|)` telescope. Adding terminal slack
therefore reduces precisely to the desired universal bound. For singleton
leaves, both totals are independent of the tree. Split selection redistributes
local deficits but cannot create aggregate budget.

The recursive G framework remains live, but no universal nonnegative
profile-only correction or independent charging theorem is established. The
additive strategies remain refuted; a failed candidate bound alone does not
prove that state enrichment is necessary.

### Separated collision slack fails on Star children (2026-09-16)

The candidate `G(F) = C(|F|) - E(|F|) - X(F)` is negative on the very Star
children proposed to carry credit. Direct evaluation using `full_star`,
`cross_collisions`, `defect`, `e_seq`, and `sbl` from
`scripts/full_star_spine.py` gives:

| Cell   | Full-Star child |   C |   E |   D |   X | C-E-X | (m+1)(E-D) | P-Phi |
| ------ | --------------- | --: | --: | --: | --: | ----: | ---------: | ----: |
| A(7,4) | S_3, R=10       |  19 |  15 |   9 |  18 |   -14 |         24 |    10 |
| A(8,4) | S_3, R=13       |  27 |  22 |  12 |  36 |   -31 |         50 |    19 |
| A(8,5) | S_4, R=13       |  27 |  22 |  12 |  36 |   -31 |         40 |     9 |

Thus this candidate fails nonnegativity. Clamping it to zero also fails to
provide credit at these children. In particular, the coupled slack of the
size-13 child in A(8,4) is **19**, not 17. The quantity `E-D` is a defect
difference, not an internal-edge deficit: root occupancy contributes `q-1` to D
but `binom(q,2)` internal edges.

The identity `P-Phi = (m+1)(E-D) + (C-E-X)` explains the positive total in these
examples. It does not establish a nonnegative total for arbitrary sets. These
calculations refute the specific separated candidate; they do not prove that
every possible invariant must explicitly depend on both D and X.

Credit requirements remain conditional on split choice. The size-11 A(7,4)
Star's split into sizes 10 and 1 requires `G(S_3) >= 1` if that split is used.
Its finite exact-menu model also permits the unit of credit at a size-8 child
instead.

An unconditional proof of `0 <= G(V) <= P(|V|)-Phi(V)` would already prove the
target bound. Merely defining a nonnegative graph counter does not supply the
second inequality. For the inductive route, the independent obligation remains
to prove nonnegativity, singleton normalization, and
`G(V) <= gap(V,p) + sum G(F_{p,s})` for some nontrivial split of every
non-singleton set. The displayed Star slack values are constraints for candidate
testing, not a construction of such an invariant.

### Strategy 4: Tested guarded dual root-compression candidate (refuted)

Inspired by Pinto's proof of the Bollobás-Leader directed-path conjectures
(arXiv:1504.07079), which resolves an analogous single-operator-fails obstacle
on the hypercube Q_n. Pinto defines two compression operators C_i (push down:
`C_i(S) = {x ∈ S : x∖{i} ∈ S}`) and D_i (push up:
`D_i(S) = S ∪ {x : x∪{i} ∈ S}`) on subsets of Q_n. Neither is individually
monotone for the directed edge/vertex boundary, but he proves
`|∂→(S)| ≥ ½(|∂→(C_i(S))| + |∂→(D_i(S))|)`, which forces at least one of the two
to be no worse than S even though neither is unconditionally so. Iterating over
i = 1..n converges to a down-set with boundary no larger than the original.

This is structurally the same shape of obstacle as A(n,k)'s root compression:
the single guarded-symbol compression already tried and refuted (`compressSet`,
A(4,2) counterexample raising boundary 5→7) is one operator that individually
fails, exactly as Pinto's C_i and D_i individually fail on some sets in Q_n. The
concrete cardinality-preserving candidate below was tested. It is refuted as a
universal mechanism; this does **not** rule out a different dual pair of
operators respecting injectivity.

This approach's appeal is that it directly targets the coordinate- tangling
obstacle (Step 5) that stalled the original single-operator compression, using a
technique proven to work around the analogous single-operator failure on Q_n;
its risk is that the injectivity constraint may block the averaging identity's
proof in a way that has no counterpart in Q_n (Pinto's proof leans on
set-complement symmetry between C_i and D_i that may not survive the "no
repeated symbol" restriction).

**First crucible test (2026-09-12): a candidate pair fails the averaging
inequality, and the test itself was degenerate.** Since |V'|=R is fixed, Pinto's
C_i/D_i (which change |S|) cannot be ported directly; any dual pair must be
cardinality-preserving swaps. Candidate pair tried, keyed to a symbol pair (a,b)
exactly as in `compressSet`:

- C(V') = the original guarded compressSet: for each v containing b, replace
  b→a; skip (leave v unchanged) if v already contains a.
- D(V') = push-up repair: for each v containing b, if v also contains a (the
  guard-blocked case for C), apply the full transposition (a b) to the whole
  vertex instead of skipping; otherwise same as C.

Tested on the exact refuted-compressSet example, A(4,2), V'={[4,3],[1,3]},
(a,b)=(1,3): C(V')={[4,1],[1,3]} (the known result, Φ: 3→1, boundary 5→7).
D(V')={[4,1],[3,1]}, computed by hand: boundary=5, Φ=3 — unchanged from the
original. Checking the averaging inequality Φ(V') ≤ ½(Φ(C(V'))+Φ(D(V'))): 3 ≤
½(1+3) = 2 is **false**. Pinto's mechanism does not transfer as stated on this
example.

**The D(V')=3 result is not evidence D works — it's a degenerate coincidence.**
Both vertices in this V' contain b=3, so D applied the transposition (1 3) to
_every_ vertex, which is exactly the global graph automorphism π=(1 3) applied
to all of V'. Automorphisms trivially preserve Φ and boundary for any set, so
Φ(D(V'))=Φ(V') here proves nothing about whether D does useful compressive work
— it only shows D degenerated into a no-op symmetry because R=2 and both
vertices shared the swapped symbol. Subsequent non-degenerate tests settle the
candidate pair: see the empirical closure below and
`docs/archive/strategy4-dual-compression-averaging-abandoned.md`.

### Strategy 3's per-step marginal induction is refuted (2026-09-13)

The marginal induction as originally framed — prove a purely local per-step
inequality `ΔX + (m+1)s ≤ ΔC(R) + m·popcount(R-1)` and let strong induction on R
do the rest — is **false in general**, confirmed by an explicit, fully
hand-verified counterexample, not merely a case where the crude ΔX bound was too
generous.

A(5,3) (k=3, m=2), R: 2→3. v=[1,2,3]. `V_old`={u1,u2} with u1=[1,2,4] (sharing
root (1,2) at p=3 with v, so s=1) and u2=[4,5,3] (sharing no root with v).
Verified directly via per-coordinate `coord_boundary` computation (not naive
neighbor-list overlap, which over-counts: two V-members sharing a root both
border the same external point through the **same** coordinate, contributing
bd_mult=1, not 2 — this tripped up the first pass of verification and was caught
and corrected before accepting the result):

- D_old=0, X_old=0 (u1, u2 share no roots and no external neighbors).
- D(V')=1, X(V')=2 (external vertices [4,2,3] and [1,5,3] each have bd_mult=2,
  hit via p=1 from v and p=2 from u2, and vice versa — genuine cross-coordinate
  collisions). Boundary=13, cross-checked against Rkm−Φ = 18−5 = 13.
- Marginal step: ΔX=2, s=1, m=2. LHS = 2+3(1) = 5. RHS = ΔC(3)+m·popcount(2) =
  2+2(1) = 4. **5 ≤ 4 is false.**
- Global check still holds: Φ(V')=5 ≤ C(3)+m·E(3)=7, with slack banked entirely
  from the R=1→2 step (u1,u2 placed at distance 3, costing 0 defect/collision
  there, banking the full ΔC(2)+mΔE(2)=3 available at that step).

**Implication.** This is not a case the crude `(k-s)m` bound merely
overestimates — the actual, correctly-computed ΔX genuinely violates the
per-step target. No tightening of the ΔX bound alone can fix this: the per-step
inequality is false as stated, for a configuration with verified D_old=0. Any
repair must abandon step-locality — e.g. an amortized/potential-method argument
(bank surplus from early steps, spend it on later deficits) rather than
requiring every step to individually satisfy the marginal bound. Note that
proving the amortized (cumulative) version directly is a rephrasing of the
**original** global claim (the per-step terms telescope back into
X(V')+(m+1)D(V') ≤ C(R)+mE(R)), so this refutation removes the main advantage
marginal induction offered — reducing an R-vertex claim to a 1-vertex check —
not just one candidate bound within it.

### Strategy 3b: coordinate-partition induction on Φ (2026-09-13, unresolved)

Distinct from the refuted single-vertex marginal peeling: `thm:defect`'s actual
proof partitions all of V' by symbol at one coordinate p into fibers {F_α},
recurses on each (smaller) fiber, and bounds the recombination via subadditivity
— no vertex is ever peeled off in isolation. Whether this same skeleton closes
for Φ=X+(m+1)D (not just D) was tested and is presently unresolved, with one
confirmed dead end and one open, concretely-scoped question:

1. **A first "verification" was caught as tautological.** Computing the
   recombination penalty (`X_cross`, `ΔD_cross`) **by subtracting the known
   fiber totals from the already-computed** `Φ(V')` makes the identity
   `Φ(V') = ΣΦ(F_α + penalty)` hold by construction for any partition of any set
   — checking it against the target is circular, since it just restates
   `Φ(V')≤target` using an answer already in hand. A real test requires bounding
   the penalty from fiber sizes alone, **before** knowing `Φ(V')`.

2. **A geometric a priori bound was derived and is basically sound.** For
   2-fiber partition `F_a, F_b` (sizes `c_a, c_b`) at coordinate p, a
   cross-collision requires `u ∈ F_a, v ∈ F_b` at Hamming distance exactly 2
   (differing at p and one other coordinate q), which forces `X_cross ≤ c_a·c_b`
   (each pair contributes ≤1) and, from a per-vertex degree argument (each v has
   at most k−1 candidate partners, one per q≠p), also
   `X_cross ≤ (k−1)·min(c_a,c_b)`. The combined bound is
   `min(c_a c_b, (k−1)·min(c_a,c_b))`. An initial claim that this bound grows
   unboundedly with `k` (making the approach "doomed") was itself an error — it
   came from summing the two constraints instead of taking their minimum;
   properly combined, the bound is k-independent for fixed, small fiber sizes.

3. **The real obstacle is an asymptotic scale mismatch, not k-dependence.**
   ΔC(R)+mΔE(R) (the marginal slack for a single 2-way split) is O(log R) — it
   depends only on popcount(R−1) and bitlength(R−1). But c_a·c_b for a roughly
   balanced split is O(R²), and even the refined (k−1)·min(c_a,c_b) bound is
   O(R) for fixed k. Checked numerically at R=20, balanced (10,10) split: slack
   = 3+3m (=6 at m=1), while c_a c_b=100 and (k−1)min bound=40 at k=5 — both far
   exceed the slack. The two small checks that appeared to work (R=3 split
   (2,1), R=4 split (2,2)) do not reveal this, since both are too small for the
   O(R²) vs O(log R) gap to show up.

   **This does not prove Strategy 3b is dead** — it only shows the generic
   combinatorial upper bounds on X_cross (pairwise count, degree count) are too
   loose to confirm the inequality at moderate-to-large R. Whether the **true,
   tightly-argued** worst-case `X_cross` for a genuine adversarial fiber pair
   stays down near O(log R) — the way earlier "crude bound achievable but the
   trade-off saves it" patterns played out elsewhere in this document — has not
   been checked. No explicit adversarial configuration at R≈10–20 has been
   hand-constructed and verified the way every other claim in this document has
   been.

4. **A specific R=20 adversarial construction (k=3) was checked and collapses.**
   F_a = {(1,y,3): y∈Y}, F_b = {(2,z,3): z∈Z}, |Y|=|Z|=10, Y∩Z=∅. All 100 pairs
   are genuinely distance-2, but every pair sharing the same z (or same y)
   generates the **same** collision target — bd_mult counts distinct
   coordinates, not pairs, so 100 raw pair-interactions collapse to X_cross=20,
   comfortably under the slack of 63 at m=20. **This collapse is a property of
   k=3 specifically, not of A(n,k) in general:** with only one non-p coordinate
   available (k=3 means positions {1,2,3}, p=1, only q=2 remains), every pair is
   forced through the same q, which is exactly why they collapse onto shared
   targets.

5. **A k=5 counter-construction shows the collapse is not universal.** Fix a
   base tuple over positions 2–5; let F_a={u_i} and F_b={v_i} for i=2..5, where
   u_i (resp. v_i) is the base with position i replaced by a fresh symbol α_i
   (resp. β_i), and position 1 fixed to a (resp. b). Only matched pairs
   (u_i,v_i) are distance-2 (mismatched i≠j give distance 3, contributing 0),
   and each matched pair uses a **different** q=i, so their targets don't
   coincide. Hand-checked at k=5, n=14, m=9: 4 valid pairs, X_cross=8, no
   collapse — confirmed distinct from the k=3 collapse case.

   **This shows "geometry always collapses collisions" is false as a general
   claim** — it disproves the blanket version of that claim, not merely restates
   the k=3 example. It does **not** show X_cross can be pushed past the slack:
   the one instance checked (k=5, R=8, m=9) gives slack=28, comfortably clear of
   X_cross=8.

6. **Open obstacle for partition induction: bounding X_cross.** The
   recombination step needs X_cross ≤ Δrecombination-slack from fiber sizes and
   m alone. Naive pairwise counting (c_a·c_b, O(R²)) fails — see point 3.
   Whether collisions collapse (point 4) or spread across distinct q's (point 5)
   depends on the specific construction, not on a general law. The k=5 spread
   construction needed roughly O(k) distinct fresh symbols to keep each pair's q
   unique without accidental collapse, which forced m to be large enough to
   supply them (m=9 for k=5 in the instance checked) — **this was only observed
   in the one construction tried, not proved as a necessary trade-off.** It
   remains open whether some other, less alphabet-hungry construction could
   achieve spread at small m, and whether any construction (collapsing,
   spreading, or otherwise) can push X_cross past the available slack at small m
   and large k. No counterexample to the recombination bound has been found in
   any case checked so far, but no proof of the trade-off as a general law
   exists either.

7. **Φ collapses to the boundary — X is not an independent adversarial target.**
   Combining three identities already established above:
   `total_coord_edges = |∂V| + X`, `total_coord_edges = U(m+1) - Rk`, and
   `D = Rk - U`, gives

   ```text
   Φ = X + (m+1)D
     = [U(m+1) - Rk - |∂V|] + (m+1)(Rk - U)
     = mRk - |∂V|
   ```

   The U terms cancel identically. So Φ ≤ C(R) + mE(R) is _equivalent_, term for
   term, to |∂V| ≥ (Rk - E(R))(n-k) - C(R) — the boundary inequality itself, with
   no slack introduced or removed by the translation.

   Consequence: "maximize X_cross while keeping m small" (the natural next
   adversarial construction to try after points 4-6) is **not a new test**. At
   fixed R, k, m, maximizing X is identically minimizing |∂V| — the original
   problem restated. A construction with large X_cross but also large |∂V|
   proves nothing; only |∂V| ever mattered. This also explains the tautology
   caught earlier in point 3/Strategy 3b's first attempt: any identity-check on
   a Φ-partition is true by construction, because Φ carries no content beyond
   the boundary size.

   Two arenas were considered and rejected for this reason before the identity
   was found: A(5,4) (m=1) has zero symbol freedom — the one remaining symbol
   per coordinate is forced, so no "reuse" experiment is even possible there —
   and A(6,4) at small R is already inside the exhaustive sweep
   (`scripts/sweep_universal_lower_bound.sh`, rows `run 6 4 2`, `run 6 4 3`), so
   a hand-built subset there is guaranteed to satisfy the inequality trivially
   and tests nothing new.

8. **The naive per-split induction is refuted outright (2026-09-13,
   independently verified).** An exhaustive sweep of all coordinate 2-way splits
   of every V' in A(5,3) up to R=4 found 810 splits (out of 4,074,840 checked)
   where the recombination penalty S exceeds the per-split slack bound
   m·ΔE(R)+ΔC(R). The worst case found: V' = {(0,1,2),(0,2,1),(0,3,4),(0,4,3)}
   in A(5,3) (n=5,k=3,m=2,R=4), split into F_a={(0,1,2),(0,2,1)},
   F_b={(0,3,4),(0,4,3)} — two "swap pairs" on disjoint symbol sets, both
   sharing root (0,·,·)-type structure. Independently recomputed with
   `boundary_metrics` from `scripts/check_universal_lower_bound.py`:

   ```text
   |∂F_a| = |∂F_b| = 12, D=X=0 in both fibers (each is internally clean)
   |∂V'| = 16, D(V')=0, X(V')=8
   S = |∂F_a| + |∂F_b| - |∂V'| = 12 + 12 - 16 = 8
   dE = E(4)-E(2)-E(2) = 2, dC = C(4)-C(2)-C(2) = 2
   bound = m·dE + dC = 2·2 + 2 = 6
   ```

   **S=8 > bound=6 — deficit 2.** Confirmed S is _entirely_ cross-target
   collision (X_cross=8, ΔD_cross=0) via direct decomposition: `∂F_a ∩ B = ∅`,
   `∂F_b ∩ A = ∅`, and the 8 shared external targets `I = ∂F_a ∩ ∂F_b` account
   for all of S (0+0+8=8) — the first example in this document where X_cross,
   not ΔD_cross, is the deficit's entire source. The global inequality still
   holds only because each fiber individually has slack: |∂F_a|-rhs(F_a) = 12-9
   = 3 and likewise for F_b, totaling 6, which absorbs the 2-unit per-split
   deficit with 4 to spare (global: |∂V'|=16 ≥ rhs(V')=12, confirmed).

   **This is the direct analogue of Strategy 3's refutation, for the partition
   skeleton rather than the single-vertex-peeling skeleton:** the naive
   induction "sum the two fiber bounds, subtract the recombination penalty,
   compare to the target" does not close step-by-step, exactly as the marginal
   per-vertex bound failed for Strategy 3. The global inequality survives here
   only via banked slack carried in the fibers themselves, not via any per-split
   bound on S. As with Strategy 3, the natural fix (an amortized argument
   tracking banked slack across the recursion) has not been attempted and is not
   obviously easier than the original claim. The full R≤4 sweep has since been
   independently re-run in full (all coordinate 2-way splits of every V' ⊆
   A(5,3), 4,074,840 splits): it reproduces the report exactly — 810 violating
   splits, worst deficit 2, extremal instance the swap-pair V' above at
   partition coordinate p=1 — with no split exceeding the fibers' combined slack
   (every violation is covered by slack banked in the two fibers).

**Status: Strategy 3b's naive per-split induction is refuted**, on the same
footing as Strategy 3 — a concrete, independently-verified counterexample
(point 8) shows the per-split recombination bound can be violated, with the
global inequality surviving only through slack banked in the sub-fibers. Point 7
additionally rules out an entire class of future experiments (anything phrased
as "push X*cross up while keeping m/k favorable") as vacuous, since X and |∂V|
are not independently controllable — so a repair cannot come from a better
X_cross bound alone. The remaining non-vacuous form of the question, in boundary
language rather than through X and D: **how many of F_a's external neighbors get
absorbed (cease to be external) when F_b is unioned in?** That count is a
genuine, non-tautological quantity with the same meaning at every (n,k); whether
an \_amortized* version of the partition induction (banking slack across
recursive levels, as opposed to a per-split bound) can close is the open
question left standing.

### Status of all sketches

**Both induction skeletons tried are refuted in their naive, step-local form.**
Strategy 3 (single-vertex marginal peeling) is refuted by the A(5,3) R:2→3
counterexample (ΔX=2, s=1, LHS=5 > RHS=4). Strategy 3b (coordinate-partition
induction on Φ) is refuted by the A(5,3) R=4 swap-pair counterexample (point 8
above: S=8 > bound=6). In both cases the global inequality survives only because
slack banked elsewhere (an earlier step, or a sibling fiber) absorbs the local
deficit — never because the local bound itself holds. Neither has an attempted
amortized/potential-method repair, and — as noted for Strategy 3 above — any
such repair that explicitly carries banked surplus across steps is provably
equivalent in strength to the original global claim, i.e. telescopes back to
Proposition 5.3 rather than simplifying it. Strategy 4 (dual-compression) has a
tested candidate operator pair, and it is refuted: existence of a non-worsening
move holds 83.5% of the time on a sampled-pair basis (small graphs, R≤9), and
even under the best-choice criterion (every (a,b,op) combination), existence
fails on 193/270 (71.5%) of stress trials at m≥2, R=15-30 — the failure rate
gets _worse_, not better, as R grows relative to the graph. See
`docs/archive/strategy4-dual-compression-averaging-abandoned.md` for the full
diagnosis (two obstructions: injectivity blockage, and an observed 1-for-1
defect/collision exchange that a fixed (m+1):1-weighted potential never
rewards). The additive convex Strategy 2 ansatz is dead: LP infeasibility and a
purely algebraic 4-cycle argument refute it. The root cause is that a sum of
independent fiber potentials is blind to cross-fiber collisions (see full
autopsy above). This does not rule out the distinct recursive coordinate-profile
correction recorded above, which has finite exact certificates but no
closed-form invariant or universal proof. As of this writing, no approach has a
confirmed, closed path to Proposition 5.3; exhaustive computation remains
supporting evidence rather than a proof.

### Entropy/Shearer candidate: tested and killed (2026-09-13)

Before writing any entropy argument by hand, the natural candidate functional
was checked empirically first, per this document's own methodology. Definition
tested: for V' with |V'|=R, per coordinate p let {c_r} be the fiber sizes
(vertices of V' sharing each root at p), and H_p(V') = -Σ_r (c_r/R)log2(c_r/R)
(Shannon entropy of the root-distribution). Candidate conjecture: **the Hamming
Ball minimizes S(V') = Σ_p H_p(V') among all R-subsets** — the natural entropy
analogue of `thm:defect` (which the Hamming Ball maximizes for D).

Exhaustively checked over all R-subsets of A(5,3), R=2,3,4 (1770, 34220, 487635
subsets respectively — the same territory as the existing sweep): **false at
R=3.** The true minimizer is {(0,1,2),(0,1,3),(0,1,4)} (two coordinates fixed,
one varying) with S=3.169925, strictly below the Hamming Ball's S=3.421554.

Worse than merely "wrong minimizer": checking `boundary_metrics` on both shows
the direction is backwards for the actual target. Both configurations achieve
the same maximal defect D=2=E(3) (ties exist at R=3), but:

|                | S (entropy)   | D   | X   | \|∂V'\|                       |
| -------------- | ------------- | --- | --- | ----------------------------- |
| Hamming Ball   | 3.42 (higher) | 2   | 1   | 11 (smaller, correct optimum) |
| entropy-argmin | 3.17 (lower)  | 2   | 0   | 12 (larger)                   |

The configuration with strictly _lower_ total root-distribution entropy has a
strictly _larger_ boundary. So this functional does not merely fail to recover
the exact C(R)+mE(R) staircase (the smooth-vs-discrete risk already flagged) —
it fails to correlate with boundary size in the required direction on the first
non-trivial case tested. This specific candidate is dead; it does not, by
itself, rule out some other entropy-type functional (a different random
variable, a different weighting) — none has been proposed or tested.

### LP-dual candidate (Vector B): tested and killed, more decisively (2026-09-13)

Move 2 was also tried as a fast empirical probe before any hand algebra: solve
the natural fractional LP relaxation of the vertex-boundary problem exactly (via
`scipy.optimize.linprog`, HiGHS), and compare to the true integer-optimal
boundary from exhaustive search. LP: minimize Σ_w y_w subject to y_w ≥ x_v − x_w
for every edge (v,w) of A(n,k), Σ_v x_v = R, 0≤x≤1, y≥0 (the standard
vertex-expansion relaxation: integral solutions reproduce |∂V'| exactly).

Solved exactly for A(4,3) and A(5,3), R=2,3,4, against the true integer minimum
boundary (exhaustive search, same territory as the sweep):

| (n,k,R) | LP relaxation | true integer min |
| ------- | ------------- | ---------------- |
| (4,3,2) | 0             | 4                |
| (4,3,3) | 0             | 5                |
| (4,3,4) | 0             | 6                |
| (5,3,2) | 0             | 9                |
| (5,3,3) | 0             | 11               |
| (5,3,4) | 0             | 12               |

**The LP optimum is exactly 0 in every case, not merely a weaker constant.**
Reason (structural, not numerical): the uniform fractional point x_v = R/N for
every vertex is feasible (sums to R, stays in [0,1]) and makes every edge
difference x_v−x_w=0, so y≡0 satisfies every constraint. This relaxation has no
mechanism forcing spread mass to generate boundary — a known failure mode of
naive vertex-isoperimetric LP relaxations. Consequence: there is no dual
certificate to examine at all; the earlier framing ("banked slack looks like a
dual certificate") had nothing to attach to, since the primal never leaves zero.
This kills the naive LP-relaxation approach more decisively than the entropy
candidate (which was at least directionally informative, if wrong) — it does not
rule out a smarter relaxation (e.g. one with symmetry- breaking or higher-order
constraints), but none has been proposed.

**Status of Vectors A and B after empirical testing:** both proposed "heavy
hammer" replacements for the failed inductions have been tried in their most
natural form and killed outright, in each case faster than an hour of hand
algebra would have taken and for a more decisive reason than the a priori
staircase-vs-smooth risk. Neither result rules out a more sophisticated version
of either tool; none has been proposed.

### The open problem, restated as a capacity/pigeonhole aggregation bound (2026-09-13, corrected)

**Correction to this section, caught immediately after first writing it:** the
original version of this section claimed only distance-2 pairs between F_a and
F_b contribute to shared external neighbors, citing the earlier-proved "ΔX=0 at
adjacency" lemma to dismiss distance-1 pairs. That citation was a category
error: the ΔX=0 lemma is about a different setting (single-vertex marginal
peeling, Strategy 3 — the change in X when adding one vertex to an existing
set), not about two fixed, disjoint fibers sharing external neighbors. Checked
directly and found false: A(6,3), F_a={(0,1,2)}, F_b={(3,1,2)} (a distance-1
pair, sharing a root at p=0) gives eb(F_a)∩eb(F_b) = {(4,1,2),(5,1,2)}, size 2 —
nonzero, and exactly m−1 (m=3 here). This is a real, independent channel this
section's first draft omitted entirely.

**The corrected per-pair picture has two channels, not one:**

1. **Distance-1 (root-sharing) channel — genuinely m-linear.** If u∈F_a, v∈F_b
   share a root at the partition coordinate p (agree everywhere except p), every
   extension (r, γ) with γ not already used by u or v is a shared external
   neighbor: exactly m+1−|{u_p,v_p}| = m−1 of them (generalizing to c_a,c_b>1
   sharing one root: m+1−|A_r∪B_r| where A_r,B_r are the symbols already used at
   p by members of F_a,F_b sharing that root). This is the **only** channel
   whose count is literally linear in m, verified at m=1 (A(4,3)) where it
   correctly vanishes (m−1=0).

2. **Distance-2 (midpoint) channel — gated, but m-free per pair.** For u∈F_a,
   v∈F_b differing at p and exactly one other coordinate q, the candidate
   cross-vertex w = u with position p swapped to β=v_p is a valid, distinct
   boundary point **iff β≠u_q**; symmetrically the other candidate exists iff
   α≠v_q. **These are two independent conditions, not one** — verified directly:
   A(5,3), u=(0,1,3), v=(1,2,3) (distance 2, p=0, q=1) has β=1=u_q (first
   candidate invalid) but α=0≠v_q=2 (second candidate valid), giving eb(u)∩eb(v)
   = {(0,2,3)}, size exactly 1 — confirming the two gates fire independently,
   not together. Each pair contributes 0, 1, or 2, and m does not appear in the
   per-pair condition itself (this part of the original section was correct).

**And the aggregation is not pair-disjoint — the per-pair sum is only an upper
bound**, not an exact count: distinct (u,v) pairs can generate the **same**
target w, exactly the k=3 collapse mechanism already established elsewhere in
this document. So the true shared-boundary count I = eb(F_a)∩eb(F_b) is bounded
above by (channel 1 total) + (channel 2 pair-sum), with equality failing
whenever two pairs collide on a target — which is precisely the phenomenon the
capacity bound needs to control, not a side issue.

**The remaining open problem, precisely, and now correctly scoped:** bound |I| =
|eb(F_a)∩eb(F_b)| from (c_a, c_b, m) alone, where I is the union of **both**
channels above, adjusted for their mutual overcounting. This is strictly harder
than "sum a 0–2 count over distance-2 pairs" — that undercounts (by omitting
channel 1) and overcounts (by ignoring collisions) at the same time. No such
bound has been derived or attempted. m enters through two distinct mechanisms
now identified — root-sharing capacity (channel 1, directly linear) and
cross-pair symbol supply for spreading (channel 2, only via the aggregate
collapse/spread trade-off) — and any correct bound has to account for both plus
their interaction. This is the trailhead for whoever picks this up next; the
first attempt at stating it (this section, initial version) undercounted the
problem's own difficulty, which is itself worth remembering before trusting the
next draft of it either.

The two concrete examples above (the channel-1 `m-1` identity and the channel-2
independent-gating example) are mechanically checked, not just re-verified in
Python twice, in `proofs/Arrangement/CapacityBoundExamples.lean`
(`lake build Arrangement.CapacityBoundExamples`, no `sorry`). That file does not
attempt the open aggregation bound itself — it only pins down, at the kernel
level, the two facts any future attempt at that bound has to remain consistent
with.

#### A candidate (c_a,c_b,m)-bound on |I|, and why it can't be the missing piece (2026-09-13)

**Candidate:** `|I| ≤ c_a·c_b·f(m)`, where `f(m) = max(m-1, 2)` for `m≥2` and
`f(1) = 1`. `f(m)` is exactly `max_I` at `c_a=c_b=1` (single-pair case),
confirmed by full exhaustive search over all pairs for `m = 1..6` (independent
of `k`: A(5,3) and A(6,4) both give `f(2) = 2`) — it is the closed form the two
channels predict: channel 1 (`m-1`, linear) dominates for `m≥3`; channel 2's
fixed 0/1/2 cap dominates at `m=2`; `m=1` is a genuine singularity (too few free
symbols for channel 2's second gate to fire independently).

**Verified as a valid upper bound in every tested cell, tight only in a
small-merge regime, then strictly loose:** exhaustive search over `(c_a,c_b)` up
to `(4,4)` in A(4,3)/A(5,3)/A(6,3)/A(7,3) (two independent runs, matching) gives
exact ties at `(1,1)`, `(1,2)`, `(1,3)`, `(2,2)` for every `m` tested —
including the `(2,2)` swap-pair counterexample from Strategy 3b hitting it
exactly (`|I|=8` at `m=2,3`, `|I|=12` at `m=4`) — then strictly loose once
either side reaches 3 or the sizes are lopsided: `(1,4)`/`(4,1)` cap at
`f(m)·1·3` not `f(m)·1·4` (the single-vertex boundary itself has only `mk` slots
— a separate, trivial cap that binds here); `(3,2)` in A(5,3) gives `9 < 12`;
`(3,3)` in A(4,3) gives `7 < 9`, with the shortfall traced (by hand, for that
exact witness) to individual pairs failing to _simultaneously_ realize their
per-pair maximum, not to the overcounting collision effect flagged above (the
naive pair-sum there equals `|I|` exactly — no collisions occurred, and it still
fell 2 short of the product bound). Zero violations found in 18 tested cells,
across two independent implementations.

**Why this is as far as a `(c_a,c_b,m)`-only bound on `|I|` can usefully go.**
The bound is tight exactly in the small-merge regime an induction step would
actually use — and there, tightness is fatal: the `(2,2)` swap pair achieves
`|I|=8` matching the bound exactly, which is the _same_ configuration where the
naive per-split induction was refuted (`S=8 > 6`, Strategy 3b's original
counterexample). A sharper `(c_a,c_b,m)`-bound on `|I|` cannot rescue the
induction skeleton, because the current bound already reproduces the refuting
witness at full tightness — there is no daylight left to close in that
direction. `|I| ≤ f(c_a,c_b,m)` is therefore _necessary but not sufficient_ for
`S ≤` the recombination budget. The missing margin is not overcounting or a
looser-than-needed `|I|` bound; it is **fiber slack** — the gap between
`rhs(F_a)+rhs(F_b)` and what those two fibers' true combined capacity actually
is when split off from a larger `V'`. That reframes the trailhead: the tractable
open question is no longer "bound `|I|` from `(c_a,c_b,m)`" (answered, to the
extent that answer can help) but "bound the slack lost by splitting a set into
two fibers," i.e. amortizing `S` directly rather than continuing to sharpen the
`|I|` side of the inequality.

### The symmetric/spread counterexample hunt (2026-09-13): vacuity at scale, starvation at the frontier

A proposed adversarial program: beat the universal lower bound with highly
symmetric "spread" sets (orthogonal arrays, finite-geometry designs) deployed at
R values where the Hamming ball is claimed to be least efficient. Closed off on
two independent grounds, both verified computationally:

1. **Vacuity at scale.** With the correct RHS, rhs(n,k,R) = (Rk − E(R))·(n−k) −
   C(R): rhs(7,3,29) = 5 but rhs(7,3,R) ≤ 0 for all R ≥ 30 (−41 at R=42). In
   A(7,3) the bound asserts nothing past R≈29; "beaten at scale" is structurally
   impossible there. The mechanism is generic for fixed k: Rk − E(R) behaves
   like R(k − ½·log₂R), turning negative against the growing correction
   constant, so the bound's content is confined to roughly R ≤ 2^k.

2. **Starvation at the frontier.** Where the bound bites (A(6,3), A(7,3) at R ∈
   {6,7,8}, targets 24/25/24 and 35/37/36), pairwise-distance-2 sets have zero
   internal edges → external boundary ≈ R·k(n−k) minus modest overlap, far above
   tight targets. Greedy spread sets: boundaries 42–77 vs targets 24–37. Random
   search 60k/cell (plus an independent 20k/cell rerun): no set below target.
   The strongest structured candidate — the OA family {(a,b,a+b) mod 7} in
   A(7,3), 30 injective triples, pairwise dist ≥ 2 — exhaustively checked over
   all C(30,9) = 14,307,150 nine-subsets: min |extbd| = 71 > 40 = target at R=9.

Scope honesty: random/structured probes, not exhaustive (the sweep stops at R=5
for k=3); they do not prove the bound at the frontier — they refute the "try
harder designs at scale" program, whose target regime is empty.

Formula-slip note (2026-09-13): an early draft of this probe quoted the wrong
RHS, C(R)+mE(R), printing "target=68" at A(7,3) R=9; the correct RHS is
(Rk−E(R))·m − C(R) and the true target there is 40. Conclusion unaffected (71 >
40 still holds), but the wrong number appeared in the initial discussion and
should not be reused.

### The amortized-slack lemma candidate (2026-09-13, evidence gathered, not a proof)

Reframing from "bound |I| from (c_a,c_b,m)" (answered, and shown incapable of
rescuing Strategy 3b — see above) to the actual remaining lever: whether every
split of a set into two fibers always carries enough banked boundary slack to
cover its own recombination penalty.

**The exact algebraic identity.** For a split `V' = F_a ⊔ F_b` with sizes
`c_a, c_b` `(R = c_a+c_b)`, define the slack of any subset X as
`S(X) = |extbd(X)| − rhs(|X|)`, where `rhs(R) = (Rk−E(R))·(n−k) − C(R)` is the
conjectured true minimum boundary (Proposition 5.3's RHS). Then, **provided
`F_a` and `F_b` share no direct edges to each other**:

`S(V') = S(F_a) + S(F_b) + Delta − I`

where Delta = rhs(c_a) + rhs(c_b) − rhs(R) is the recombination budget and I =
|extbd(F_a) ∩ extbd(F_b)| is the shared-boundary count from the two channels
above. Verified exactly against the known swap-pair witness (A(5,3), R=4):
S(F_a)=S(F_b)=3, Delta=6, I=8, giving 3+3+6−8=4=S(V') — matches the
directly-computed value exactly.

**Caveat found and confirmed by direct construction, not assumed:** this
identity is _not_ unconditional — it breaks by exactly +2 per mutual edge when
F_a and F_b are directly adjacent to each other (checked on the minimal case, an
adjacent pair in A(4,3): the naive identity predicts S(V')=1, the true value is
0). Any general induction built on this identity has to handle direct F_a–F_b
adjacency as a third effect, on top of the two shared-neighbor channels already
catalogued; this has not been done.

**The critical-case test, corrected (2026-09-13, same session, second pass).**
An amortized induction's dangerous case is exactly S(F_a)=S(F_b)=0 (both fibers
individually tight). The first pass at this test (table below, struck through)
checked only I < Delta, using the direct-adjacency caveat above as a footnote
rather than folding it into the quantity being tested. That was an error, not
merely an incompleteness: the exact identity derived above is

`S(V') = S(F_a) + S(F_b) + Delta − (I + B_ba + B_ab)`

where B_ba = |F_b ∩ ∂F_a| and B_ab = |F_a ∩ ∂F_b| are the direct F_a↔F_b
adjacency crossover counts (derived cleanly: ∂F_a = E_a ⊔ B_ba with E_a = ∂F_a \
F_b, so ∂V' = E_a ∪ E_b exactly and E_a ∩ E_b = I — this decomposition was
checked against the algebra independently before trusting it). The critical-case
quantity that actually has to stay `≤ Delta` is **I + B_ba + B_ab**, not I
alone; the first-pass table below undercounted it whenever F_a, F_b shared a
direct edge (which the search already allowed — there was never an adjacency
filter to drop, only a term missing from what was measured).

<details>
<summary>Superseded first-pass table (I only, undercounts — kept for the
record, not to be cited)</summary>

| cell (n,k,c_a,c_b) | Delta | max(I−Delta) found |
| ------------------ | ----- | ------------------ |
| (5,3,1,1)          | 3     | −1                 |
| (5,3,2,2)          | 6     | −3                 |
| (5,3,1,2)          | 4     | −2                 |
| (5,3,1,3)          | 5     | −3                 |
| (6,3,1,1)          | 4     | −2                 |
| (6,3,2,2)          | 8     | −4                 |
| (6,3,1,2)          | 5     | −2                 |
| (5,3,3,3)          | 9     | −5                 |
| (5,3,4,4)          | 12    | −8                 |
| (6,3,3,3)          | 12    | −4                 |
| (6,3,3,4)          | 13    | −6                 |
| (6,3,4,4)          | 16    | −8                 |
| (6,3,4,5)          | 17    | −11                |
| (6,3,5,5)          | 20    | −12                |
| (7,3,4,4)          | 20    | −8                 |

The "margin widens as sizes grow" conclusion drawn from this table does **not**
survive the correction below and should not be reused.

</details>

**Corrected table**, extended to 17 cells (`src/check_amortized_slack.cpp`):

| cell (n,k,c_a,c_b) | Delta | max(I+B_ba+B_ab−Delta) found |
| ------------------ | ----- | ---------------------------- |
| (5,3,1,1)          | 3     | 0                            |
| (5,3,2,2)          | 6     | 0                            |
| (5,3,1,2)          | 4     | 0                            |
| (5,3,1,3)          | 5     | 0                            |
| (6,3,1,1)          | 4     | 0                            |
| (6,3,2,2)          | 8     | 0                            |
| (6,3,1,2)          | 5     | 0                            |
| (5,3,3,3)          | 9     | −2                           |
| (5,3,4,4)          | 12    | −6                           |
| (6,3,3,3)          | 12    | 0                            |
| (6,3,3,4)          | 13    | 0                            |
| (6,3,4,4)          | 16    | 0                            |
| (7,3,4,4)          | 20    | 0                            |
| (6,3,2,3)          | 9     | 0                            |
| (6,3,4,5)          | 17    | −5                           |
| (6,3,5,5)          | 20    | −8                           |
| (7,3,3,3)          | 15    | 0                            |

**This is a materially different picture than the first pass, not just a smaller
margin.** The corrected quantity hits the bound **exactly** (margin 0, i.e.
S(V')=0 exactly at that split) in 13 of 17 cells — the inequality is _tight_,
not comfortably loose. Zero violations (margin > 0) found anywhere. Equality
that consistent is usually explained by a clean bijective/counting argument, not
a slack inequality with room in it — and the 4 negative cells now pin down
exactly when that argument would have to break:

**"Sharp pattern," 17/17 cells — REFUTED at the 18th (2026-09-13, later same
session).** The embeddability rule above (margin = 0 iff
`n−k ≥ ⌈log₂(c_a+c_b)⌉`) was reported as holding with zero exceptions across 17
cells, framed as a candidate exact law. It is false: **A(7,3), c_a=c_b=5** has
`n−k=4`, `R=10`, `⌈log₂10⌉=4`, so `4≥4` — the rule predicts margin=0 — but the
measured value is **margin=−2**. Caught immediately by running one more cell
rather than stopping at 17, and retracted here rather than left standing. Full
data (25 cells; two still running when this was written and not included:
(7,3,6,6) is in progress):

| cell                                                | n−k           | R             | ⌈log₂R⌉              | margin                                       |
| --------------------------------------------------- | ------------- | ------------- | -------------------- | -------------------------------------------- |
| (5,3,1,1)..(5,3,1,3), (6,3,1,1),(6,3,2,2),(6,3,1,2) | 2,2,2,2,3,3,3 | 2,4,3,4,2,4,3 | ≤2,≤2,≤2,≤2,≤2,≤2,≤2 | 0 (7 cells)                                  |
| (5,3,3,3)                                           | 2             | 6             | 3                    | −2                                           |
| (5,3,4,4)                                           | 2             | 8             | 3                    | −6                                           |
| (5,3,2,3)                                           | 2             | 5             | 3                    | −1                                           |
| (5,3,1,4)                                           | 2             | 5             | 3                    | −1                                           |
| (5,3,2,4)                                           | 2             | 6             | 3                    | −2                                           |
| (5,3,3,4)                                           | 2             | 7             | 3                    | −4                                           |
| (6,3,3,3)                                           | 3             | 6             | 3                    | 0                                            |
| (6,3,3,4)                                           | 3             | 7             | 3                    | 0                                            |
| (6,3,4,4)                                           | 3             | 8             | 3                    | 0                                            |
| (6,3,2,3)                                           | 3             | 5             | 3                    | 0                                            |
| (6,3,4,5)                                           | 3             | 9             | 4                    | −5                                           |
| (6,3,5,5)                                           | 3             | 10            | 4                    | −8                                           |
| (6,3,3,6)                                           | 3             | 9             | 4                    | −2                                           |
| (6,3,4,6)                                           | 3             | 10            | 4                    | −4                                           |
| (6,3,6,6)                                           | 3             | 12            | 4                    | −10                                          |
| (7,3,4,4)                                           | 4             | 8             | 3                    | 0                                            |
| (7,3,3,3)                                           | 4             | 6             | 3                    | 0                                            |
| (7,3,5,5)                                           | 4             | 10            | 4                    | **−2** (breaks the rule: `n−k≥⌈log₂R⌉` here) |

The embeddability rule correctly predicts every `margin=0` cell (`n−k ≥ ⌈log₂R⌉`
held in all of them) but is not sufficient: it also holds for (6,3,2,3),
(6,3,3,4), (7,3,3,3), (7,3,4,4) with margin 0 — and now also for (7,3,5,5) where
margin is −2. So `n−k ≥ ⌈log₂R⌉` is necessary-looking but not sufficient for
margin=0; something about the specific sizes (c_a,c_b), not just R and n−k, also
matters, the same way it already mattered for the deficit magnitude in the
non-embeddable regime (A(6,3) R=9: (4,5)→−5 vs (3,6)→−2; R=10: (5,5)→−8 vs
(4,6)→−4 — recorded above, and now shown to extend to whether the deficit is
zero at all, not just how large it is when nonzero). No closed-form invariant is
currently known. Do not cite the embeddability rule as more than "correctly
predicts margin=0 in every case checked so far where it says 0, but is known
incomplete."

The open lemma is therefore back to its unreframed form: **characterize exactly
when tight F_a, F_b give I+B_ba+B_ab = Delta versus a strict deficit, and bound
the deficit when it is nonzero.** 25 data points and a partial, known-incomplete
rule — not a proof, and not as sharp a reframing as previously claimed.

**Correction to the correction (2026-09-13, later same session): the rule above
was itself incomplete, not wrong in kind.** The retraction only tested
`n−k ≥ ⌈log₂R⌉` — half of Theorem `thm:main`(ii)'s own stated achievability
condition (`paper/sections/04_defect_framework.tex:176-177`): **`n−k ≥ ⌈log₂R⌉`
AND `k ≥ ⌈log₂R⌉`.** The (7,3,5,5) "counterexample" has `n−k=4≥⌈log₂10⌉=4`
(passes) but `k=3<4` (fails) — A(7,3) physically has only 3 coordinates, so no
configuration of any alphabet size can embed a `Q_4`, exactly the achievability
theorem's own second condition. This was an omission in this document's
restatement of the rule, not an error in the paper (grep-verified against the
paper text before writing this correction) or in the underlying phenomenon.

Re-checked programmatically against **all 27** recorded cells (the 25 above,
plus (7,3,3,4) and two new sweep points (8,3,3,4)=0, (8,3,4,4)=0, run to test
saturation on a second `(c_a,c_b)` pair): the two-sided rule predicts `margin=0`
exactly where `n−k ≥ ⌈log₂R⌉ AND k ≥ ⌈log₂R⌉` both hold, and a strict deficit
exactly where either fails — **27/27, zero exceptions**, including (7,3,5,5) now
correctly predicted as a deficit case rather than a counterexample. Also matches
the two-pair saturation sweep exactly: `(4,4)` and `(3,4)` both go from deficit
at m=2 to exact equality at every larger m tested, precisely because `R=8` and
`R=7` respectively both satisfy `k=3 ≥ ⌈log₂R⌉=3` already at that k, so only the
`n−k` side was ever the constraint for those two pairs — which is also why the
earlier (unqualified) `n−k`-only rule happened to work for them despite being
incomplete in general.

**Restored, corrected statement:** margin = 0 iff
`n−k ≥ ⌈log₂(c_a+c_b)⌉ AND k ≥ ⌈log₂(c_a+c_b)⌉` — i.e. iff the combined Hamming
ball `HB_R` is actually embeddable in `A(n,k)` at all, matching Theorem
`thm:main`(ii)'s own achievability condition exactly, not a new or separate
rule. The open lemma is reframed exactly as it was before the refutation: (a)
prove `I+B_ba+B_ab = Delta` exactly whenever `HB_R` embeds (a bijective/counting
claim); (b) bound the deficit `Delta − (I+B_ba+B_ab)` when it does not (12 data
points, −1 to −10, still no closed form). The lesson kept from the brief detour:
verify a proposed invariant against a paper's own already-precise statement of
the same condition before restating it informally, since restating it is exactly
where the omission happened.

margin=0 at (1,1) is not a coincidence to explain away: it is exactly the R=2
case (an adjacent pair achieving rhs(2) exactly, S(V')=0 by the Hamming Ball
Evaluation proposition), reappearing correctly once `B_ba`/`B_ab` are counted.
The very first version of this corrected tool flagged margin=0 as
`CRITICAL-CASE VIOLATION` — a real bug (wrong inequality direction: equality is
not a violation of `S(V') ≥ 0`), caught and fixed before any of the numbers
above were trusted.

Also verified as a sanity check (not new information, but confirms no bug in the
identity/search code): the TRUE minimum of S(V') over all splits, exhaustively,
at R=2 and R=3 is exactly 0, matching Proposition 5.3 / the
Hamming-ball-evaluation proposition exactly.

**One genuine edge case surfaced while extending the table, not a bug:** A(5,3)
c_a=4, c_b=5 has zero tight fibers of size 5 at all (`#tight_b=0`) — confirmed
independently in both the Python and C++ implementations. This is the
achievability log-condition (n−k ≥ ⌈log₂R⌉) failing, not a search error: in
A(5,3), n−k=2 < ⌈log₂5⌉=3, so the Hamming ball construction cannot even embed at
R=5 there, and nothing else hits rhs(5) exactly either. A useful independent
confirmation that the achievability caveat already in the paper (Section 1) is
the real boundary, not a formality.

**Ported to C++** (`src/check_amortized_slack.cpp`,
`make check_amortized_slack`) after the Python version stalled at A(6,3)
c_a=c_b=4 (it did eventually finish and cross-validated exactly: max(I−Delta)=−8
both ways). The C++ version is a straight port — bitset boundary computation
instead of Python sets, same combinatorial search, same output format —
cross-validated against every Python result above before being trusted on new
cells; it reaches (6,3,5,5) (checking ~2M disjoint tight-fiber pairs) in under
10 seconds where the Python equivalent would not complete in reasonable time.
Passes `cppcheck` and the repo's `make lint`/`clang-format` checks cleanly.

**Second-pass update to the C++ tool (same session):** extended in place to
compute B_ba and B_ab alongside I and report the combined quantity;
cross-checked against the known swap-pair witness's non-adjacent-fiber case
(where B_ba=B_ab=0 by construction, reducing correctly to the old I=8 value)
before trusting it on adjacent pairs. Re-ran 13 of the original cells (all but
(6,3,4,5) and (6,3,5,5), not yet redone with the corrected quantity); results
are the corrected table above, not the struck-through one.

**What is still missing before this is a lemma, let alone a proof:** (a) a
characterization of exactly when tight F_a, F_b give I + B_ba + B_ab = Delta
exactly versus a strict deficit — the embeddability rule (n−k ≥ ⌈log₂(c_a+c_b)⌉)
predicts every margin=0 case correctly but was refuted as a complete
characterization by A(7,3) c_a=c_b=5 (embeddable by that rule, margin=−2
anyway), so this is back to an open question, not a "why does the known pattern
hold" question; (b) a bound (not yet even conjectured in closed form) on the
deficit Delta − (I+B_ba+B_ab) when it is nonzero — now 12 data points
(−1,−1,−2,−2,−2,−2,−4,−4,−5,−6,−8,−10, plus one more pending) spanning multiple
(n,k), still not enough to guess a formula, and now known to depend on (c_a,c_b)
individually, not just on R and n−k; (c) the direct-adjacency correction term is
now characterized exactly (the identity above) but not yet proven from first
principles independent of the computational check; (d) even granting (a)–(c), an
amortized induction also needs to handle the non-tight case (S(F_a), S(F_b) > 0)
in general, not just confirm one already-known witness has enough margin.
`scripts/check_amortized_slack.py` (I-only, now marked superseded in its own
docstring) and `src/check_amortized_slack.cpp` (the corrected, canonical tool)
are both set up to extend the critical-case table further for whoever picks this
up next.

**28th point, still consistent:** A(7,3) c_a=6, c_b=6 (R=12): checked 24,740,100
disjoint tight-fiber pairs, `max(I+B_ba+B_ab−Delta) = −4`. Two-sided rule:
`n−k=4 ≥ ⌈log₂12⌉=4` passes but `k=3 < 4` fails, so predicted deficit — matches.
28/28, zero exceptions.

### The subcube-intersection attack (started 2026-09-13, same session)

With the two-sided embeddability rule validated to 28/28, the next step is
attempting (a) above analytically rather than by further sweeping: prove
`I+B_ba+B_ab = Delta` exactly whenever `HB_R` embeds, by modeling `F_a`, `F_b`
as (or against) Hamming balls / subcubes and counting the cross-boundary
directly, instead of treating it as an opaque search output.

**Setup.** When `HB_R` embeds, the tight fibers `F_a`, `F_b` are (by the
uniqueness evidence above — tight implies Hamming-ball-consistent defect, 0
counterexamples in 5 exhaustive checks) Hamming balls of radii giving sizes
`c_a`, `c_b`. A Hamming ball of size `c` sits inside a `Q_d` subcube
(`d = ⌈log₂c⌉` coordinates carrying the "used" alphabet symbols beyond the base
point) embedded in the `m = n−k` free alphabet-symbol slots per coordinate. Two
disjoint tight fibers `F_a`, `F_b` correspond to two disjoint Hamming-ball
regions, each living in its own local subcube of the same ambient graph.

**Goal:** express `I+B_ba+B_ab` — the total cross-boundary edges between `F_a`
and `F_b` — as a function of `c_a`, `c_b`, and `m = n−k` alone (when
embeddable), and show it equals `Delta` exactly. Whiteboard starting point
(verified arithmetically against (5,3,4,4) and (7,3,4,4) by the user before this
section was written):

```text
Delta = rhs(c_a) + rhs(c_b) - rhs(c_a+c_b)
      = [E_seq(c_a+c_b) - E_seq(c_a) - E_seq(c_b)] * m
        + [C(c_a+c_b) - C(c_a) - C(c_b)]
```

i.e. Delta splits cleanly into an m-linear term and an m-independent constant
term. If `I+B_ba+B_ab` can be shown to split the same way — a
per-coordinate-position cross-count term scaling with the number of free
alphabet slots `m`, plus a fixed combinatorial constant from the internal defect
structure — the two sides could be matched term by term instead of only
numerically.

**First attempt, and its errors (2026-09-13, same session, caught by advisor
review before being trusted).** A first pass tried to argue that the worst-case
(margin-0) configuration is _forced_ to be a rigid Hamming-ball embedding by
appeal to "the boundary theorem we're proving says S(V') ≥ 0 unconditionally,
and HB achieves 0, so nothing can beat it" — then tried to match `I+B_ba+B_ab`
to `Delta`'s split term by term via: each of the
`ΔE = E_seq(c_a+c_b)-E_seq(c_a)-E_seq(c_b)` cross-edges contributes `1` to
`B_ab` and `1` to `B_ba` (total `2` per edge) plus `m-1` shared external targets
to `I`, so Channel 1 totals `ΔE*(m+1)`, leaving a claimed remainder `ΔC-ΔE`
(`ΔC` defined analogously from `C_constant`) asserted to be exactly the number
of distance-2 cross-pair shared targets. This is **wrong in two ways**, caught
before being written in as fact:

1. **`B_ba`/`B_ab` count touching vertices, not edges** — the "`2` per
   cross-edge" step silently assumes the cross-cut is a perfect matching.
   Counterexample inside the embeddable regime: `A(4,2)`, `c_a=1, c_b=3` (`R=4`,
   `⌈log₂4⌉=2 ≤ k=2` and `≤ m=2`, so the rule predicts margin 0). With base
   `v0=(0,1)`, fresh symbols `{2,3}`, `HB_4 = {(0,1),(2,1),(0,3),(2,3)}`, take
   `F_b = {(0,1),(2,1),(0,3)}` (tight, `|extbd|=5=rhs(4,2,3)`), `F_a = {(2,3)}`
   (tight, `|extbd|=4=rhs(4,2,1)`). `ΔE=2` cross-edges, but the single vertex
   `(2,3)` touches both of `F_b`'s exposed neighbors, giving `B_ab=1, B_ba=2`
   (total `3`, not `2*ΔE=4`). The overall margin is still `0` (`I=2`, total
   `2+1+2=5=Delta=rhs(1)+rhs(3)-rhs(4)`), but via a different split of terms
   than claimed.
2. **`ΔC-ΔE` is not a count in general** — it goes negative for other splits
   (`(c_a,c_b)=(1,3)`: `-1`; `(3,5)`: `-3`; `(1,7)`: `-2`), so it cannot be "the
   number of distance-2 shared targets" as a general claim; the two cases
   checked by hand ((4,4) and (3,4), both giving 0 and matching by coincidence
   of small numbers) do not generalize.

There is also a **circularity** in the framing, independent of the arithmetic:
invoking "`S(V') ≥ 0` unconditionally" — the theorem this whole document is
trying to prove — to conclude the worst case must hit `Delta` exactly assumes
the conclusion at the very size being inducted to. It is legitimate as a
_consistency check_ on data already gathered, not as a step usable inside a
proof. The further claim that the worst-case fibers must be _rigid Hamming-ball
embeddings_ additionally leans on `uniqueness_conjecture`
(`ArrangementExtraconnectivity.lean`), which is still open — 0 counterexamples
across 5 exhaustive sizes (`check_uniqueness.cpp`), not a proof.

**What survives, corrected.** The m-linear term of `Delta` _is_ matched by a
real, checkable mechanism: each of the `ΔE` cross-edges joins some `u ∈ F_a`,
`v ∈ F_b` sharing a root (a common (k-1)-tuple obtained by deleting the one
coordinate where `u,v` differ); their common neighbors are the `m-1` other fresh
symbols available at that coordinate, all strictly external to `F_a ∪ F_b` —
contributing `ΔE*(m-1)` to `I` with exactly the slope `ΔE` that `Delta`'s own
`m`-coefficient has. The corrected form of the constant-term target, replacing
the false `ΔC-ΔE` claim above, is

```text
T + (B_ab + B_ba) = ΔE + ΔC
```

where `T` is the count of strictly-external shared targets generated
specifically by _distance-2_ cross pairs (`u ∈ F_a`, `v ∈ F_b` with
`dist(u,v)=2`), and `B_ab+B_ba` is the touching-_vertex_ count (not `2*ΔE`).
Hand-checked against four splits: `(c_a,c_b)=(4,4)`: `0+8=8=ΔE+ΔC`; `(3,4)`:
`1+6=7`; `(1,3)`: `0+3=3`; `(3,5)`: `0+7=7`. Worked fully at `A(6,3)`,
`c_a=3, c_b=4`, `m=3`: base `v0=(0,1,2)`, fresh `{3,4,5}`, `F_b` = the size-4
face, `F_a` = the size-3 remainder of `HB_7`; both tight (`|extbd|=20,18`);
`I=7 = ΔE*(m-1)+1 = 3*2+1`, the `+1` being the one missing cube corner reached
by a distance-2 path from each fiber; `B_ba=3, B_ab=3`; total
`13 = Delta = 18+20-25`.

**Empirical follow-up (same session): `X=dE` confirmed, disjointness resolved.**
`src/check_amortized_slack.cpp` was extended twice more to test the mechanism
directly rather than only the net identity:

- `compute_X`: counts direct cross-edges `(u,v)`, `u∈F_a`, `v∈F_b`, adjacent —
  via the pair loop (`sum over u∈F_a of |nbrMask[u]∩F_b|`), not derivable from
  `B_ba`/`B_ab` (which count touching _vertices_, not edges). Checked `X == ΔE`
  on every margin=0 pair.
- `D1 = I - T` (the distance-1-attributed share of `I`) checked against
  `ΔE*(m-1)`.

Run across 7 cells spanning `m∈{2,3,4}`, `k∈{2,3}`: `(4,2,1,3)` m=2, `(5,2,1,3)`
m=3, `(6,2,1,3)` m=4, `(6,3,3,4)` m=3, `(6,3,4,4)` m=3, `(6,3,3,5)` m=3,
`(7,3,4,4)` m=4 — **5,124 margin=0 pairs total, `X=ΔE` and `D1=ΔE*(m-1)` hold on
all of them, zero exceptions.** At `(1,3)`, every tight pair found was margin=0
(24/24, 120/120, 360/360), so this isn't a thin slice.

**Scoping this correctly (per advisor review, to avoid over-claiming):** only
`X=ΔE` is an _independent_ count — it's a direct enumeration with nothing
derived from `Delta`. Given margin=0 (`I+B_ba+B_ab=Delta`) and `Delta=ΔE*m+ΔC`
(both already established), `X=ΔE` alone forces `D1=ΔE*(m-1)` and
`T+(B_ab+B_ba)=ΔE+ΔC` algebraically — so those two checks passing are a
consistency check on the instrumentation, not a second independent confirmation.
The real content of this batch is: **the disjointness assumption flagged in the
previous entry (do the `ΔE*(m-1)` cross-edge targets and the `T` distance-2
targets overlap?) is now resolved empirically** — `X=ΔE` pins the cross-edge
count exactly, and combined with margin=0 there is no room left for collision
slack to hide. This does _not_ touch the circularity noted above: `X=ΔE` is a
count over configurations that already are margin=0, and says nothing about
_why_ margin=0 is unbeatable, so it still cannot be a step inside the induction.

**`X=ΔE` derived, conditionally (2026-09-13, same session).** For any disjoint
partition `V'=F_a⊔F_b` (no tightness assumed yet), edge double-counting over the
split gives, unconditionally,

```text
D(V') = D(F_a) + D(F_b) + X
```

where `D(S)=|E_int(S)|` is defect (internal edges). If `F_a` and `F_b` are each
tight (`S(F_a)=S(F_b)=0`), that pins `D(F_a)=E_seq(c_a)`, `D(F_b)=E_seq(c_b)`
(tightness of a set pins its boundary, and the boundary is a function of defect
plus a collision correction — the `rhs(c)` formula's own shape,
`(ck-E_seq(c))m - C_constant(c)`, is built from exactly these two pieces). If,
_additionally_, `V'` itself is tight — which is precisely what margin=0
(`I+B_ba+B_ab=Delta`) gives via the slack identity, since
`S(V')=S(F_a)+S(F_b)+Delta-(margin quantity)=0` when the margin is exactly
`Delta` — then `D(V')` is likewise pinned to `E_seq(c_a+c_b)`. Substituting all
three:

```text
X = E_seq(c_a+c_b) - E_seq(c_a) - E_seq(c_b) = ΔE
```

**Correction (caught immediately after, before further work built on it): the
claim above that "tightness alone pins `D(F_a)=E_seq(c_a)`, no uniqueness
conjecture needed" is wrong.** Tightness pins `external_neighbors(F_a)`, and by
the repo's own decomposition
(`external_neighbors V' + cross_collisions V' + R·k = U(V')·(n−k+1)`, line 39)
that pins a _combination_ of defect and collision count, not defect alone.
Excluding the possibility of a low-defect/high-collision fiber that still hits
the same boundary value — i.e. ruling out exactly what `check_uniqueness.cpp`'s
docstring calls out as open — requires an independent upper bound on collisions,
which is the unformalized Kruskal-Katona path
(`docs/collision-axiom-roadmap.md`, `hypercube_fracture_gap_conjecture` in the
Lean file). So the fiber-side substitutions `D(F_a)=E_seq(c_a)`,
`D(F_b)=E_seq(c_b)` **do** depend on the uniqueness conjecture after all — the
previous paragraph's claim to the contrary is retracted, not just softened.

**What the Lean check below actually buys, correctly stated this time.**
`sum_unique_roots_lower_bound` (line 632) is a _proved_ theorem, no tightness
hypothesis, giving `D(S) ≤ E_seq(|S|)` for every subset `S`. Applied to
`V'=F_a⊔F_b` this gives, unconditionally in `V'`:

```text
X = D(V') - D(F_a) - D(F_b) ≤ E_seq(c_a+c_b) - D(F_a) - D(F_b)
```

so if `F_a`, `F_b` are tight _and_ the uniqueness conjecture holds for them
(`D(F_a)=E_seq(c_a)`, `D(F_b)=E_seq(c_b)`):

```text
X ≤ ΔE
```

**unconditionally in whether `V'` itself is tight** — that dependency (needing
`D(V')=E_seq(c_a+c_b)` exactly) is genuinely removed by this Lean lemma,
replaced by the weaker, already-proved `≤`. The honest summary: `X ≤ ΔE` for any
two tight fibers, conditional only on tight-fiber defect-maximality (the
uniqueness conjecture, still open, 0 counterexamples across 5 exhaustive sizes)
— not on margin=0 of the union. Combined with the 7-cell sweep's `X=ΔE`
observation restricted to margin=0 pairs, the open question sharpens to: does
`X` ever fall _strictly below_ `ΔE` on a non-margin-0 tight pair (which the
bound allows but nothing yet rules out), and is that exactly what produces the
non-embeddable regime's deficit?

**Lean infrastructure check (corrected — the load-bearing lemma is a different
one than first identified):** `sum_unique_roots_lower_bound` (line 632 of
`ArrangementExtraconnectivity.lean`) is a **proved** theorem (strong induction
on `R`, no hypotheses beyond `V'.card = R`) giving exactly `D(V') ≤ E_seq(|V'|)`
for every subset — this is the lemma the `X ≤ ΔE` bound above actually rests on,
not `E_seq_add_bound`/`E_seq_list_sum_le` (lines 299-329), which are a separate,
more specialized superadditivity tool used inside that theorem's own induction,
not something to invoke directly here. Also relevant:
`sandwich_lower_bound_proven` (proved) vs. `sandwich_upper_bound_conjecture` and
`hypercube_fracture_gap_conjecture` (both still open, the latter marked
"TODO(review)" in the file itself) — confirms the "defect ≤ E_seq" direction
used above is on solid ground, while the collision-side bound the
uniqueness-conjecture dependency above needs is exactly the still-open half.

**Explicit coordinate trace, A(6,3) c_a=c_b=4 (2026-09-13, same session).**
Extended `check_amortized_slack.cpp` with a `(T, B_ab+B_ba)` histogram (keyed
per margin=0 pair) plus a witness vertex-tuple pair per distinct bucket, to
check whether the split is constant before trusting a single hand trace. Result:
**all 270 margin=0 pairs land in exactly one bucket, `T=0, B_ab+B_ba=8`** — the
split is constant for this `(c_a,c_b)`, so the witness below is representative,
not cherry-picked.

Witness: `F_a = {(0,1,2),(0,1,3),(0,4,2),(0,4,3)}`,
`F_b = {(5,1,2),(5,1,3),(5,4,2),(5,4,3)}`. This is a `Q_3` subcube cut in half
by position 0 (`{0,5}`), with positions 1 (`{1,4}`) and 2 (`{2,3}`) shared
between the halves. Every cross-edge is `(0,y,z)↔(5,y,z)` for the four
`(y,z)∈{1,4}×{2,3}` pairs — a **perfect matching**, `X=4=ΔE` cross-edges, each
vertex touching exactly one partner on the other side. That perfect-matching
structure is exactly why `B_ab=B_ba=4` (total 8): each of the 4 cross-edges
contributes 1 touching-vertex to each side, with no vertex absorbing more than
one cross-edge (contrast the A(4,2) `(1,3)` counterexample from the first
attempt, where `F_a`'s single vertex _did_ absorb two, because there the fibers
weren't a matched bipartition of a single subcube). And `T=0` because no
external vertex is reachable by one flip from each side independently except the
matched partner itself, already counted at distance 1 — the merge uses up all
three dimensions' alternation exactly, leaving no room for a distance-2
"wraparound" collision.

**Mechanical reading of the m-threshold.** Building this `Q_3` needs one fresh
(unused-at-the-base-tuple) symbol per dimension: position 0's alternate (`5`),
position 1's alternate (`4`), position 2's alternate (`3`) — 3 fresh symbols
total, i.e. `m≥3`, matching `⌈log₂8⌉=3` from the embeddability rule exactly. The
`A(5,3)` comparison run confirms this isn't just a quantitative worsening but a
**qualitative disappearance**: at `m=2`, margin=0 pairs vanish entirely (0/0,
`worst` stuck at `-6` — not merely below 0, the _tight-pair population itself_
never reaches margin=0) for this `(c_a,c_b)`. Mechanically: with only 2 fresh
symbols available, at most 2 of the 3 needed dimension-alternates can be
assigned without reusing a symbol across two positions, which breaks the
disjoint-subcube structure the merge needs — consistent with, though not yet a
full proof of, why `m<⌈log₂R⌉` forces a deficit.

**Scope of this trace — not yet a general argument.** This confirms the
mechanism for one `(c_a,c_b)=(4,4)`, a power-of-2-sized, exactly balanced split
producing a clean `Q_2⊔Q_2→Q_3` structure with a perfect matching. Not yet
checked: whether every margin=0 configuration is a perfect-matching subcube
split (the `(3,4)` split earlier in this section had `T=1`, so _some_ margin=0
configurations are not perfect-matching — that case needs its own trace), and
whether the "3 fresh symbols, one per dimension" counting argument generalizes
correctly to non-power-of-2 `R` and unbalanced `(c_a,c_b)`.

**Still open:** whether/when margin=0 actually occurs in general (the original,
unresolved question — this trace explains the mechanism for one representative
case, not the general occurrence question), the non-perfect-matching case (e.g.
`(3,4)`, `T=1`), and the long-standing deficit bound in the non-embeddable
regime.

### Non-perfect-matching trace, A(6,3) c_a=3, c_b=4

`check_amortized_slack` extended (commit `29815a3`) to histogram
`(T, B_ab+B_ba)` over all margin=0 pairs, not just report the first witness —
confirming a fixed split is genuinely fixed, not an artifact of witness
selection. Run: `./check_amortized_slack 6 3 3 4`. `ΔE=3`, `ΔC=4`, so the
constant-term target is `T+(B_ab+B_ba)=ΔE+ΔC=7`. Result: 2160/2160 margin=0
pairs land in exactly one bucket, `T=1, B_ab+B_ba=6`.

Witness: `F_a={(0,1,2),(0,1,3),(0,4,2)}`,
`F_b={(5,1,2),(5,1,3),(5,4,2), (5,4,3)}`. `F_b` is the full `Q_2`
`{5}×{1,4}×{2,3}`; `F_a` is the same `Q_2` shape at position-0 value `0`,
**minus its fourth corner** `(0,4,3)`. The 3 present vertices of `F_a` still
pair off with 3 of `F_b`'s 4 vertices via position-0 flips — a matching,
`X=ΔE=3`, `B_ab=3` (all of `F_a`), `B_ba=3` (the 3 matched `F_b` vertices),
`B_ab+B_ba=6`.

The missing corner `(0,4,3)` is the `T=1` witness itself: it is adjacent to
`F_b` via `(5,4,3)` (position-0 flip, so `(0,4,3)∈∂F_b`) and _also_ adjacent to
`F_a` via `(0,1,3)` (position-1 flip `1↔4`, so `(0,4,3)∈∂F_a`) — it sits in `I`.
But it is not distance-1-explained: its `F_a`-neighbor `(0,1,3)` and its
`F_b`-neighbor `(5,4,3)` differ in _two_ coordinates (positions 0 and 1), not
one, so no real cross-edge stitches them together. That is exactly the `T` count
firing.

**Mechanical reading: `T` counts the ghosts of missing corners.** Whenever the
smaller fiber is a Hamming ball with one corner absent relative to a full
subcube, that missing corner becomes a _phantom_ shared external target —
reachable at distance 1 from both sides independently, but not stitched by any
actual edge. Both traced cases are consistent with the sharper identity
`T=ΔC-ΔE` (`(4,4)`: `ΔE=4,ΔC=4,T=0`; `(3,4)`: `ΔE=3,ΔC=4,T=1`) — but this
refinement is **conditional on the cross-edges forming a perfect matching**
(`B_ab+B_ba=2X=2ΔE`, observed in both witnesses, not proven in general); it is a
consequence of the already-established `T+(B_ab+B_ba)=ΔE+ΔC` identity plus that
matching property, not an independently-checked fact.

This generalizes the `(4,4)` case cleanly: perfect-matching/`T=0` is the special
case where both fibers are complete subcube corners; whenever one fiber is a
proper Hamming ball short of a full corner (the general case for non-power-of-2
`R`), the shortfall shows up as a `T`-ghost rather than a missing edge. Not yet
checked: whether `T` always equals exactly the count of missing corners for more
than one missing vertex, or for both fibers simultaneously incomplete.

### Non-embeddable regime: three deficit mechanisms

`check_amortized_slack` extended again (commit `b75a2fa`) to track the
`(T, B_ab+B_ba)` histogram over the _dynamic worst-margin_ bucket (the
least-negative margin actually achieved) rather than only `margin=0`, so the
same tool traces the crushed regime where margin=0 pairs don't exist at all. Two
cells isolate the two ways embeddability fails: `(5,3,4,4)` has `k=3≥⌈log₂8⌉=3`
but `m=2<3` ("alphabet starvation"); `(7,3,5,5)` has `m=4≥⌈log₂10⌉=4` but
`k=3<4` ("coordinate starvation").

**m-crush, `A(5,3)` c_a=c_b=4.** `worst=-6` (`Delta=12`, so `combined=6`); all
180 worst-margin pairs land in `T=0, B_ab+B_ba=4`. Witness:
`F_a={(0,1,2),(0,1,3),(0,4,2),(0,4,3)}` = `{0}×{1,4}×{2,3}`,
`F_b={(1,0,2),(1,0,3),(1,4,2),(1,4,3)}` = `{1}×{0,4}×{2,3}` — both full `Q_2`s,
but their position-1 alphabets (`{1,4}` vs `{0,4}`) share only the symbol `4`.
Hand-checking all 16 pairs: only `(0,4,2)-(1,4,2)` and `(0,4,3)-(1,4,3)` are
adjacent (`X=2`, not `ΔE=4`) — the 4 `F_a` vertices with position-1`=1` have
**no partner at all** in `F_b`, since `F_b` never uses symbol `1` there.
Cross-edges aren't merely fewer than expected; half are **structurally
impossible**. `B_ab+B_ba=4` (one touch per side per real edge), and back-solving
`I=combined-(B_ab+B_ba)=2`, fully accounted for by the 2 real edges (`T=0`,
nothing left over). The deficit lands in missing `B`/edges: `m=2` free symbols
force the two fibers to each pick their own position-1 pair, overlapping in only
one place.

**k-crush, `A(7,3)` c_a=c_b=5.** `worst=-2` (`Delta=25`, so `combined=23`);
45,360 worst-margin pairs split into two buckets: `T=0,B_ab+B_ba=10` (15,120
pairs) and `T=1,B_ab+B_ba=9` (30,240 pairs) — both summing to `10=ΔE+ΔC` despite
`margin≠0`, i.e. that sum is not by itself a signal of criticality here.

Witness (bucket 1): `F_a={(0,1,2),(0,1,3),(0,4,2),(0,4,3),(5,1,2)}` (the
familiar `{0}×{1,4}×{2,3}` `Q_2` plus a bridge vertex `(5,1,2)`),
`F_b={(5,1,3),(6,1,2),(6,1,3),(6,4,2),(6,4,3)}` (`{6}×{1,4}×{2,3}` plus bridge
`(5,1,3)`). Hand-checking all 25 pairs finds **7** actual cross-edges (not
`ΔE=5`): `A1-B2, A2-B1, A2-B3, A3-B4, A4-B5, A5-B1, A5-B2`. Because `c_a=c_b=5`
and the two bridge vertices add extra incidences, every vertex on _both_ sides
ends up touched regardless of the edge-multiplicity mismatch — `B_ab=5`,
`B_ba=5` (full saturation on both sides, `B_ab+B_ba=10`). Back-solving:
`I=combined-10=13`, and `T=0` means `D1=I=13`. If this were the critical case,
`D1` should be `ΔE·(m-1)=5·3=15` — instead it is **exactly 2 short**, matching
the margin precisely. **The deficit lands in `D1` itself, not in missing edges
or unsaturated `B`**: both fibers do achieve full mutual saturation (unlike the
m-crush case), but the distance-1 targets those edges generate are not all
distinct — some coincide, because there isn't a 4th free coordinate to keep them
apart. The `ΔE·(m-1)` formula assumes each cross-edge's `(m-1)` fresh-symbol
targets are independent of every other cross-edge's; the k-crush is exactly the
case where that independence assumption breaks.

The second bucket (`T=1, B_ab+B_ba=9`) shows the same total shortfall
re-expressed with one unit moved from `B` into a `T`-type ghost instead — a
further compensation pattern on top of saturation, not traced vertex-by-vertex
here.

**Summary — three distinct deficit mechanisms, not one:**

| regime                        | what breaks                                                                                 | where the deficit lands                                 |
| ----------------------------- | ------------------------------------------------------------------------------------------- | ------------------------------------------------------- |
| m-crush (alphabet starvation) | fibers can't agree on symbols for a shared axis                                             | missing `B`/edges (some become structurally impossible) |
| k-crush, saturated            | fibers saturate each other, but run out of a coordinate to keep distance-1 targets distinct | `D1` falls short of `ΔE·(m-1)`                          |
| k-crush, ghost-compensated    | same shortfall, partially re-expressed                                                      | split between `D1` and `T`                              |

**Scope: this is a mechanistic account of two specific cells, not a general
deficit formula.** It explains _why_ margin goes negative in each observed case
and gives the mechanism a name, but does not yet predict the _magnitude_ of the
deficit as a closed-form function of `(n,k,c_a,c_b)`, nor establish that these
are the only two failure modes for larger/unbalanced splits. The long-standing
non-embeddable deficit-bound problem (12+ data points, `-1` to `-10`, no
formula) is still open; this section narrows _what kind_ of formula to look for
(likely two regime-dependent pieces, one for each crush type, rather than one
uniform expression) but does not supply one.

### Two refuted separability hypotheses (negative results, recorded to

avoid repeating them)

Built `src/sweep_deficit.cpp` (a stripped-down worst-margin-only scan, no
histogram/identity overhead, so many cells can be checked quickly) to test
whether the deficit collapses onto a small number of variables. Two specific
hypotheses were tried and refuted, each by exactly one counterexample
immediately after looking promising on prior data — a pattern worth noting for
its own sake (two points is not evidence of a closed form; the base rate for a
hand-picked hypothesis surviving a third test appears to be low here).

**Hypothesis 1 (refuted): `deficit` is a function of the two starvation gaps
alone,** `dk = ⌈log₂R⌉-k`, `dm = ⌈log₂R⌉-(n-k)`, independent of `n,k,R,c_a,c_b`
individually. Supporting data initially looked clean: `deficit(dk=1, dm=0)=2`
(`A(7,3)` c=5,5), `deficit(dk=0,dm=1)=6` (`A(5,3)` c=4,4),
`deficit(dk=1,dm=1)=8` (`A(4,2)` c=4,4) — and `8=2+6` exactly, suggesting
`deficit=f(dk)+g(dm)`. Refuted by a same-`(dk,dm)=(1,1)` cell at different
`R`/balance: `A(6,3)` c=4,5 (`R=9`, unbalanced) gave `deficit=5`, not `8`.
`(dk,dm)` alone does not determine the deficit.

**Hypothesis 2 (refuted): the underlying arithmetic error that initially seemed
to explain hypothesis 1's exception.** The unbalanced counterexample was first
(wrongly) explained via `ΔE(4,5) < ΔE(4,4)` ("the unbalanced split is promised
less interference, so it can't suffer the same deficit") — but
`ΔE(4,5)=e_seq(9)-e_seq(4)-e_seq(5) =13-4-5=4`, equal to
`ΔE(4,4)=e_seq(8)-e_seq(4)-e_seq(4)=12-4-4=4` (the error: `e_seq(9)=12` was used
instead of the correct `13`, missing `popcount(8)=1`). With `ΔE` actually equal,
the revised hypothesis was that the _achieved_ physical interference
`combined=I+B_ab+B_ba =Delta-deficit` — not `ΔE` — depends only on `(n,k,m)`,
not on the `(c_a,c_b)` split. This looked clean on one pair: `A(6,3)` c=4,5 and
c=5,5 (same `n,k,m=6,3,3`, different split) both gave `combined=12`. Refuted
immediately by a second pair: `A(4,2)` c=4,4 gives `combined=4`, but `A(4,2)`
c=3,4 (same `n,k,m=4,2,2`) gives `combined=5`, not `4`. `combined` also depends
on the split, not just `(n,k,m)`.

**Additional data points (2026-09-13, long-running sweep, recorded for
completeness).** `./sweep_deficit 6 3 5 5   7 3 4 6` (real 102m8.795s — the most
expensive `sweep_deficit` run to date, well past the earlier ones, because
`A(7,3)` has `N=210` vertices and both its tight-fiber enumerations are large):

```text
n   k   ca   cb   R    m     ceilog2 dk   dm   Delta   worst   deficit
6   3   5    5    10   3     4       1    1    20      -8      8
7   3   4    6    10   4     4       1    0    22      -2      2
```

Both values match the _existing_ pattern exactly rather than adding a new
exception: `A(6,3)` c=5,5 lands at `(dk,dm)=(1,1)` with `deficit=8`, the same
value already recorded for `A(4,2)` c=4,4 at that same `(dk,dm)` pair (and _not_
the `deficit=5` of the known `(dk,dm)=(1,1)` counterexample `A(6,3)` c=4,5 — so
`(dk,dm)=(1,1)` now has two witnesses giving 8 and one giving 5, i.e. it is
genuinely multi-valued, not just refuted by a single outlier). `A(7,3)` c=4,6
lands at `(dk,dm)=(1,0)` with `deficit=2`, matching the earlier `A(7,3)` c=5,5
value at that same pair exactly. Net effect: no new refutation, but confirmation
that `(dk,dm)=(1,0)` is stable across at least one split change on the same
`(n,k)`, while `(dk,dm)=(1,1)` is not stable across splits even on different
`(n,k)` — sharpening Hypothesis 1's failure mode (splits matter at some
`(dk,dm)` pairs and apparently not at others) without yet suggesting what
distinguishes them.

**Conclusion of the empirical phase.** Two hand-picked separability guesses,
each briefly consistent with the data on hand, were both refuted by the very
next targeted cell. This is a weak enough hit rate that further hand-picked
hypotheses are not a good use of compute or time; a real closed-form fit (if one
exists) would need a systematic multi-cell regression, not one-off comparisons,
and even then may not exist in a simple form given the deficit is now known to
arise from at least three qualitatively different mechanisms (m-crush, k-crush
saturated, k-crush ghost-compensated) that need not combine additively. This
closes the empirical (exhaustive-enumeration) phase of the subcube-intersection
attack.

**What survives, to build on analytically instead of empirically:**

- The unconditional bound `X ≤ ΔE` (Lean's `sum_unique_roots_lower_bound`,
  proved by strong induction, no tightness hypothesis needed for this direction
  — see "X=ΔE derived, conditionally" above), giving a provable ceiling on
  cross-edges for _any_ disjoint `F_a,F_b`, crushed or not.
- `Delta`'s exact closed form in `(c_a,c_b)` (already known: `Delta=ΔE·m+ΔC`).
- The two-sided embeddability gate (`margin=0` iff `k≥⌈log₂R⌉ AND m≥⌈log₂R⌉`),
  verified 28/28 plus every trace in this document.
- Three concretely identified (not just asserted) physical mechanisms by which
  margin goes negative when that gate fails, each with a hand-verified witness.

**What does not exist yet, and is not expected to come from more sweeps:** a
closed-form deficit magnitude. The path forward is to prove a structural
inequality — the achievable physical interference is _strictly_ bounded below
`Delta` whenever the embeddability gate fails, using the case-split by mechanism
above and the unconditional `X≤ΔE` bound as building blocks — rather than to
keep searching for the exact scalar deficit as a function of `(n,k,c_a,c_b)`.

### The shadow/absorption mapping for `T` (analytic phase, Front 3)

Picking up the "Front 3" plan above (bound `T+B_{ba}+B_{ab}-2X ≤ ΔC-ΔE`, the
ghost-capacity target): four further margin=0 witnesses were hand-traced
(`A(6,3)` `c=3,3` two buckets, `c=3,5`, `c=6,2`) to find the general mechanism
behind `T`. The result generalizes the single-missing-corner picture from the
`(4,4)`/`(3,4)` traces above into three distinct archetypes, defined via a
**bridging-coordinate projection**: fix the coordinate along which `F_a` and
`F_b` sit at different symbols (`F_a` at symbol `x`, `F_b` at symbol `y`, in the
simplest two-slice case), and let `φ_{x→y}` be the map that flips only that
coordinate from `x` to `y`. Then:

- `X = |F_a ∩ φ⁻¹(F_b)|` — the direct matching edges are exactly the vertices of
  `F_a` whose translated image is an actual `F_b` member.
- The **unabsorbed shadow** of `F_a` is `F_a \ φ⁻¹(F_b)`: the vertices whose
  translated image is _not_ an `F_b` member. Each such image lands either on
  empty space or is itself absorbed by other structure; which one determines the
  archetype.

**Archetype 1 — the joint hole (true ghost).** Witnessed directly in `A(6,3)`
`c_a=2,c_b=5`: `F_a∪F_b` forms a complete `Q_3` minus exactly one vertex, and
that missing vertex is the `T=1` ghost, reachable from both fibers via genuinely
different (non-mutually-adjacent) routes — one flip from `F_a`, a _different_
flip from `F_b`. This is the clean case: the shadow lands in empty space with
nothing else going on.

**Archetype 2 — exact complement (perfect absorption, `T=0`).** Witnessed in
`A(6,3)` `c_a=6,c_b=2`: `F_b` is _precisely_ the two corners missing from
`F_a`'s otherwise-complete 6-of-8 `Q_3`. Every unabsorbed-shadow vertex of `F_a`
lands exactly on an `F_b` member — there is no leftover hole anywhere in
`F_a∪F_b`, so `T=0` trivially and the entire budget shows up as extra `B`
(`B_ab=4,B_ba=2`) instead. This is the same family as the `(3,3)` bucket-A trace
(two fibers missing _the same_ corner shape, canceling with `T=0`), just via
full absorption rather than mutual non-overlap.

**Archetype 3 — swapped holes (displaced ghosts).** Witnessed in `A(6,3)`
`c_a=3,c_b=3` bucket B: `F_a`'s natural missing corner is a literal member of
`F_b`, and vice versa — the two fibers have traded holes. Hand-computing the
full boundaries (18+18 vertices) found the actual `T=2` ghosts are **not** at
either fiber's hole; they are at `(5,1,2)` and `(5,4,2)` — the translated images
of two of `F_a`'s own _members_ (not its hole) that happen not to land on `F_b`
members either. So swapping holes doesn't cancel the shadow, it displaces it one
level: the ghosts appear at ordinary members' images rather than at the hole's
image, because the hole's own image is occupied by the other fiber's absorbed
vertex, forcing the accounting to shift to a different pair of images.

**Open: does the mapping handle two simultaneous ghosts on one fiber, and can
they merge onto the same target?** No witness with a genuinely multi-hole,
multi-ghost configuration has been found yet. An attempted probe (`A(6,3)`
`c_a=6,c_b=2`, chosen to have `F_a` missing 2 corners) turned out to be the
degenerate exact-complement case (Archetype 2) rather than a multi-ghost case,
after a very expensive scan (`C(120,6)≈3.65×10⁹`, the largest single tight-fiber
scan this repo has run) — a concrete illustration of the point below. Finding a
genuine two-ghost witness by guessing `(n,k,c_a,c_b)` and hoping the shape
appears is exactly the wrong tool for the question: what's needed is a targeted
existence search (`F_a,F_b` disjoint, both tight, `T≥2`), which is a natural fit
for a CP-SAT/SMT model — not yet written, but sketched in this session's
discussion: boolean membership variables `x_v,y_v` per vertex, cardinality
constraints (`Σx_v=c_a`, `Σy_v=c_b`), disjointness, reified boundary indicators
per vertex to encode the `|∂F_a|=rhs(c_a)` and `|∂F_b|=rhs(c_b)` tightness
constraints, reified shared/absorbed indicators for `I`, `B_ab`, `B_ba`, and
`T≥2` as the solve target (or objective) directly, rather than filtering it out
of a blind enumeration after the fact. This is a natural fit precisely because
the earlier `(6,3,6,2)` miss shows the failure mode of guessing cells: the
tight-fiber fraction at that scale is astronomically thin
(`1080/3.65×10⁹≈3×10⁻⁵`), so a constraint solver's propagation has much more
room to prune than at the smaller cells traced so far. **This is flagged as the
next concrete step, not yet attempted**, and is the natural point at which this
attack should switch tools from exhaustive enumeration to constraint search.

**Scope.** These three archetypes were found by hand-tracing five witnesses
across four cells; they are not claimed to be exhaustive. The general
shadow/projection framing (`X`, unabsorbed shadow, absorption vs. non-absorption
vs. displacement) is a good organizing picture consistent with everything traced
so far, and the target inequality `T+B_{ba}+B_{ab}-2X≤ΔC-ΔE` holds (with
equality at margin=0, by the already-established identity, and strictly in the
crushed-regime witnesses checked earlier in this document) on every witness —
but no proof that these three archetypes are the only ones, nor a formal
argument bounding `T` in general from the shadow structure, has been attempted
yet.

### Pure-fiber interface calculation: the `A(6,3)`, `6+2` equality witness (2026-09-14)

The CP-SAT `amortized` search found the following **FEASIBLE equality witness**
(not an optimality certificate):

```text
F_a = {012, 053, 412, 413, 452, 453}
F_b = {013, 052}
```

It has `L = I + B_ba + B_ab = 18`, recombination budget `Delta(6,2) = 14`, and
fiber slacks `S(F_a)=S(F_b)=2`. Thus it realizes the equality `18 = 14 + 2 + 2`.
Its six direct cross-edges explain all 12 shared external targets (`T=0`); this
is a sharp example, but not a proof that 18 is the cell maximum.

The useful local subblocks are

```text
N_0 = {012, 053},       N_4 = {412, 413, 452, 453},
F_b = {013, 052}.
```

`N_0` and `F_b` are the diagonal pairs of the alternating four-cycle
`{012,013,053,052}`. Their interaction has `L(N_0,F_b)=12` against
`Delta(2,2)=8`, while `S(N_0)=S(F_b)=2`; hence this is a local equality block.
It is coordinate-tangled: its four cycle edges alternate between coordinates 1
and 2, so it cannot be accounted for in a single root clique.

The apparently separate tight four-cycle `N_4` has an isolated interaction with
`F_b` satisfying

```text
|boundary(N_4)| = 20,  |boundary(F_b)| = 16,
|boundary(N_4 union F_b)| = 26,
L(N_4,F_b) = 10 = Delta(4,2).
```

This does **not** make the two interactions additive. Define, for disjoint sets,
the exact boundary-loss functional

```text
L(A,C) = |boundary(A)| + |boundary(C)| - |boundary(A union C)|.
```

It agrees with `I+B_ba+B_ab`. Therefore the exact interface correction is the
definition

```text
J(A,B;C) = L(A,C) + L(B,C) - L(A union B,C),
```

or equivalently

```text
L(A union B,C) = L(A,C) + L(B,C) - J(A,B;C).
```

For this witness,

```text
L(N_0,F_b) + L(N_4,F_b) - L(F_a,F_b) = 12 + 10 - 18 = 4.
```

Two visible contributors are `012` and `053`: they are shared external targets
for the isolated `N_4`--`F_b` calculation, but become members of `F_a` and hence
direct-absorption targets in the full calculation. The number 4 is nevertheless
an aggregate inclusion--exclusion correction, not a proved bijection with four
particular vertices. A sign theorem for `J` is proved in the subsequent
tripartite calibration; no useful packing or upper-bound theorem for `J` is
known. Consequently a proposed pure-fiber-tree proof needs an interface-aware
coupled potential; raw local losses cannot be summed.

The separate defect/collision ledger is also signed. Here `m+1=4`,
`D(F_a)=6<E(6)=7`, and `D(F_b)=0<E(2)=1`. With the boundary identity, the two
slacks satisfy

```text
S(F_a) = 4*(7-6) - X(F_a) = 2,
S(F_b) = 4*(1-0) - X(F_b) = 2,
```

so `X(F_a)=X(F_b)=2`. Collision mass offsets defect-derived slack in this
example; it is not an independently nonnegative resource to which cross-fiber
loss can be charged.

### Tripartite interface calibration (2026-09-14)

`src/search_triples.cpp` is a CP-SAT optimizer for the symmetric third-order
boundary interface of three pairwise-disjoint sets:

```text
J(A,B;C) = d(A)+d(B)+d(C)-d(AuB)-d(AuC)-d(BuC)+d(AuBuC),
```

where `d(S)=|boundary(S)|`. Equivalently, `J(A,B;C)=L(A,C)+L(B,C)-L(AuB,C)`. The
following runs were made in `A(5,3)`; `OPTIMAL` is a finite-cell certificate and
`FEASIBLE` is only an incumbent at the stated time limit.

| sizes | constraints                  | objective | status          | value |
| ----- | ---------------------------- | --------- | --------------- | ----- |
| 1+1+1 | none                         | min `J`   | OPTIMAL         | 0     |
| 1+1+1 | none                         | max `J`   | OPTIMAL         | 3     |
| 2+2+2 | none                         | min `J`   | FEASIBLE, 120 s | 0     |
| 2+2+2 | none                         | max `J`   | FEASIBLE, 120 s | 6     |
| 2+2+2 | individually tight           | min `J`   | OPTIMAL         | 0     |
| 2+2+2 | individually tight           | max `J`   | OPTIMAL         | 3     |
| 2+2+3 | individually tight, `A(5,3)` | max `J`   | OPTIMAL         | 4     |
| 2+2+2 | individually tight, `A(6,3)` | max `J`   | OPTIMAL         | 8     |

The nonnegativity question is actually settled directly, without a solver. For
each vertex `w`, let `a,b,c` say whether `w` has a neighbor in `A,B,C`,
respectively. The per-vertex contribution to `J` is:

- `a*b*c` when `w` lies outside all three sets;
- `b*c` when `w` lies in `A`;
- `a*c` when `w` lies in `B`;
- `a*b` when `w` lies in `C`.

Thus every contribution is 0 or 1, and `J(A,B;C) >= 0` for arbitrary
pairwise-disjoint sets in any loopless graph. It counts exactly the three-way
external targets and the category-flipper vertices. The remaining substantive
computational question is how large `J` can be, and whether a useful upper bound
follows from coupled pure-fiber slack and root-incidence data.

The `A(6,3)` maximum is particularly diagnostic:

```text
A = {012,013},  B = {512,513},  C = {412,413}.
```

It decomposes into the two coordinate-0 root cliques with roots `12` and `13`.
In each root clique, one member from each color contributes three
category-flipper units, and the fourth available symbol gives one three-way
external-target unit, for four units per root and `J=8` in total. Thus `J` is
nonnegative but is not bounded by a small cardinality-only constant; a viable
interface bound must retain ambient root-fiber capacity (hence dependence on
`m=n-k`) or equivalent incidence information.

Root cliques nevertheless do **not** give a decomposition of `J`. The certified
tight `A(5,3)`, `2+2+3` maximizer is

```text
A = {012,013},  B = {042,043},  C = {021,023,041},  J=4.
```

The contributors `013`, `023`, and `043` form one three-colour root clique
(delete coordinate 1, root `03`) and account for three units. But `042` is a
fourth, coordinate-tangled category flipper: it is adjacent to `012` in `A` by
changing coordinate 1 and to `041` in `C` by changing coordinate 2. It belongs
to no single three-colour root clique. Hence a root-local upper bound would need
a canonical ownership rule plus explicitly bounded multi-root corner gadgets;
summing only root-clique contributions would miss valid interface mass.

### Single-fiber collision--defect adversary (2026-09-14)

`src/search_single.cpp` is a CP-SAT model for one subset `F subseteq V(A(n,k))`.
It reifies membership, external boundary, and occupancy of every coordinate
root. For `R=|F|`, `m=n-k`, and `O(F)` the number of occupied coordinate roots,
it computes exactly

```text
D(F) = R*k - O(F),
X(F) = (m+1)*O(F) - R*k - |boundary(F)|.
```

Thus `X` is the collision term in the boundary identity, not simply a count of
distance-two pairs. Put `deltaD=E(R)-D(F)`. The `collision-excess` objective is
`X-(m+1)*deltaD`. It probes the stronger experimental refinement

> **Zero-base refinement (candidate; unproved).** `X(F) <= (m+1)*(E(|F|)-D(F))`
> for every `F subseteq V(A(n,k))`.

The exact slack identity is

```text
S(F) = [C(R)-E(R)] + (m+1)*(E(R)-D(F)) - X(F).
```

Therefore the proposition-equivalent collision statement is the weaker and
correct target

> **Deficit-compensated collision inequality (candidate; unproved).**
> `X(F) <= [C(R)-E(R)] + (m+1)*(E(R)-D(F))`.

The binary arithmetic term `C(R)-E(R)` is zero at powers of two and is generally
positive between powers of two; it is the collision allowance already used by an
ideal partial Hamming ball. A positive `collision-excess` refutes only the
zero-base refinement. It refutes the proposition-equivalent inequality only when
it exceeds `C(R)-E(R)`. The certified finite results below are evidence, not a
general proof or a justified pure-tree telescoping argument.

All reported runs use an exact `--defect-deficit=q` constraint and return
`OPTIMAL`. The `A(6,3)` rows at deficits 2 and 3 initially reached the old
60-second limit, then were rerun with no limit and certified.

#### `A(6,3)`, `R=6`, `m=3`

Here `E(6)=7`, `C(6)-E(6)=2`, and the proposition-equivalent bound is
`X <= 2+4*deltaD`. The displayed residual uses the stronger zero-base refinement
`X <= 4*deltaD`.

| `deltaD` | `D(F)` | maximum `X(F)` | `X-4*deltaD` | `S(F)` |
| -------: | -----: | -------------: | -----------: | -----: |
|        1 |      6 |              4 |            0 |      2 |
|        2 |      5 |              6 |           -2 |      4 |
|        3 |      4 |              8 |           -4 |      6 |
|        4 |      3 |             11 |           -5 |      7 |

The deficit-one maximizer is `{012,053,412,413,452,453}`. It has 12 occupied
roots, boundary 26, and `X=4*12-18-26=4`. This is the same six-vertex fiber in
the `6+2` interface equality witness. It refutes the stronger empirical guess
`X <= 2*deltaD`; the `deltaD=4` result `X=11` also refutes an exact `X=2*deltaD`
pattern.

#### `A(5,3)`, `R=6`, `m=2`

Here `C(6)-E(6)=2`; the proposition-equivalent bound is `X <= 2+3*deltaD`. The
displayed residual uses the stronger zero-base refinement `X <= 3*deltaD`.

| `deltaD` | `D(F)` | maximum `X(F)` | `X-3*deltaD` | `S(F)` |
| -------: | -----: | -------------: | -----------: | -----: |
|        1 |      6 |              2 |           -1 |      3 |
|        2 |      5 |              4 |           -2 |      4 |
|        3 |      4 |              6 |           -3 |      5 |

#### Restrictive `m=1` tests

For `m=1`, the zero-base refinement is `X <= 2*deltaD`; the exact target also
includes the size-dependent allowance `C(R)-E(R)`. Complete feasible
defect-deficit frontiers were certified in `A(4,3)` for the selected sizes:

| `R` | `deltaD` values in order | corresponding maximum `X` values | maximum `X-2*deltaD` |
| --: | ------------------------ | -------------------------------- | -------------------: |
|   4 | 1, 2, 3, 4               | 0, 2, 3, 4                       |                   -2 |
|   5 | 1, 2, 3, 4, 5            | 1, 2, 3, 4, 6                    |                   -1 |
|   6 | 1, 2, 3, 4, 5, 6, 7      | 0, 1, 2, 4, 5, 6, 9              |                   -2 |

As a zero-deficit calibration, `A(4,3)`, `R=3`, `deltaD=0` was also optimized
and returned `X=0` (with `S(F)=1`) at `{012,013,023}`. Thus a positive
arithmetic term `C(R)-E(R)` need not appear as collision mass; it may remain as
genuine boundary slack.

The first higher-dimensional `m=1` test also certified `A(5,4)`, `R=4`,
`deltaD=1`: `D=3`, `X=0`, `X-2*deltaD=-2`, and `S(F)=2`, attained by
`{0123,0124,3104,3124}`. These finite frontiers contain no candidate violation
of either the stronger refinement or the exact target, but are not an exhaustive
verification of all sizes or a proof of either claim.

#### Coordinate-local empty-slice audit

For a split at coordinate `c`, identify `S_i` with its projection `Q_i` in the
local arrangement graph. Let `P_i` be the occupied transversal projections
compatible with symbol `i`; compatibility is essential:
`P_i = {x in pi_c(S) : i notin x}`. If `L_c` counts the transversal empty slots
which also lie in a local boundary, the exact incidence identity is

```text
X(S) - sum_i X(S_i) = L_c.
```

The immune transversal slots are

```text
sum_i |P_i \ (Q_i union boundary_local(Q_i))|
 = N_transversal - R - L_c.
```

Empty slices contribute their whole compatible `P_i` to this quantity.
`search_single --objective=local-residual` fixes coordinate 0 and maximizes the
unresolved amount

```text
m*R - dC_0 - m*dE_0 - V_0,
```

where `V_0` is the exact contribution from empty compatible slices. Positive
output is a finite witness that void slots alone do not close the coordinate
step. Nonpositive output is only a finite-cell result; it is not a proof of a
universal void-cover inequality.

For `A(4,3)`, `R=6`, the unrestricted local-residual optimizer returned
`OPTIMAL` value 0. Its witness concentrated all six vertices in one coordinate-0
slice, with `|pi|=6`, `V_0=6`, and required gap 6, so the empty slices cover the
coordinate step sharply. In contrast, the previous high-deficit `deltaD=7`
collision optimizer had zero void slots in every coordinate, but its required
gaps were negative; those `void=vacuous` reports provide no geometric void-cover
evidence.

## Empirical Closure: The Three-Stool Trap Architecture (2026-09-14)

Three independent computational campaigns — exhaustive corpus, Swiss-cheese
hole-punching, and core-plus-crumbs injection — totalling 3,796 adversarial sets
across A(4,3), A(5,3), and A(6,3), found zero counterexamples to the
coordinate-local inequality and revealed a rigid trap architecture.

### The Pool Envelope

For every slice $i$ at every coordinate, the local collision count satisfies

```text
L_{c,i} <= |P_i| - R_i
```

where $P_i$ is the compatible projection pool and $R_i$ the slice size. This is
tight: a slice can swallow at most its un-occupied compatible projections. The
envelope is saturated when $L_{c,i} = |P_i| - R_i$, which occurs precisely when
every compatible projection not already in the slice is eaten by a local
collision — i.e., the slice is maximally swollen. Across 4,806 slices with
$L_{c,i} > 0$ up to $R = 19$, this bound is never violated and is tight at every
$R_i$ value from 1 to 19.

### The Failure of Edge-Gradient Charging

A natural approach: charge each local collision to its projection edges via a
union bound. The result is catastrophic. The ratio of total gradient
($\sum_i |S_x \triangle S_y|$ over all edges) to local collision count $L_c$
reaches 64x on the corpus, with 404 sufficiency failures (witnesses where the
gradient exceeds $L_c$). The loosest cases all occur at $R = 3$ in $A(4,3)$,
where single-slice sets have $R_i = 3$ and $L_{c,i} = 1$, but the union-bound
overcount yields gradients up to 64. The core obstruction: local graph geometry
(edge counts) scales faster than global subadditivity ($\Delta C + m \Delta E$).
Any successful charging argument must operate globally per symbol, not locally
per edge.

### The Strictness Conjecture ($L_c > 0 \implies S > 0$)

The most striking empirical discovery. Define the colliding slack as

```text
S = dC + m*dE - (m+1)*conc - L_c
```

where $dC, dE$ are the subadditivity penalties from the slice partition,
$\mathrm{conc} = \sum_i \binom{R_i}{2}$ counts concollision pairs, and $L_c$ is
the total local collision count. The 159 zero-slack sets are \emph{all}
collision-free: $L_c = 0$, consisting of pure single-slice sets ($R = 2$, one
nonempty slice) and full transversals ($R = 2$, two unit slices with $L_c = 0$).
The moment $L_c > 0$, the slack is at least 1. This held across every
configuration three distinct adversarial archetypes could devise: random
sampling (3,449 sets), Swiss-cheese hole-punching with sizes 16/24/32 (197
sets), and core-plus-crumbs injection at A(5,3) and A(6,3) (150 sets). Minimum
colliding slack = 1 in all three campaigns.

The interpretation: the adversary is mathematically trapped. To starve the
subadditivity penalty ($\Delta C, \Delta E$), they must concentrate vertices in
a saturated core, but a saturated core leaves no void space to swallow slots.
The moment they fracture the set to create collision mass ($L_c > 0$), the
hypercube's arithmetic overpays for the theft by at least one full unit. Zero
slack is exclusively reserved for collision-free configurations where the
arithmetic is trivially exact.

### Summary Table

| Probe                                       | Sets  | Fails | Min colliding slack |
| ------------------------------------------- | ----- | ----- | ------------------- |
| Exhaustive A(4,3) R≤3 + random + structured | 3,449 | 0     | 1                   |
| + Swiss cheese (sizes 16/24/32)             | 3,646 | 0     | 1                   |
| + Core+crumbs (A(5,3), A(6,3))              | 3,796 | 0     | 1                   |

### What Remains Open

The strictness conjecture ($L_c > 0 \implies S > 0$) is empirically decisive but
unproved. A proof would require showing that any set with $L_c > 0$ necessarily
has $\Delta C + m \Delta E > (m+1) \cdot \mathrm{conc} + L_c$, i.e., the
subadditivity penalties strictly dominate the collision mass. The pool envelope
($L_{c,i} \le |P_i| - R_i$) is a necessary ingredient: it bounds how much
collision mass the adversary can generate per slice, but closing the gap
requires a global argument that the overpayment is at least 1 when any slice has
$L_{c,i} > 0$.

### Single-Vertex Hole-Filling Induction: Falsified

A third induction was proposed for the strictness conjecture: walk from `S` up
to a saturated, collision-free state one vertex at a time by filling swallowed
slots, and show slack is non-increasing along the walk (the only polarity that
would actually bound `Slack(S)` below). Tested computationally against the full
3,796-set corpus (`scripts/hole_filling_check.py`, 7,836 individual hole-fill
steps):

- Universal ("every fill is safe") fails on 22% of steps, worst single step
  `ΔSlack = +6`.
- The assumed per-hole geometric refund `α ≥ 1` is false — filling one hole can
  _create_ new swallowed slots elsewhere (`α` as low as `-2`).
- Existential ("some fill is safe") also fails: **212 of 2,318** sets with
  `L_c > 0` have _no_ safe hole at all. Minimal witness: `A4r-R4-2` (`R=4` in
  `A(4,3)`), one hole, one possible move, and that move strictly increases slack
  — a binary-digit phase misalignment between the global `R` and the local `R_i`
  crossing power-of-two boundaries at different times.

Full autopsy in `docs/archive/bridge1-hole-filling-induction-abandoned.md`. This
single-vertex hole-fill candidate is closed; a different nonlocal move or
potential is not ruled out.

### Dual Root-Compression Candidate: Falsified

The specific cardinality-preserving Pinto-style pair described in Strategy 4 was
tested rather than left as a metaphor. For its guarded replacement operator `C`
and guard-repair operator `D`, the literal averaging condition already fails in
380/1,052 non-degenerate small-cell trials. The weaker condition actually needed
for an induction -- that _some_ admissible choice of ordered symbol pair and of
`C` or `D` does not decrease `Phi = X + (m+1)D` -- also fails.

The small-cell check has two different denominators that must not be confused. A
randomly selected symbol pair admits a non-decreasing branch in 878/1,052 trials
(83.5%). Among the 174 trials whose originally selected pair failed, an
exhaustive retry over all admissible symbol pairs rescues 171, leaving three
all-pair witnesses in $A(4,3)$ at $R=7,8$. Their best outcome lowers `Phi` by at
least one. They exhibit both failure mechanisms:

1. **Injectivity blockage.** Most symbol pairs produce duplicate image vertices
   and are not admissible moves.
2. **Defect--collision exchange.** A valid move can trade one unit of defect for
   one collision. Since defect has coefficient $m+1$ in `Phi`, that exchange
   lowers `Phi` by at least one.

The apparent small-cell concentration at $m=1$ is not a valid base-case
carve-out. A larger finite stress test over random and Swiss-cheese sets in
$A(5,3)$ and $A(6,3)$, for $R\in\{15,20,30\}$, finds 193 failures among 270
tested sets when every admissible `(a,b,C/D)` choice is considered. In some
cases the candidate's non-degeneracy and validity filters leave no admissible
move. This is finite diagnostic evidence, not a theorem about all operator
pairs, but it refutes this one guarded pair as a universal compression
mechanism. Full data, witnesses, and the precise scope of the filters are
recorded in `docs/archive/strategy4-dual-compression-averaging-abandoned.md`.

### Full-Star Failure Landscape and the D-Invariance Finding

Following the full-Star counterexample in $A(10,8)$ (Proposition~5.3 in the
paper), `scripts/sweep_boundary.py`, `scripts/partial_star_sweep.py`, and
`scripts/occupancy_sweep.py` map where the failure occurs. Key findings:

- The full-Star cumulative-slack failure point $j_{\text{fail}}$ is not a fixed
  constant in $m$: it drifts from $j\approx6$ at $m=3$ up to $j\approx13$ at
  $m=1000$, so no bound of the form $R\le 1+Cm$ for fixed $C$ holds universally.
- For any fixed total leaf count $R-1$ distributed across branches (cap $m$ per
  branch) in a Star-shaped configuration, $D$ depends only on the total leaf
  count, not on how it is distributed across branches -- it is shape-invariant
  per $R$. Consequently cumulative slack is driven entirely by $X$, and
  concentrating leaves into fewer/higher-occupancy branches strictly increases
  $X$ (worsens slack) while spreading them thinner strictly helps.
- The full Star is **not** the exact worst-case (maximum-$X$) shape among
  same-total-leaf-count partitions: at $m=3$, $R=19$, the uneven partition
  $(3,3,3,3,3,2,1)$ (six full branches' worth of leaves spread across seven
  branches) has $X=91$ (cum. slack $-7$), strictly worse than the symmetric full
  Star $(3,3,3,3,3,3)$ at $X=90$ (cum. slack $-6$). The full Star is
  near-worst-case, not extremal; the exact extremal partition is uncharacterized
  and is an open combinatorial question, not resolved here.

### Safe Parameter Regime ($m \le 4$)

The unrestricted universal lower bound fails at $R_{\text{fail}} = 1 + j_{\text{fail}} \cdot m$, where
$j_{\text{fail}}$ drifts as $O(\log m)$. However, the hypercube embedding condition requires
$R \le 2^m$. For $m \le 4$, we have $2^m < R_{\text{fail}}$:

| $m$ | $2^m$ (embed max) | $R_{\text{fail}}$ | Safe margin | Status |
|---|---|---|---|---|
| 2 | 4 | 17 | +13 | Unconditionally Safe ($A(10,8)$ has $m=2$) |
| 3 | 8 | 19 | +11 | Unconditionally Safe ($A(6,3), A(7,4), A(9,6)$ have $m=3$) |
| 4 | 16 | 25 | +9 | Unconditionally Safe ($A(8,4)$ has $m=4$) |
| 5 | 32 | 26 | -6 | Crossover / Open frontier ($R \le 25$ safe) |

Thus for $m \le 4$, ALL embeddable $R$ have the Hamming Ball as the unambiguous
minimizer. The paper's canonical examples ($A(6,3)$, $A(7,4)$, $A(8,4)$, $A(9,6)$,
$A(10,8)$) all have $m \le 4$, so the main theorem is unconditionally useful for
every example in the paper. The general case ($m \ge 5$) remains open.

## Status

This is a research sketch, not a proof. Root compression (Step 4/5) is stalled
on the coordinate-tangling obstacle. Both induction-based replacements attempted
since (marginal peeling and coordinate-partition induction) are refuted in their
naive form — see "Status of all sketches" above for the final state of the
board. The single-vertex hole-filling induction attempted after that is also
refuted (see above). The tested dual-compression candidate (Strategy 4) is
refuted for its one guarded operator pair -- see
`docs/archive/strategy4-dual-compression-averaging-abandoned.md`. The Lyapunov
function over fiber-size multisets (Strategy 2) is dead: the additive convex
ansatz is refuted by both LP infeasibility and a closed-form 4-cycle argument
showing the locked potential is structurally blind to cross-fiber collisions
(see full autopsy above). A fundamentally different compression operator that
does not merely alter the selection rule for this guarded pair also remains
open.

**Update (post full-Star counterexample):** the unrestricted inequality X(V') +
(m+1)D(V') ≤ C(R) + m·E(R) is false in general -- it fails for the full Star in
$A(10,8)$ at $R=17$ (Proposition 5.3 in the paper) and, more broadly, for full
Stars at $j\ge j_{\text{fail}}(m)$ branches for every $m\ge2$ (see "Full-Star
Failure Landscape" above). The exhaustive computational evidence below predates
that counterexample and was bounded by $R\le10$ (connected) or $R\le6$ (all
subsets) in its parameter cells; it is not in tension with the refutation, which
occurs at larger $R$ relative to $m$, but it no longer supports an unrestricted
claim. The open proof target is now the restricted inequality, for whatever
restricted regime (in $R$, $m$, or set shape) survives the failure landscape
mapped above -- that regime is not yet characterized.

The exhaustive computational evidence (3,796 sets as of 2026-09-14, across
exhaustive, random, structured, Swiss, and crumbs corpora) found no
counterexample within its bounded search; the full-Star witness above shows this
reflects the bound on $R$, not the truth of the unrestricted claim. The existing
Cheng et al. computational results \cite{cheng2022extraconnectivity} do not
supply this — they are limited to small g and do not address the all-subsets
vertex-isoperimetric question.
