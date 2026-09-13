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

## Proposed Approach: Coordinate-Root Compression in H(k,n)

### Step 1: Lift to the Hamming Graph

A(n,k) embeds naturally in the Hamming graph H(k,n) = K_n □ ... □ K_n
(k copies). Vertices of H(k,n) are all k-tuples over {0,...,n−1}
(no injectivity constraint). The arrangement graph is the induced
subgraph on injective tuples.

The fiber structure extends: at coordinate p, the root is the (k−1)-tuple
obtained by deleting position p. Two vertices share a root iff they differ
at exactly position p.

### Step 2: Known Result for H(k,n)

In H(k,n), the initial segment of colex order (the "Hamming ball" in
the unconstrained setting) minimizes the vertex boundary. This follows
from the Kruskal-Katona theorem applied to the hypercube structure:
compression toward colex is monotone for the boundary.

The proof uses the standard shift operation: if a vertex v uses symbol b
at position p and no vertex uses symbol a < b at position p, shift b → a
at position p. This is monotone for the boundary in H(k,n) because
there is no injectivity constraint — the shift only moves v into a
closer fiber, never displacing other vertices.

### Step 3: Transfer Inequality from H(k,n) to A(n,k)

The key observation: for any V' ⊆ A(n,k), the fiber structure in A(n,k)
is a _restriction_ of the fiber structure in H(k,n). Specifically:

- In H(k,n), each root at position p has fiber size (n−1)!/(n−k+1)!.
  In A(n,k), each root at position p has the same fiber size (since
  the root is a (k−1)-tuple of distinct symbols, and extending it to a
  k-tuple with a fresh symbol at position p gives (n−k+1) choices).

- The collision count X(V') in A(n,k) is computed from the same root
  fibers as in H(k,n). The only difference is that some roots may have
  fewer members in A(n,k) if the injectivity constraint eliminates
  extensions.

- The defect D(V') = Rk − |roots(V')| counts repeated roots, which is
  the same in both graphs (the roots are (k−1)-tuples of distinct
  symbols in both cases).

### Step 4: The Weighted Potential and Compression

The critical difference from the standard approach: we do NOT compress
vertices directly. Instead, we compress **roots**.

Define a root compression at position p: given two roots r₁ <\_colex r₂,
if r₂ has more V'-members than r₁, "shift" one vertex from the r₂-fiber
to the r₁-fiber (changing its symbol at position p to match r₁).

This operation:

1. Preserves |V'| (we move, not copy)
2. Preserves injectivity (the target root r₁ has a fresh symbol available
   at position p, since r₁ and r₂ differ at some position in the
   (k−1)-tuple, so the extension symbol for r₁ is different)
3. Increases X: moving a vertex into a larger fiber creates new collisions
   within that fiber
4. Increases D: the moved vertex now shares a root with more vertices

The claim is that this root compression is monotone for the weighted
potential X + (m+1)D. If true, iterating root compressions at every
coordinate converges to a configuration where roots are in colex order
with maximal fiber occupancy — which is exactly the Hamming ball.

### Step 5: Why This Might Work (and Why It's Hard)

The root compression concentrates vertices in fewer fibers, increasing
both X and D. Since the weighted potential is X + (m+1)D with m+1 > 1,
the D-increase is amplified. The boundary decrease follows from the
fiber identity.

The difficulty: when we move a vertex from fiber r₂ to fiber r₁ at
position p, we may affect the root structure at OTHER positions q ≠ p.
The moved vertex changes its symbol at position p, which changes its
root at every other position q. This could decrease X or D at other
coordinates.

The Hamming ball avoids this problem because it is "aligned" — all
vertices share the same first d coordinates, so the fiber structure is
nested across positions. A general configuration is not aligned, and
root compression at one position can disrupt the structure at others.

### Possible Resolution

1. **Coordinate-by-coordinate with repair**: compress at position p,
   then repair the damage at positions q ≠ p by further compressions.
   Show that the total effect across all coordinates is non-negative
   for the weighted potential.

2. **Global potential function**: define a Lyapunov function (e.g.,
   lexicographic order on the multiset of fiber sizes across all
   coordinates) and show that root compression increases it. Since the
   Hamming ball is the unique maximum of this Lyapunov function, the
   iteration terminates there.

3. **Induction on R**: for R = 1, trivial. For R > 1, remove a vertex
   from V' and use the induction hypothesis on the remaining R−1 vertices.
   Show that adding the vertex back does not decrease the weighted potential
   below the bound.

4. **Direct counting**: decompose X(V') + (m+1)D(V') into a sum over
   coordinates and roots, and bound each term using the structure of the
   root fibers.

## Status

This is a research sketch, not a proof. The root compression approach
(Step 4) is the most promising direction but the multi-coordinate
interaction (Step 5) is the main obstacle. Options 2-4 above are
alternative resolution strategies.

The exhaustive computational evidence (24 rows, up to 190M subsets)
supports the inequality. The proof target is precise: show that root
compression is monotone for X + (m+1)D, or find an alternative global
argument.
