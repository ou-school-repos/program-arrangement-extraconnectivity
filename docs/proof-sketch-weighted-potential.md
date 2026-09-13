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

## Status

This is a research sketch, not a proof. The root compression approach
(Step 4) is the most promising direction but the multi-coordinate
interaction (Step 5) is the main obstacle. The five resolution strategies
above are unexplored.

The exhaustive computational evidence (24 rows, up to 190M subsets)
supports the inequality. The proof target is precise: show that root
compression is monotone for X + (m+1)D, or find an alternative global
argument. The existing Cheng et al. computational results \cite{cheng2022extraconnectivity}
do not supply this — they are limited to small g and do not address the
all-subsets vertex-isoperimetric question.
