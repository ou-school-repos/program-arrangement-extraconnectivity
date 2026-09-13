# Proof Sketch: Weighted Fiber-Overlap Inequality

## Target

For all V' ⊆ A(n,k) with |V'| = R and k ≤ n:

\[
X(V') + (m+1) D(V') \le C(R) + m \cdot E(R)
\]

where m = n−k, X = cross-collisions, D = defect, C = C_constant, E = E_seq.

Equivalently (via the fiber identity |∂V| = Rkm − X − (m+1)D):

\[
|\partial V'| \ge (Rk − E(R)) \cdot m − C(R)
\]

The Hamming ball achieves equality in both forms.

## Why Standard Compression Fails

The guarded symbol compression `compressSet V' a b` shifts symbol b → a
across all vertices, guarding against collisions. In A(4,2), shifting
3 → 1 in {[4,3], [1,3]} produces {[4,1], [1,3]} and raises the boundary
from 5 to 7. The weighted potential X + (m+1)D falls from 3 to 1.

The failure is structural: in A(n,k), the injectivity constraint means
moving a vertex from one fiber to another can destroy an existing fiber
member's uniqueness, creating new collisions at a different coordinate.
The boundary increase is a global consequence of a local move that
ignores the multi-coordinate fiber structure.

This counterexample is verified and documented in
`IsoperimetricPartialPermutation.lean` and `universal-lower-bound-work.md`.

## Proposed Approach: Coordinate-Root Compression via H(k,n)

### Step 1: Lift to the Hamming Graph

A(n,k) embeds naturally in the Hamming graph H(k,n) = K_n □ ... □ K_n
(k copies). Vertices of H(k,n) are all k-tuples over {0,...,n−1}
(no injectivity constraint). The arrangement graph is the induced
subgraph on injective tuples.

The fiber structure extends: at coordinate p, the root is the (k−1)-tuple
obtained by deleting position p. Two vertices share a root iff they differ
at exactly position p.

### Step 2: Known Vertex-Isoperimetric Result for H(k,n)

**For the binary case (n=2):** Harper's Edge Isoperimetric Theorem
\cite{harper1966optimal} states that among all R-element subsets of the
d-dimensional hypercube Q_d, the initial segment of colex (binary
lexicographic) order minimizes the edge boundary. This is proven in Lean
as `harpers_edge_isoperimetry` in `ArrangementGraphUtils.lean`. By the
handshake lemma, minimizing the edge boundary is equivalent to maximizing
internal edges, which is equivalent to maximizing E_seq(R).

**For the n-ary case (n>2):** The correct reference is Bollobás and
Leader \cite{bollobas1991vertex}, who proved that for the n-ary Hamming
graph H(k,n), the initial segment of colex order minimizes the vertex
boundary |∂S| = |N(S) \ S|. This is a genuinely harder result than
Harper's theorem — it requires a compression argument specific to
n-ary alphabets, not a one-line corollary of Kruskal-Katona. The
Kruskal-Katona theorem itself applies to uniform set systems (binary
layers of the hypercube), not to n-ary Hamming graphs directly.

**The vertex-isoperimetric profile of H(k,n)** is known: for each R,
the minimum vertex boundary is achieved by a colex-initial segment, and
the minimum value is determined by the "profile function" of H(k,n).
This is the result we would need to transfer to A(n,k).

### Step 3: The Transfer Problem (This Is the Hard Part)

The natural approach is to show that the vertex-isoperimetric inequality
for H(k,n) implies the weighted inequality for A(n,k). However, this
transfer is not straightforward — it is the core open problem, not a
minor technical step.

The difficulty: for V' ⊆ A(n,k), the fiber structure in A(n,k) is a
**restriction** of the fiber structure in H(k,n), but the restriction is
not benign. In H(k,n), each root at position p has fiber size
(n−1)!/(n−k+1)!. In A(n,k), each root has the same full fiber size
(since the root is a (k−1)-tuple of distinct symbols, extending it with
a fresh symbol at position p gives n−k+1 choices). So the fiber sizes
are actually identical — the difference is in which vertices of those
fibers are **present in V'**.

The injectivity constraint creates a coupling between coordinates that
does not exist in H(k,n): moving a vertex at position p changes its
root at every other position q ≠ p. This "coordinate tangling" is the
source of the X/D interaction that the weighted potential must resolve.
A clean transfer inequality would need to quantify how much the
injectivity constraint can degrade the boundary below the H(k,n)
optimum, and show that this degradation is exactly compensated by the
(m+1)D term. No such inequality is currently known.

**What the Cheng et al. literature provides:** Cheng, Lipták, and Tian
\cite{cheng2022extraconnectivity} computed small-g extraconnectivity
cases (g ≤ 6) by exhaustive computer search. Their ad-hoc topological
constructions turn out to be Hamming balls. They do NOT prove a general
vertex-isoperimetric theorem for A(n,k) — their contribution is
computational, not structural. The general inequality we need is not in
their paper.

### Step 4: The Weighted Potential and Root Compression

Since vertex compression in A(n,k) fails (Step 1), and transfer from
H(k,n) is open (Step 3), a direct approach on the root structure of
A(n,k) may be more tractable.

Define a **root compression** at position p: given two roots
r₁ <\_colex r₂, if the r₂-fiber contains more V'-members than the
r₁-fiber, move one vertex from the r₂-fiber to the r₁-fiber by
changing its symbol at position p.

**Properties of root compression:**

1. **Preserves |V'|**: we move a vertex, not copy it.

2. **Preserves injectivity**: the moved vertex v has symbol b at position
   p. We change it to symbol a, where a is determined by the target root
   r₁. Since r₁ is a (k−1)-tuple of distinct symbols, the extension
   symbol a (the unique value at position p in any vertex with root r₁)
   is one of the n−k+1 symbols not appearing in r₁. Since v currently
   uses k distinct symbols and r₁ uses k−1 distinct symbols, and
   n − k + 1 ≥ 1, the symbol a is available and distinct from all
   symbols in r₁. The resulting vertex has k distinct symbols, so
   injectivity is preserved.

3. **Does not decrease X** (needs proof): moving a vertex into a fiber
   that already has more members should not reduce the total collision
   count. This is plausible because the moved vertex joins a larger
   fiber, creating at least as many new collisions as it destroys in
   the old fiber. However, the moved vertex's roots at other positions
   q ≠ p also change, which could affect X at those positions. The
   net effect on X is not obvious and requires analysis.

4. **Does not decrease D** (needs proof): D = Rk − |unique roots|. The
   moved vertex may create a new shared root at position p (increasing
   D) but may also eliminate a shared root at position q ≠ p (decreasing
   D). The net effect on D is not obvious.

The claim — **not yet proved** — is that the net effect on X + (m+1)D
is non-negative. If true, iterating root compressions at every coordinate
converges to a configuration where roots are in colex order with maximal
fiber occupancy, which should be the Hamming ball.

### Step 5: Why This Is Hard (Multi-Coordinate Interaction)

The root compression concentrates vertices in fewer fibers at position p,
which locally increases X and D at that coordinate. But the moved
vertex changes its symbol at position p, which changes its root at every
other position q ≠ p. This can:

- Create new shared roots at some positions q (increasing X and D there)
- Destroy shared roots at other positions q (decreasing X and D there)

The Hamming ball avoids this problem because it is "aligned" — all
vertices share the same first d coordinates, so the fiber structure is
nested across positions. A general configuration is not aligned, and
root compression at one position can disrupt the structure at others.

The key obstacle is that X and D are **global** quantities (summed over
all coordinates), but root compression is a **local** operation (at one
coordinate). The multi-coordinate interaction means the local benefit
at position p can be offset by damage at other positions.

### Partial-Layer Case

When R does not exactly fill a colex-initial segment of a "layer"
(vertices with the same number of 1-bits in the binary case, or the
same weight class in the n-ary case), the Hamming ball for size R is a
partial layer: it fills complete layers below and takes an initial
segment of the next layer. Standard isoperimetric proofs (Harper,
Bollobás–Leader) handle this via a tie-breaking argument: within each
layer, colex order is used, and the partial layer is an initial segment
of colex within its layer.

For the weighted inequality, the partial-layer case matters because the
Hamming ball's fiber structure changes qualitatively at layer boundaries.
At R = 2^d (a full hypercube), the Hamming ball has X = 0 and D = E(R)
maximal. At R = 2^d + 1 (one vertex into the next layer), X jumps to
a positive value and D drops. The weighted potential X + (m+1)D must
track these discontinuities.

The exhaustive evidence suggests the inequality remains tight at layer
boundaries (e.g., A(7,2) R=4 = 2^2 is tight, A(7,2) R=5 is slack 1).
A proof would need to handle the partial-layer transition explicitly,
showing that the weighted potential does not drop below the bound during
the transition.

### Possible Resolution Strategies

1. **Coordinate-by-coordinate with repair**: compress at position p,
   then repair the damage at positions q ≠ p by further compressions.
   Show that the total effect across all coordinates is non-negative
   for the weighted potential.

2. **Global Lyapunov function**: define a potential that is monotone
   under root compression at every coordinate simultaneously (e.g.,
   lexicographic order on the multiset of fiber sizes across all
   coordinates). Show that the Hamming ball is the unique maximum.
   The challenge is finding a Lyapunov function that actually works —
   the natural candidates (fiber-size multisets) may not be monotone
   due to the multi-coordinate interaction.

3. **Induction on R with partial-layer tracking**: for R = 1, trivial.
   For R > 1, remove a vertex and use the induction hypothesis. Show
   that adding the vertex back preserves the bound, tracking the
   partial-layer transition explicitly.

4. **Direct counting**: decompose X(V') + (m+1)D(V') into a sum over
   coordinates and roots, and bound each term. This avoids compression
   entirely but requires a new combinatorial identity.

5. **Contrapositive approach**: instead of showing the Hamming ball
   maximizes X + (m+1)D, show that any configuration with
   X + (m+1)D > C(R) + m·E(R) would violate some known constraint
   (e.g., the defect bound D ≤ E(R), or the fiber identity itself).

## Whiteboard Sketches for Strategies 2 and 3 (2026-09-12)

Root compression (Step 4) has stalled on the coordinate-tangling obstacle
(Step 5): a local move at position p unavoidably scrambles roots at every
q ≠ p, and no argument has bounded that scatter damage tightly enough to
recover monotonicity. Rather than keep pushing on that specific operator,
here are informal starting sketches for Strategies 2 and 3 from the list
above. **Neither is worked out to the point of being checkable, let alone
a proof — both are starting points for further whiteboard work, recorded
here so the next pass doesn't restart from nothing.**

### Strategy 3 sketch: Marginal induction with partial-layer tracking

This reuses the exact skeleton already used to prove `Theorem~\ref{thm:defect}`
(`D(V') ≤ E_seq(R)`, proven by strong induction on R via `sum_unique_roots_lower_bound`
and the subadditivity lemma `E_add_min_le`), extended to carry the joint
quantity Φ = X + (m+1)D instead of D alone.

Proceed by strong induction on R. Strip a vertex v from V', apply the
inductive hypothesis to V' \ {v} (size R−1), then bound the marginal
change Δ Φ = ΔX + (m+1)ΔD when v is added back.

1. **Marginal defect.** v brings k roots. Let s = the number of these
   roots already populated by V' \ {v} (shared roots), so k−s are fresh.
   By definition, ΔD = s.

2. **Marginal collisions.** v contributes at most (k−s)(n−k) new external
   edges from its fresh roots (0 from shared roots, since those don't
   expose new boundary). Some of these edges may land on vertices already
   in the external boundary of V' \ {v}; each such overlap adds 1 to ΔX
   instead of exposing a genuinely new boundary vertex.

3. **Target bound for the inductive step.** To close the induction, we'd
   need

   ```
   ΔX + (m+1)s ≤ ΔC(R) + m·Δ E_seq(R)
   ```

   where Δ E_seq(R) = popcount(R−1) (from the defining recurrence of
   E_seq) and ΔC(R) is the corresponding marginal change in C_constant.

4. **Partial result on ΔX (2026-09-12, derived and hand-checked, not yet
   Lean-formalized).** Write `total_coord_edges` for the quantity
   `Σ_w bd_mult V' w` over external w. Since each of the U used roots at
   a coordinate contributes (m+1) minus its own occupancy, and occupancies
   sum to Rk, `total_coord_edges = U(m+1) − Rk`; combined with the proved
   fiber identity `|∂V'| = Um − D − X` (using U = Rk − D), this gives
   `total_coord_edges = |∂V'| + X`, i.e. **X = total_coord_edges − |∂V'|**.

   Adding v with s shared roots: ΔU = k−s, ΔR = 1, so
   `Δ(total_coord_edges) = (k−s)(m+1) − k = (k−s)m − s`. Separately,
   Δ|∂V'| ≥ −[s>0] (the only way the boundary can shrink is v itself
   dropping out of it, which requires s>0, and by at most 1 — every other
   effect of adding v can only add new external vertices). Substituting:

   ```
   ΔX = Δ(total_coord_edges) − Δ|∂V'| ≤ (k−s)m         (all s, since the
                                                          s=0 and s>0 cases
                                                          both reduce to this)
   ```

   This matches the maximal ΔX = km observed in the s=0 double-clique
   stress test in A(6,2) (R: 8→9, k=2, m=4, ΔX=8=km exactly), so the bound
   is tight, not just an upper estimate.

   A second, dimension-independent cap: only vertices of V'\{v} at Hamming
   distance exactly 2 from v can border one of v's fresh-root fibers, and
   each such vertex differs from v in exactly 2 coordinates, so it can
   border at most 2 fresh fibers. With N₂ = #{u ∈ V'\{v} : d(u,v)=2} ≤ R−1,
   this gives ΔX ≤ 2(R−1) independent of k and m. **The "exactly 2"
   claim above needs a more careful check** — a distance-2 vertex's second
   differing coordinate could in principle coincide with a _shared_ root's
   coordinate rather than a fresh one, which would need ruling out before
   this half is treated as proved.

   **What remains.** The bound ΔX ≤ (k−s)m only closes the induction if
   `(k−s)m ≤ Δ C(R) + m·Δ E_seq(R) − (m+1)s` for all valid s, i.e. if
   `(k−s)m + (m+1)s ≤ ΔC(R) + m·popcount(R−1)`, i.e.
   `km + s ≤ ΔC(R) + m·popcount(R−1)`. This must hold uniformly in s up to
   k, which is the actual remaining obstacle — the ΔX bound above is now
   solid, but plugging it into the target inequality has not yet been
   checked against ΔC(R)'s actual formula.

   **The (k−s)m bound is checked and fails as a standalone closer.**
   Plugging R=1→2 into A(10,5) (k=5, m=5, s=k−1=4 since the two connected
   vertices share k−1 roots): LHS = km+s = 29, RHS = ΔC(2)+m·popcount(1)
   = 1+5 = 6. 29 ≤ 6 is false, so `(k−s)m` alone cannot close the
   induction at small R with large k, m.

   **Proven exact fact for this case: ΔX = 0 for any two adjacent
   vertices, in general (not just this example).** If external w borders
   v via coordinate p and borders u via a different coordinate q, then w
   agrees with u everywhere except q, forcing w_p = u_p; but w disagrees
   with v at p, forcing u_p ≠ v_p. Symmetrically, w agrees with v
   everywhere except p, forcing w_q = v_q, and w disagrees with u at q,
   forcing v_q ≠ u_q. So u and v differ at both p and q — contradicting
   that adjacent vertices differ at exactly one coordinate. Hence p = q,
   i.e. any two adjacent vertices contribute to X only through a shared
   coordinate, never a cross-coordinate collision: **ΔX = 0 whenever v is
   adjacent to the vertex it's added next to**, independent of n, k, R.
   This is a genuine, reusable lemma (worth Lean-formalizing on its own),
   not just an artifact of the R=2 example.

   **Conjectured second cap (not yet proved as a general trade-off).**
   Separately, ΔX ≤ 2·N₂ where N₂ = #{u ∈ V'\{v} : d(u,v)=2} ≤ R−1 (see
   above); this "Source Cap" is small when V'\{v} is sparse near v. The
   working conjecture is that ΔX is always bounded by
   `min((k−s)m, 2N₂)`, and that whichever cap is small enough to be the
   binding one always keeps `ΔX + (m+1)s ≤ ΔC(R) + m·popcount(R−1)`
   satisfied — i.e., that a configuration cannot simultaneously have
   large (k−s)m (small s, so cheap ΔD) _and_ large N₂ (many distance-2
   neighbors near v, which should itself require existing structure that
   already costs D_old/X_old budget). **This trade-off has not been
   proved.** Only two data points support it so far — the sparse R=2 case
   (Source Cap ≈ 0 saves it) and the dense R=9 clique-trap (Volume Cap is
   tight but RHS has banked enough slack from D_old to absorb it). No
   mid-range example (moderate R, k, m, with neither cap trivially small)
   has been checked; that is the natural next stress test before treating
   the crossover as anything more than a naming convention for two
   observed cases.

This approach's main appeal is that steps 1–2 reuse verified machinery
directly; the ΔX bound in step 4 is now a checked derivation with one
fully proved special case (ΔX=0 at adjacency) and one still-conjectural
general trade-off, but closing the induction for all R, s still requires
either proving that trade-off or finding a counterexample to it.

### Strategy 2 sketch: Global Lyapunov function on fiber multisets

1. **State space.** Let 𝓕(V') be the multiset of nonzero fiber sizes
   across all k coordinates: 𝓕(V') = ⋃ₚ {|F\_{p,r}| : r ∈ U_p(V')}.

2. **Candidate potential.** Since D(V') = Rk − |𝓕(V')|, a natural
   candidate is Φ(V') = Σₚ Σᵣ f(|F\_{p,r}|) for a convex f (e.g.
   f = E_seq or f(c) = C(c,2)). Convexity would make concentrating
   vertices into fewer, larger fibers — which the Hamming ball
   maximizes — strictly increase Φ.

3. **Single-operation accounting.** For one root-compression move
   (vertex v shifted from a smaller to a larger fiber at coordinate p):
   the _local_ gain at p is positive by convexity of f. The _collateral_
   effect at every other coordinate q ≠ p, where v's root also changes,
   could go either way; worst case v lands in k−1 brand-new
   singleton fibers, each contributing f(1) at those coordinates.

4. **What would need to be shown.** (a) Φ actually lower-bounds
   X(V') + (m+1)D(V') in a form matching the target inequality — this
   link has not been established, only motivated informally via D;
   (b) the local gain at p strictly dominates the worst-case collateral
   loss summed over q ≠ p, for every possible compression move. Neither
   (a) nor (b) has been attempted formally.

This approach's main appeal is a single global argument with no per-layer
casework; its main open question is whether such an f and such a
domination bound actually exist — nothing here rules out that they don't.

### Strategy 4 sketch: Dual root-compression via averaging (2026-09-12, untried)

Inspired by Pinto's proof of the Bollobás-Leader directed-path conjectures
(arXiv:1504.07079), which resolves an analogous single-operator-fails
obstacle on the hypercube Q_n. Pinto defines two compression operators
C_i (push down: `C_i(S) = {x ∈ S : x∖{i} ∈ S}`) and D_i (push up:
`D_i(S) = S ∪ {x : x∪{i} ∈ S}`) on subsets of Q_n. Neither is
individually monotone for the directed edge/vertex boundary, but he
proves `|∂→(S)| ≥ ½(|∂→(C_i(S))| + |∂→(D_i(S))|)`, which forces at least
one of the two to be no worse than S even though neither is
unconditionally so. Iterating over i = 1..n converges to a down-set with
boundary no larger than the original.

This is structurally the same shape of obstacle as A(n,k)'s root
compression: the single guarded-symbol compression already tried and
refuted (`compressSet`, A(4,2) counterexample raising boundary 5→7) is
one operator that individually fails, exactly as Pinto's C_i and D_i
individually fail on some sets in Q_n. **Untried question:** does a pair
of dual root-compression operators exist on A(n,k) — analogous to C_i/D_i
but respecting the injectivity constraint — such that neither is
monotone for X + (m+1)D alone, but an averaging inequality like Pinto's
forces at least one to be non-worsening? This has not been attempted;
even finding the right candidate pair of operators (one "concentrate
toward smaller root" and one "concentrate toward larger root," suitably
guarded for injectivity) is open, let alone proving an averaging bound
for them.

This approach's appeal is that it directly targets the coordinate-
tangling obstacle (Step 5) that stalled the original single-operator
compression, using a technique proven to work around the analogous
single-operator failure on Q_n; its risk is that the injectivity
constraint may block the averaging identity's proof in a way that has no
counterpart in Q_n (Pinto's proof leans on set-complement symmetry
between C_i and D_i that may not survive the "no repeated symbol"
restriction).

### Status of all sketches

None of the four are results. Strategy 3 is judged the most advanced in
practice: its steps 1–2 reuse already-verified Lean machinery
(`sum_unique_roots_lower_bound`, `E_add_min_le`), and its open step (a
bound on ΔX) now has a proved special case (ΔX=0 at adjacency) and one
still-conjectural general trade-off, both recorded above. Strategy 4 is
the newest and completely unexplored beyond the analogy; it is flagged
because the averaging technique is a good structural match for the
coordinate-tangling failure mode, not because any part of it has been
attempted on A(n,k). Strategy 2 remains a from-scratch invariant search
with no candidate potential yet shown to work.

## Status

This is a research sketch, not a proof. Root compression (Step 4/5) is
stalled on the coordinate-tangling obstacle. The two strategies sketched
above (marginal induction and the global Lyapunov function) are candidate
replacements, recorded at the whiteboard-sketch stage only; neither has
been carried far enough to check, let alone prove.

The exhaustive computational evidence (30 rows as of 2026-09-12, up to
3.65 billion subsets in a single row) supports the inequality with no
counterexample found. The proof target is precise: show
X(V') + (m+1)D(V') ≤ C(R) + m·E(R) for all V', by any of the strategies
above or another route. The existing Cheng et al. computational results
\cite{cheng2022extraconnectivity} do not supply this — they are limited to
small g and do not address the all-subsets vertex-isoperimetric question.
