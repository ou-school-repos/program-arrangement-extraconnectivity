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

**First crucible test (2026-09-12): a candidate pair fails the averaging
inequality, and the test itself was degenerate.** Since |V'|=R is fixed,
Pinto's C_i/D_i (which change |S|) cannot be ported directly; any dual
pair must be cardinality-preserving swaps. Candidate pair tried, keyed to
a symbol pair (a,b) exactly as in `compressSet`:

- C(V') = the original guarded compressSet: for each v containing b,
  replace b→a; skip (leave v unchanged) if v already contains a.
- D(V') = push-up repair: for each v containing b, if v also contains a
  (the guard-blocked case for C), apply the full transposition (a b) to
  the whole vertex instead of skipping; otherwise same as C.

Tested on the exact refuted-compressSet example, A(4,2),
V'={[4,3],[1,3]}, (a,b)=(1,3): C(V')={[4,1],[1,3]} (the known result,
Φ: 3→1, boundary 5→7). D(V')={[4,1],[3,1]}, computed by hand: boundary=5,
Φ=3 — unchanged from the original. Checking the averaging inequality
Φ(V') ≤ ½(Φ(C(V'))+Φ(D(V'))): 3 ≤ ½(1+3) = 2 is **false**. Pinto's
mechanism does not transfer as stated on this example.

**The D(V')=3 result is not evidence D works — it's a degenerate
coincidence.** Both vertices in this V' contain b=3, so D applied the
transposition (1 3) to _every_ vertex, which is exactly the global graph
automorphism π=(1 3) applied to all of V'. Automorphisms trivially
preserve Φ and boundary for any set, so Φ(D(V'))=Φ(V') here proves
nothing about whether D does useful compressive work — it only shows D
degenerated into a no-op symmetry because R=2 and both vertices shared
the swapped symbol. **This candidate pair has not been meaningfully
tested.** The next test needs R≥3 with at least one vertex not containing
b, so D acts as a genuine partial (non-global) relabeling rather than a
whole-set automorphism, before either accepting or discarding this
operator pair.

### Strategy 3's per-step marginal induction is refuted (2026-09-13)

The marginal induction as originally framed — prove a purely local
per-step inequality `ΔX + (m+1)s ≤ ΔC(R) + m·popcount(R-1)` and let
strong induction on R do the rest — is **false in general**, confirmed by
an explicit, fully hand-verified counterexample, not merely a case where
the crude ΔX bound was too generous.

A(5,3) (k=3, m=2), R: 2→3. v=[1,2,3]. V*old={u1,u2} with u1=[1,2,4]
(sharing root (1,2) at p=3 with v, so s=1) and u2=[4,5,3] (sharing no
root with v). Verified directly via per-coordinate `coord_boundary`
computation (not naive neighbor-list overlap, which over-counts: two
V-members sharing a root both border the same external point through
the \_same* coordinate, contributing bd_mult=1, not 2 — this tripped up
the first pass of verification and was caught and corrected before
accepting the result):

- D_old=0, X_old=0 (u1, u2 share no roots and no external neighbors).
- D(V')=1, X(V')=2 (external vertices [4,2,3] and [1,5,3] each have
  bd_mult=2, hit via p=1 from v and p=2 from u2, and vice versa —
  genuine cross-coordinate collisions). Boundary=13, cross-checked
  against Rkm−Φ = 18−5 = 13.
- Marginal step: ΔX=2, s=1, m=2. LHS = 2+3(1) = 5.
  RHS = ΔC(3)+m·popcount(2) = 2+2(1) = 4. **5 ≤ 4 is false.**
- Global check still holds: Φ(V')=5 ≤ C(3)+m·E(3)=7, with slack banked
  entirely from the R=1→2 step (u1,u2 placed at distance 3, costing 0
  defect/collision there, banking the full ΔC(2)+mΔE(2)=3 available at
  that step).

**Implication.** This is not a case the crude `(k-s)m` bound merely
overestimates — the actual, correctly-computed ΔX genuinely violates the
per-step target. No tightening of the ΔX bound alone can fix this: the
per-step inequality is false as stated, for a configuration with
verified D*old=0. Any repair must abandon step-locality — e.g. an
amortized/potential-method argument (bank surplus from early steps,
spend it on later deficits) rather than requiring every step to
individually satisfy the marginal bound. Note that proving the amortized
(cumulative) version directly is a rephrasing of the \_original* global
claim (the per-step terms telescope back into X(V')+(m+1)D(V') ≤
C(R)+mE(R)), so this refutation removes the main advantage marginal
induction offered — reducing an R-vertex claim to a 1-vertex check — not
just one candidate bound within it.

### Strategy 3b: coordinate-partition induction on Φ (2026-09-13, unresolved)

Distinct from the refuted single-vertex marginal peeling: `thm:defect`'s
actual proof partitions all of V' by symbol at one coordinate p into
fibers {F_α}, recurses on each (smaller) fiber, and bounds the
recombination via subadditivity — no vertex is ever peeled off in
isolation. Whether this same skeleton closes for Φ=X+(m+1)D (not just D)
was tested and is presently unresolved, with one confirmed dead end and
one open, concretely-scoped question:

1. **A first "verification" was caught as tautological.** Computing the
   recombination penalty (X*cross, ΔD_cross) *by subtracting the known
   fiber totals from the already-computed Φ(V')* makes the identity
   Φ(V')=ΣΦ(F*α)+penalty hold by construction for any partition of any
   set — checking it against the target is circular, since it just
   restates Φ(V')≤target using an answer already in hand. A real test
   requires bounding the penalty from fiber sizes alone, _before_ knowing
   Φ(V').

2. **A geometric a priori bound was derived and is basically sound.**
   For 2-fiber partition F_a, F_b (sizes c_a, c_b) at coordinate p, a
   cross-collision requires u∈F_a, v∈F_b at Hamming distance exactly 2
   (differing at p and one other coordinate q), which forces
   X_cross ≤ c_a·c_b (each pair contributes ≤1) and, from a per-vertex
   degree argument (each v has at most k−1 candidate partners, one per
   q≠p), also X_cross ≤ (k−1)·min(c_a,c_b). The combined bound is
   min(c_a c_b, (k−1)·min(c_a,c_b)). An initial claim that this bound
   grows unboundedly with k (making the approach "doomed") was itself an
   error — it came from summing the two constraints instead of taking
   their minimum; properly combined, the bound is k-independent for
   fixed, small fiber sizes.

3. **The real obstacle is an asymptotic scale mismatch, not
   k-dependence.** ΔC(R)+mΔE(R) (the marginal slack for a single 2-way
   split) is O(log R) — it depends only on popcount(R−1) and
   bitlength(R−1). But c_a·c_b for a roughly balanced split is O(R²), and
   even the refined (k−1)·min(c_a,c_b) bound is O(R) for fixed k. Checked
   numerically at R=20, balanced (10,10) split: slack = 3+3m (=6 at
   m=1), while c_a c_b=100 and (k−1)min bound=40 at k=5 — both far
   exceed the slack. The two small checks that appeared to work (R=3
   split (2,1), R=4 split (2,2)) do not reveal this, since both are too
   small for the O(R²) vs O(log R) gap to show up.

   **This does not prove Strategy 3b is dead** — it only shows the
   generic combinatorial upper bounds on X*cross (pairwise count, degree
   count) are too loose to confirm the inequality at moderate-to-large R.
   Whether the \_true, tightly-argued* worst-case X_cross for a genuine
   adversarial fiber pair stays down near O(log R) — the way earlier
   "crude bound achievable but the trade-off saves it" patterns played
   out elsewhere in this document — has not been checked. No explicit
   adversarial configuration at R≈10–20 has been hand-constructed and
   verified the way every other claim in this document has been.

4. **A specific R=20 adversarial construction (k=3) was checked and
   collapses.** F*a = {(1,y,3): y∈Y}, F_b = {(2,z,3): z∈Z}, |Y|=|Z|=10,
   Y∩Z=∅. All 100 pairs are genuinely distance-2, but every pair sharing
   the same z (or same y) generates the \_same* collision target — bd_mult
   counts distinct coordinates, not pairs, so 100 raw pair-interactions
   collapse to X_cross=20, comfortably under the slack of 63 at m=20.
   **This collapse is a property of k=3 specifically, not of A(n,k) in
   general:** with only one non-p coordinate available (k=3 means
   positions {1,2,3}, p=1, only q=2 remains), every pair is forced
   through the same q, which is exactly why they collapse onto shared
   targets.

5. **A k=5 counter-construction shows the collapse is not universal.**
   Fix a base tuple over positions 2–5; let F*a={u_i} and F_b={v_i} for
   i=2..5, where u_i (resp. v_i) is the base with position i replaced by
   a fresh symbol α_i (resp. β_i), and position 1 fixed to a (resp. b).
   Only matched pairs (u_i,v_i) are distance-2 (mismatched i≠j give
   distance 3, contributing 0), and each matched pair uses a \_different*
   q=i, so their targets don't coincide. Hand-checked at k=5, n=14, m=9:
   4 valid pairs, X_cross=8, no collapse — confirmed distinct from the
   k=3 collapse case.

   **This shows "geometry always collapses collisions" is false as a
   general claim** — it disproves the blanket version of that claim, not
   merely restates the k=3 example. It does **not** show X_cross can be
   pushed past the slack: the one instance checked (k=5, R=8, m=9) gives
   slack=28, comfortably clear of X_cross=8.

6. **Open obstacle for partition induction: bounding X_cross.** The
   recombination step needs X_cross ≤ Δrecombination-slack from fiber
   sizes and m alone. Naive pairwise counting (c_a·c_b, O(R²)) fails —
   see point 3. Whether collisions collapse (point 4) or spread across
   distinct q's (point 5) depends on the specific construction, not on a
   general law. The k=5 spread construction needed roughly O(k) distinct
   fresh symbols to keep each pair's q unique without accidental
   collapse, which forced m to be large enough to supply them (m=9 for
   k=5 in the instance checked) — **this was only observed in the one
   construction tried, not proved as a necessary trade-off.** It remains
   open whether some other, less alphabet-hungry construction could
   achieve spread at small m, and whether any construction (collapsing,
   spreading, or otherwise) can push X_cross past the available slack at
   small m and large k. No counterexample to the recombination bound has
   been found in any case checked so far, but no proof of the trade-off
   as a general law exists either.

7. **Φ collapses to the boundary — X is not an independent adversarial
   target.** Combining three identities already established above:
   `total_coord_edges = |∂V| + X`, `total_coord_edges = U(m+1) - Rk`,
   and `D = Rk - U`, gives

   ```
   Φ = X + (m+1)D
     = [U(m+1) - Rk - |∂V|] + (m+1)(Rk - U)
     = mRk - |∂V|
   ```

   The U terms cancel identically. So Φ ≤ C(R) + mE(R) is _equivalent_,
   term for term, to |∂V| ≥ (Rk - E(R))(n-k) - C(R) — `UniversalLowerBound`
   itself, with no slack introduced or removed by the translation.

   Consequence: "maximize X_cross while keeping m small" (the natural
   next adversarial construction to try after points 4-6) is **not a
   new test**. At fixed R, k, m, maximizing X is identically minimizing
   |∂V| — the original problem restated. A construction with large
   X_cross but also large |∂V| proves nothing; only |∂V| ever mattered.
   This also explains the tautology caught earlier in point 3/Strategy
   3b's first attempt: any identity-check on a Φ-partition is true by
   construction, because Φ carries no content beyond the boundary size.

   Two arenas were considered and rejected for this reason before the
   identity was found: A(5,4) (m=1) has zero symbol freedom — the one
   remaining symbol per coordinate is forced, so no "reuse" experiment
   is even possible there — and A(6,4) at small R is already inside the
   exhaustive sweep (`scripts/sweep_universal_lower_bound.sh`, rows
   `run 6 4 2`, `run 6 4 3`), so a hand-built subset there is guaranteed
   to satisfy the inequality trivially and tests nothing new.

8. **The naive per-split induction is refuted outright (2026-09-13,
   independently verified).** An exhaustive sweep of all coordinate
   2-way splits of every V' in A(5,3) up to R=4 found 810 splits (out of
   4,074,840 checked) where the recombination penalty S exceeds the
   per-split slack bound m·ΔE(R)+ΔC(R). The worst case found:
   V' = {(0,1,2),(0,2,1),(0,3,4),(0,4,3)} in A(5,3) (n=5,k=3,m=2,R=4),
   split into F_a={(0,1,2),(0,2,1)}, F_b={(0,3,4),(0,4,3)} — two
   "swap pairs" on disjoint symbol sets, both sharing root (0,·,·)-type
   structure. Independently recomputed with `boundary_metrics` from
   `scripts/check_universal_lower_bound.py`:

   ```
   |∂F_a| = |∂F_b| = 12, D=X=0 in both fibers (each is internally clean)
   |∂V'| = 16, D(V')=0, X(V')=8
   S = |∂F_a| + |∂F_b| - |∂V'| = 12 + 12 - 16 = 8
   dE = E(4)-E(2)-E(2) = 2, dC = C(4)-C(2)-C(2) = 2
   bound = m·dE + dC = 2·2 + 2 = 6
   ```

   **S=8 > bound=6 — deficit 2.** Confirmed S is _entirely_ cross-target
   collision (X_cross=8, ΔD_cross=0) via direct decomposition:
   `∂F_a ∩ B = ∅`, `∂F_b ∩ A = ∅`, and the 8 shared external targets
   `I = ∂F_a ∩ ∂F_b` account for all of S (0+0+8=8) — the first example
   in this document where X_cross, not ΔD_cross, is the deficit's entire
   source. The global inequality still holds only because each fiber
   individually has slack: |∂F_a|-rhs(F_a) = 12-9 = 3 and likewise for
   F_b, totaling 6, which absorbs the 2-unit per-split deficit with 4 to
   spare (global: |∂V'|=16 ≥ rhs(V')=12, confirmed).

   **This is the direct analogue of Strategy 3's refutation, for the
   partition skeleton rather than the single-vertex-peeling skeleton:**
   the naive induction "sum the two fiber bounds, subtract the
   recombination penalty, compare to the target" does not close
   step-by-step, exactly as the marginal per-vertex bound failed for
   Strategy 3. The global inequality survives here only via banked slack
   carried in the fibers themselves, not via any per-split bound on S.
   As with Strategy 3, the natural fix (an amortized argument tracking
   banked slack across the recursion) has not been attempted and is not
   obviously easier than the original claim. The full R≤4
   sweep has since been independently re-run in full (all coordinate
   2-way splits of every V' ⊆ A(5,3), 4,074,840 splits): it reproduces
   the report exactly — 810 violating splits, worst deficit 2, extremal
   instance the swap-pair V' above at partition coordinate p=1 — with no
   split exceeding the fibers' combined slack (every violation is
   covered by slack banked in the two fibers).

**Status: Strategy 3b's naive per-split induction is refuted**, on the
same footing as Strategy 3 — a concrete, independently-verified
counterexample (point 8) shows the per-split recombination bound can be
violated, with the global inequality surviving only through slack
banked in the sub-fibers. Point 7 additionally rules out an entire
class of future experiments (anything phrased as "push X*cross up
while keeping m/k favorable") as vacuous, since X and |∂V| are not
independently controllable — so a repair cannot come from a better
X_cross bound alone. The remaining non-vacuous form of the question, in
boundary language rather than through X and D: **how many of F_a's
external neighbors get absorbed (cease to be external) when F_b is
unioned in?** That count is a genuine, non-tautological quantity with
the same meaning at every (n,k); whether an \_amortized* version of the
partition induction (banking slack across recursive levels, as opposed
to a per-split bound) can close is the open question left standing.

### Status of all sketches

**Both induction skeletons tried are refuted in their naive, step-local
form.** Strategy 3 (single-vertex marginal peeling) is refuted by the
A(5,3) R:2→3 counterexample (ΔX=2, s=1, LHS=5 > RHS=4). Strategy 3b
(coordinate-partition induction on Φ) is refuted by the A(5,3) R=4
swap-pair counterexample (point 8 above: S=8 > bound=6). In both cases
the global inequality survives only because slack banked elsewhere
(an earlier step, or a sibling fiber) absorbs the local deficit — never
because the local bound itself holds. Neither has an attempted
amortized/potential-method repair, and — as noted for Strategy 3 above
— any such repair that explicitly carries banked surplus across steps
is provably equivalent in strength to the original global claim, i.e.
telescopes back to Proposition 5.3 rather than simplifying it. Strategy
4 (dual-compression) is unexplored beyond one failed, degenerate
candidate operator pair. Strategy 2 (Lyapunov over fiber-size
multisets, Ψ) remains a from-scratch invariant search with no candidate
potential yet shown to work. As of this writing, none of the four
sketches has a confirmed, closed path to Proposition 5.3; the
exhaustive computational evidence (30+ rows, no counterexample) remains
the _only_ supporting evidence for the proposition.

### Entropy/Shearer candidate: tested and killed (2026-09-13)

Before writing any entropy argument by hand, the natural candidate
functional was checked empirically first, per this document's own
methodology. Definition tested: for V' with |V'|=R, per coordinate p
let {c_r} be the fiber sizes (vertices of V' sharing each root at p),
and H_p(V') = -Σ_r (c_r/R)log2(c_r/R) (Shannon entropy of the
root-distribution). Candidate conjecture: **the Hamming Ball minimizes
S(V') = Σ_p H_p(V') among all R-subsets** — the natural entropy
analogue of `thm:defect` (which the Hamming Ball maximizes for D).

Exhaustively checked over all R-subsets of A(5,3), R=2,3,4 (1770,
34220, 487635 subsets respectively — the same territory as the
existing sweep): **false at R=3.** The true minimizer is
{(0,1,2),(0,1,3),(0,1,4)} (two coordinates fixed, one varying) with
S=3.169925, strictly below the Hamming Ball's S=3.421554.

Worse than merely "wrong minimizer": checking `boundary_metrics` on
both shows the direction is backwards for the actual target. Both
configurations achieve the same maximal defect D=2=E(3) (ties exist at
R=3), but:

|                | S (entropy)   | D   | X   | \|∂V'\|                       |
| -------------- | ------------- | --- | --- | ----------------------------- |
| Hamming Ball   | 3.42 (higher) | 2   | 1   | 11 (smaller, correct optimum) |
| entropy-argmin | 3.17 (lower)  | 2   | 0   | 12 (larger)                   |

The configuration with strictly _lower_ total root-distribution entropy
has a strictly _larger_ boundary. So this functional does not merely
fail to recover the exact C(R)+mE(R) staircase (the smooth-vs-discrete
risk already flagged) — it fails to correlate with boundary size in
the required direction on the first non-trivial case tested. This
specific candidate is dead; it does not, by itself, rule out some other
entropy-type functional (a different random variable, a different
weighting) — none has been proposed or tested.

### LP-dual candidate (Vector B): tested and killed, more decisively (2026-09-13)

Move 2 was also tried as a fast empirical probe before any hand algebra:
solve the natural fractional LP relaxation of the vertex-boundary
problem exactly (via `scipy.optimize.linprog`, HiGHS), and compare to
the true integer-optimal boundary from exhaustive search. LP: minimize
Σ_w y_w subject to y_w ≥ x_v − x_w for every edge (v,w) of A(n,k),
Σ_v x_v = R, 0≤x≤1, y≥0 (the standard vertex-expansion relaxation:
integral solutions reproduce |∂V'| exactly).

Solved exactly for A(4,3) and A(5,3), R=2,3,4, against the true integer
minimum boundary (exhaustive search, same territory as the sweep):

| (n,k,R) | LP relaxation | true integer min |
| ------- | ------------- | ---------------- |
| (4,3,2) | 0             | 4                |
| (4,3,3) | 0             | 5                |
| (4,3,4) | 0             | 6                |
| (5,3,2) | 0             | 9                |
| (5,3,3) | 0             | 11               |
| (5,3,4) | 0             | 12               |

**The LP optimum is exactly 0 in every case, not merely a weaker
constant.** Reason (structural, not numerical): the uniform fractional
point x_v = R/N for every vertex is feasible (sums to R, stays in
[0,1]) and makes every edge difference x_v−x_w=0, so y≡0 satisfies
every constraint. This relaxation has no mechanism forcing spread mass
to generate boundary — a known failure mode of naive vertex-isoperimetric
LP relaxations. Consequence: there is no dual certificate to examine at
all; the earlier framing ("banked slack looks like a dual certificate")
had nothing to attach to, since the primal never leaves zero. This
kills the naive LP-relaxation approach more decisively than the entropy
candidate (which was at least directionally informative, if wrong) —
it does not rule out a smarter relaxation (e.g. one with symmetry-
breaking or higher-order constraints), but none has been proposed.

**Status of Vectors A and B after empirical testing:** both proposed
"heavy hammer" replacements for the failed inductions have been tried
in their most natural form and killed outright, in each case faster
than an hour of hand algebra would have taken and for a more decisive
reason than the a priori staircase-vs-smooth risk. Neither result rules
out a more sophisticated version of either tool; none has been proposed.

### The open problem, restated as a capacity/pigeonhole aggregation bound (2026-09-13, corrected)

**Correction to this section, caught immediately after first writing
it:** the original version of this section claimed only distance-2
pairs between F_a and F_b contribute to shared external neighbors,
citing the earlier-proved "ΔX=0 at adjacency" lemma to dismiss
distance-1 pairs. That citation was a category error: the ΔX=0 lemma
is about a different setting (single-vertex marginal peeling, Strategy
3 — the change in X when adding one vertex to an existing set), not
about two fixed, disjoint fibers sharing external neighbors. Checked
directly and found false: A(6,3), F_a={(0,1,2)}, F_b={(3,1,2)} (a
distance-1 pair, sharing a root at p=0) gives eb(F_a)∩eb(F_b) =
{(4,1,2),(5,1,2)}, size 2 — nonzero, and exactly m−1 (m=3 here). This
is a real, independent channel this section's first draft omitted
entirely.

**The corrected per-pair picture has two channels, not one:**

1. **Distance-1 (root-sharing) channel — genuinely m-linear.** If
   u∈F*a, v∈F_b share a root at the partition coordinate p (agree
   everywhere except p), every extension (r, γ) with γ not already
   used by u or v is a shared external neighbor: exactly
   m+1−|{u_p,v_p}| = m−1 of them (generalizing to c_a,c_b>1 sharing one
   root: m+1−|A_r∪B_r| where A_r,B_r are the symbols already used at p
   by members of F_a,F_b sharing that root). This is the \_only* channel
   whose count is literally linear in m, verified at m=1 (A(4,3)) where
   it correctly vanishes (m−1=0).

2. **Distance-2 (midpoint) channel — gated, but m-free per pair.** For
   u∈F_a, v∈F_b differing at p and exactly one other coordinate q, the
   candidate cross-vertex w = u with position p swapped to β=v_p is a
   valid, distinct boundary point **iff β≠u_q**; symmetrically the
   other candidate exists iff α≠v_q. **These are two independent
   conditions, not one** — verified directly: A(5,3), u=(0,1,3),
   v=(1,2,3) (distance 2, p=0, q=1) has β=1=u_q (first candidate
   invalid) but α=0≠v_q=2 (second candidate valid), giving
   eb(u)∩eb(v) = {(0,2,3)}, size exactly 1 — confirming the two gates
   fire independently, not together. Each pair contributes 0, 1, or 2,
   and m does not appear in the per-pair condition itself (this part of
   the original section was correct).

**And the aggregation is not pair-disjoint — the per-pair sum is only
an upper bound**, not an exact count: distinct (u,v) pairs can generate
the _same_ target w, exactly the k=3 collapse mechanism already
established elsewhere in this document. So the true shared-boundary
count I = eb(F_a)∩eb(F_b) is bounded above by (channel 1 total) +
(channel 2 pair-sum), with equality failing whenever two pairs collide
on a target — which is precisely the phenomenon the capacity bound
needs to control, not a side issue.

**The remaining open problem, precisely, and now correctly scoped:**
bound |I| = |eb(F*a)∩eb(F_b)| from (c_a, c_b, m) alone, where I is the
union of \_both* channels above, adjusted for their mutual overcounting.
This is strictly harder than "sum a 0–2 count over distance-2 pairs" —
that undercounts (by omitting channel 1) and overcounts (by ignoring
collisions) at the same time. No such bound has been derived or
attempted. m enters through two distinct mechanisms now identified —
root-sharing capacity (channel 1, directly linear) and cross-pair
symbol supply for spreading (channel 2, only via the aggregate
collapse/spread trade-off) — and any correct bound has to account for
both plus their interaction. This is the trailhead for whoever picks
this up next; the first attempt at stating it (this section, initial
version) undercounted the problem's own difficulty, which is itself
worth remembering before trusting the next draft of it either.

The two concrete examples above (the channel-1 `m-1` identity and the
channel-2 independent-gating example) are mechanically checked, not
just re-verified in Python twice, in
`proofs/Arrangement/CapacityBoundExamples.lean` (`lake build
Arrangement.CapacityBoundExamples`, no `sorry`). That file does not
attempt the open aggregation bound itself — it only pins down, at the
kernel level, the two facts any future attempt at that bound has to
remain consistent with.

#### A candidate (c_a,c_b,m)-bound on |I|, and why it can't be the missing piece (2026-09-13)

**Candidate:** `|I| ≤ c_a·c_b·f(m)`, where `f(m) = max(m-1, 2)` for
`m≥2` and `f(1) = 1`. `f(m)` is exactly `max_I` at `c_a=c_b=1`
(single-pair case), confirmed by full exhaustive search over all pairs
for `m = 1..6` (independent of `k`: A(5,3) and A(6,4) both give `f(2) =
2`) — it is the closed form the two channels predict: channel 1
(`m-1`, linear) dominates for `m≥3`; channel 2's fixed 0/1/2 cap
dominates at `m=2`; `m=1` is a genuine singularity (too few free
symbols for channel 2's second gate to fire independently).

**Verified as a valid upper bound in every tested cell, tight only in
a small-merge regime, then strictly loose:** exhaustive search over
`(c_a,c_b)` up to `(4,4)` in A(4,3)/A(5,3)/A(6,3)/A(7,3) (two
independent runs, matching) gives exact ties at `(1,1)`, `(1,2)`,
`(1,3)`, `(2,2)` for every `m` tested — including the `(2,2)`
swap-pair counterexample from Strategy 3b hitting it exactly (`|I|=8`
at `m=2,3`, `|I|=12` at `m=4`) — then strictly loose once either side
reaches 3 or the sizes are lopsided: `(1,4)`/`(4,1)` cap at `f(m)·1·3`
not `f(m)·1·4` (the single-vertex boundary itself has only `mk`
slots — a separate, trivial cap that binds here); `(3,2)` in A(5,3)
gives `9 < 12`; `(3,3)` in A(4,3) gives `7 < 9`, with the shortfall
traced (by hand, for that exact witness) to individual pairs failing
to _simultaneously_ realize their per-pair maximum, not to the
overcounting collision effect flagged above (the naive pair-sum there
equals `|I|` exactly — no collisions occurred, and it still fell 2
short of the product bound). Zero violations found in 18 tested cells,
across two independent implementations.

**Why this is as far as a `(c_a,c_b,m)`-only bound on `|I|` can
usefully go.** The bound is tight exactly in the small-merge regime an
induction step would actually use — and there, tightness is fatal:
the `(2,2)` swap pair achieves `|I|=8` matching the bound exactly,
which is the _same_ configuration where the naive per-split induction
was refuted (`S=8 > 6`, Strategy 3b's original counterexample). A
sharper `(c_a,c_b,m)`-bound on `|I|` cannot rescue the induction
skeleton, because the current bound already reproduces the refuting
witness at full tightness — there is no daylight left to close in that
direction. `|I| ≤ f(c_a,c_b,m)` is therefore _necessary but not
sufficient_ for `S ≤` the recombination budget. The missing margin is
not overcounting or a looser-than-needed `|I|` bound; it is **fiber
slack** — the gap between `rhs(F_a)+rhs(F_b)` and what those two
fibers' true combined capacity actually is when split off from a
larger `V'`. That reframes the trailhead: the tractable open question
is no longer "bound `|I|` from `(c_a,c_b,m)`" (answered, to the extent
that answer can help) but "bound the slack lost by splitting a set
into two fibers," i.e. amortizing `S` directly rather than continuing
to sharpen the `|I|` side of the inequality.

### The symmetric/spread counterexample hunt (2026-09-13): vacuity at scale, starvation at the frontier

A proposed adversarial program: beat the universal lower bound with highly symmetric "spread" sets (orthogonal arrays, finite-geometry designs) deployed at R values where the Hamming ball is claimed to be least efficient. Closed off on two independent grounds, both verified computationally:

1. **Vacuity at scale.** With the correct RHS, rhs(n,k,R) = (Rk − E(R))·(n−k) − C(R): rhs(7,3,29) = 5 but rhs(7,3,R) ≤ 0 for all R ≥ 30 (−41 at R=42). In A(7,3) the bound asserts nothing past R≈29; "beaten at scale" is structurally impossible there. The mechanism is generic for fixed k: Rk − E(R) behaves like R(k − ½·log₂R), turning negative against the growing correction constant, so the bound's content is confined to roughly R ≤ 2^k.

2. **Starvation at the frontier.** Where the bound bites (A(6,3), A(7,3) at R ∈ {6,7,8}, targets 24/25/24 and 35/37/36), pairwise-distance-2 sets have zero internal edges → external boundary ≈ R·k(n−k) minus modest overlap, far above tight targets. Greedy spread sets: boundaries 42–77 vs targets 24–37. Random search 60k/cell (plus an independent 20k/cell rerun): no set below target. The strongest structured candidate — the OA family {(a,b,a+b) mod 7} in A(7,3), 30 injective triples, pairwise dist ≥ 2 — exhaustively checked over all C(30,9) = 14,307,150 nine-subsets: min |extbd| = 71 > 40 = target at R=9.

Scope honesty: random/structured probes, not exhaustive (the sweep stops at R=5 for k=3); they do not prove the bound at the frontier — they refute the "try harder designs at scale" program, whose target regime is empty.

Formula-slip note (2026-09-13): an early draft of this probe quoted the wrong RHS, C(R)+mE(R), printing "target=68" at A(7,3) R=9; the correct RHS is (Rk−E(R))·m − C(R) and the true target there is 40. Conclusion unaffected (71 > 40 still holds), but the wrong number appeared in the initial discussion and should not be reused.

### The amortized-slack lemma candidate (2026-09-13, evidence gathered, not a proof)

Reframing from "bound |I| from (c_a,c_b,m)" (answered, and shown incapable
of rescuing Strategy 3b — see above) to the actual remaining lever: whether
every split of a set into two fibers always carries enough banked boundary
slack to cover its own recombination penalty.

**The exact algebraic identity.** For a split V' = F_a ⊔ F_b with sizes
c_a, c_b (R = c_a+c_b), define the slack of any subset X as
S(X) = |extbd(X)| − rhs(|X|), where rhs(R) = (Rk−E(R))·(n−k) − C(R) is the
conjectured true minimum boundary (Proposition 5.3's RHS). Then, **provided
F_a and F_b share no direct edges to each other**:

    S(V') = S(F_a) + S(F_b) + Delta − I

where Delta = rhs(c_a) + rhs(c_b) − rhs(R) is the recombination budget and
I = |extbd(F_a) ∩ extbd(F_b)| is the shared-boundary count from the two
channels above. Verified exactly against the known swap-pair witness
(A(5,3), R=4): S(F_a)=S(F_b)=3, Delta=6, I=8, giving
3+3+6−8=4=S(V') — matches the directly-computed value exactly.

**Caveat found and confirmed by direct construction, not assumed:** this
identity is _not_ unconditional — it breaks by exactly +2 per mutual edge
when F_a and F_b are directly adjacent to each other (checked on the
minimal case, an adjacent pair in A(4,3): the naive identity predicts
S(V')=1, the true value is 0). Any general induction built on this
identity has to handle direct F_a–F_b adjacency as a third effect, on top
of the two shared-neighbor channels already catalogued; this has not been
done.

**The critical-case test, corrected (2026-09-13, same session, second
pass).** An amortized induction's dangerous case is exactly
S(F_a)=S(F_b)=0 (both fibers individually tight). The first pass at this
test (table below, struck through) checked only I < Delta, using the
direct-adjacency caveat above as a footnote rather than folding it into
the quantity being tested. That was an error, not merely an
incompleteness: the exact identity derived above is

    S(V') = S(F_a) + S(F_b) + Delta − (I + B_ba + B_ab)

where B_ba = |F_b ∩ ∂F_a| and B_ab = |F_a ∩ ∂F_b| are the direct
F_a↔F_b adjacency crossover counts (derived cleanly: ∂F_a = E_a ⊔ B_ba
with E_a = ∂F_a \ F_b, so ∂V' = E_a ∪ E_b exactly and E_a ∩ E_b = I —
this decomposition was checked against the algebra independently before
trusting it). The critical-case quantity that actually has to stay
`≤ Delta` is **I + B_ba + B_ab**, not I alone; the first-pass table
below undercounted it whenever F_a, F_b shared a direct edge (which the
search already allowed — there was never an adjacency filter to drop,
only a term missing from what was measured).

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

The "margin widens as sizes grow" conclusion drawn from this table does
**not** survive the correction below and should not be reused.

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

**This is a materially different picture than the first pass, not just
a smaller margin.** The corrected quantity hits the bound **exactly**
(margin 0, i.e. S(V')=0 exactly at that split) in 13 of 17 cells — the
inequality is _tight_, not comfortably loose. Zero violations
(margin > 0) found anywhere. Equality that consistent is usually
explained by a clean bijective/counting argument, not a slack
inequality with room in it — and the 4 negative cells now pin down
exactly when that argument would have to break:

**"Sharp pattern," 17/17 cells — REFUTED at the 18th (2026-09-13, later
same session).** The embeddability rule above (margin = 0 iff
`n−k ≥ ⌈log₂(c_a+c_b)⌉`) was reported as holding with zero exceptions
across 17 cells, framed as a candidate exact law. It is false: **A(7,3),
c_a=c_b=5** has `n−k=4`, `R=10`, `⌈log₂10⌉=4`, so `4≥4` — the rule
predicts margin=0 — but the measured value is **margin=−2**. Caught
immediately by running one more cell rather than stopping at 17, and
retracted here rather than left standing. Full data (25 cells; two
still running when this was written and not included:
(7,3,6,6) is in progress):

| cell | n−k | R | ⌈log₂R⌉ | margin |
|---|---|---|---|---|
| (5,3,1,1)..(5,3,1,3), (6,3,1,1),(6,3,2,2),(6,3,1,2) | 2,2,2,2,3,3,3 | 2,4,3,4,2,4,3 | ≤2,≤2,≤2,≤2,≤2,≤2,≤2 | 0 (7 cells) |
| (5,3,3,3) | 2 | 6 | 3 | −2 |
| (5,3,4,4) | 2 | 8 | 3 | −6 |
| (5,3,2,3) | 2 | 5 | 3 | −1 |
| (5,3,1,4) | 2 | 5 | 3 | −1 |
| (5,3,2,4) | 2 | 6 | 3 | −2 |
| (5,3,3,4) | 2 | 7 | 3 | −4 |
| (6,3,3,3) | 3 | 6 | 3 | 0 |
| (6,3,3,4) | 3 | 7 | 3 | 0 |
| (6,3,4,4) | 3 | 8 | 3 | 0 |
| (6,3,2,3) | 3 | 5 | 3 | 0 |
| (6,3,4,5) | 3 | 9 | 4 | −5 |
| (6,3,5,5) | 3 | 10 | 4 | −8 |
| (6,3,3,6) | 3 | 9 | 4 | −2 |
| (6,3,4,6) | 3 | 10 | 4 | −4 |
| (6,3,6,6) | 3 | 12 | 4 | −10 |
| (7,3,4,4) | 4 | 8 | 3 | 0 |
| (7,3,3,3) | 4 | 6 | 3 | 0 |
| (7,3,5,5) | 4 | 10 | 4 | **−2** (breaks the rule: `n−k≥⌈log₂R⌉` here) |

The embeddability rule correctly predicts every `margin=0` cell
(`n−k ≥ ⌈log₂R⌉` held in all of them) but is not sufficient: it also
holds for (6,3,2,3), (6,3,3,4), (7,3,3,3), (7,3,4,4) with margin 0 —
and now also for (7,3,5,5) where margin is −2. So `n−k ≥ ⌈log₂R⌉` is
necessary-looking but not sufficient for margin=0; something about the
specific sizes (c_a,c_b), not just R and n−k, also matters, the same
way it already mattered for the deficit magnitude in the
non-embeddable regime (A(6,3) R=9: (4,5)→−5 vs (3,6)→−2; R=10:
(5,5)→−8 vs (4,6)→−4 — recorded above, and now shown to extend to
whether the deficit is zero at all, not just how large it is when
nonzero). No closed-form invariant is currently known. Do not cite the
embeddability rule as more than "correctly predicts margin=0 in every
case checked so far where it says 0, but is known incomplete."

The open lemma is therefore back to its unreframed form: **characterize
exactly when tight F_a, F_b give I+B_ba+B_ab = Delta versus a strict
deficit, and bound the deficit when it is nonzero.** 25 data points and
a partial, known-incomplete rule — not a proof, and not as sharp a
reframing as previously claimed.

margin=0 at (1,1) is not a coincidence to explain away: it is exactly
the R=2 case (an adjacent pair achieving rhs(2) exactly, S(V')=0 by
the Hamming Ball Evaluation proposition), reappearing correctly once
`B_ba`/`B_ab` are counted. The very first version of this corrected
tool flagged margin=0 as `CRITICAL-CASE VIOLATION` — a real bug (wrong inequality
direction: equality is not a violation of `S(V') ≥ 0`), caught and
fixed before any of the numbers above were trusted.

Also verified as a sanity check (not new information, but confirms no
bug in the identity/search code): the TRUE minimum of S(V') over all
splits, exhaustively, at R=2 and R=3 is exactly 0, matching Proposition
5.3 / the Hamming-ball-evaluation proposition exactly.

**One genuine edge case surfaced while extending the table, not a bug:**
A(5,3) c_a=4, c_b=5 has zero tight fibers of size 5 at all
(`#tight_b=0`) — confirmed independently in both the Python and C++
implementations. This is the achievability log-condition
(n−k ≥ ⌈log₂R⌉) failing, not a search error: in A(5,3), n−k=2 <
⌈log₂5⌉=3, so the Hamming ball construction cannot even embed at R=5
there, and nothing else hits rhs(5) exactly either. A useful independent
confirmation that the achievability caveat already in the paper
(Section 1) is the real boundary, not a formality.

**Ported to C++** (`src/check_amortized_slack.cpp`, `make
check_amortized_slack`) after the Python version stalled at A(6,3)
c_a=c_b=4 (it did eventually finish and cross-validated exactly:
max(I−Delta)=−8 both ways). The C++ version is a straight port —
bitset boundary computation instead of Python sets, same combinatorial
search, same output format — cross-validated against every Python
result above before being trusted on new cells; it reaches (6,3,5,5)
(checking ~2M disjoint tight-fiber pairs) in under 10 seconds where
the Python equivalent would not complete in reasonable time. Passes
`cppcheck` and the repo's `make lint`/`clang-format` checks cleanly.

**Second-pass update to the C++ tool (same session):** extended in place
to compute B_ba and B_ab alongside I and report the combined quantity;
cross-checked against the known swap-pair witness's non-adjacent-fiber
case (where B_ba=B_ab=0 by construction, reducing correctly to the old
I=8 value) before trusting it on adjacent pairs. Re-ran 13 of the
original cells (all but (6,3,4,5) and (6,3,5,5), not yet redone with the
corrected quantity); results are the corrected table above, not the
struck-through one.

**What is still missing before this is a lemma, let alone a proof:** (a)
a characterization of exactly when tight F_a, F_b give
I + B_ba + B_ab = Delta exactly versus a strict deficit — the
embeddability rule (n−k ≥ ⌈log₂(c_a+c_b)⌉) predicts every margin=0 case
correctly but was refuted as a complete characterization by A(7,3)
c_a=c_b=5 (embeddable by that rule, margin=−2 anyway), so this is back
to an open question, not a "why does the known pattern hold" question;
(b) a bound (not yet even conjectured in closed form) on the deficit
Delta − (I+B_ba+B_ab) when it is nonzero — now 12 data points
(−1,−1,−2,−2,−2,−2,−4,−4,−5,−6,−8,−10, plus one more pending) spanning
multiple (n,k), still not enough to guess a formula, and now known to
depend on (c_a,c_b) individually, not just on R and n−k; (c) the
direct-adjacency correction term is
now characterized exactly (the identity above) but not yet proven from
first principles independent of the computational check; (d) even
granting (a)–(c), an amortized induction also needs to handle the
non-tight case (S(F_a), S(F_b) > 0) in general, not just confirm one
already-known witness has enough margin. `scripts/check_amortized_slack.py`
(I-only, now marked superseded in its own docstring) and
`src/check_amortized_slack.cpp` (the corrected, canonical tool) are both
set up to extend the critical-case table further for whoever picks this
up next.

## Status

This is a research sketch, not a proof. Root compression (Step 4/5) is
stalled on the coordinate-tangling obstacle. Both induction-based
replacements attempted since (marginal peeling and coordinate-partition
induction) are refuted in their naive form — see "Status of all
sketches" above for the final state of the board. The two remaining
unexplored directions (dual-compression averaging and the Lyapunov
function over fiber-size multisets) are recorded at the
whiteboard-sketch stage only; neither has been carried far enough to
check, let alone prove.

The exhaustive computational evidence (30 rows as of 2026-09-12, up to
3.65 billion subsets in a single row) supports the inequality with no
counterexample found. The proof target is precise: show
X(V') + (m+1)D(V') ≤ C(R) + m·E(R) for all V', by any of the strategies
above or another route. The existing Cheng et al. computational results
\cite{cheng2022extraconnectivity} do not supply this — they are limited to
small g and do not address the all-subsets vertex-isoperimetric question.
