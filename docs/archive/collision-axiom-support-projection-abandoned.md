# Archived: Support-Projection / Boolean Cube Injection Approach (Abandoned)

**Status: ABANDONED, NOT VALIDATED.** This document preserves a formalization
strategy that was explored for closing `UniversalLowerBound` via Mathlib's
Kruskal-Katona theorem, kept here for historical record only. It is **not**
part of the active roadmap (see `docs/collision-axiom-roadmap.md`), and none
of the claims below should be cited as established results.

The core inequality this approach depended on,

```
sum_unique_roots(V') ≤ k · |φ(V')| − |∂(φ(V'))|
```

has a known **singleton counterexample** that refutes it in its stated form
(see the note originally attached to this section). The colex correspondence
between the support-projection family and the Kruskal-Katona initial segment
was never established, and the final "pullback" step from the projected
hypercube bound back to the arrangement-graph bound was never derived. The
closing claim that this strategy "completely eliminates the factorial trap
... allowing us to mechanize the complete proof of `lower_bound_all_embeddings`
and `external_neighbors_collision_bound`" was an overclaim that contradicted
the unresolved status of the steps immediately preceding it, and should not
have stood as the section's conclusion. (Those two identifiers are also
themselves stale: the current hypothesis interfaces in
`ArrangementExtraconnectivity.lean` are `UniversalLowerBound` and
`HBCrossCollisions`, not raw axioms of those names.)

If this direction is revisited, it needs, at minimum: a corrected/proven
version of the shadow-count inequality above (or a replacement), an explicit
proof of the colex correspondence between `φ(V')` and Kruskal-Katona initial
segments, and an explicit pullback lemma from the hypercube shadow bound back
to `sum_unique_roots`. None of that work has been done.

## Step 3 (archived): Transfer to Arrangement Graphs

### What's needed

Show that the permutation constraint (no duplicate symbols) in A(n,k) only
**removes** edges compared to the full Hamming graph H(k,n). Therefore:

- Any collision bound proven for hypercubes transfers to A(n,k)
- The arrangement graph can only have **fewer** collisions than the hypercube
- This means the external boundary is at least as large as the hypercube bound

### Key lemma (unvalidated — the inequality chain below is directionally invalid)

```
A(n,k) ⊆ H(k,n)  as graphs (isometric embedding)
⟹ squares_A(S) ≤ squares_H(S)  for any S
⟹ collisions_A(S) ≤ collisions_H(S)
⟹ |N_A(S)| ≥ |N_H(S)|
```

This chain was never rederived after being flagged as invalid; do not treat
it as a completed reduction.

## Future Work (archived): Bypassing Custom Compressions via Boolean Cube Injection

### The claimed breakthrough

Previously, it was assumed that formalizing the Kruskal-Katona shadow bounds
in $A(n,k)$ would require developing custom, coordinate-aware compression
operators on the ordered, injective sequences of $A(n,k)$ from scratch. The
idea explored here was to bypass this by projecting subsets of $A(n,k)$
directly into the Boolean hypercube, where Mathlib's standard, built-in
Kruskal-Katona theorem (`Mathlib.Combinatorics.SetFamily.KruskalKatona`)
could in principle be applied as-is.

### 1. The Support Projection to k-Subsets

Every vertex $v \in A(n,k)$ is an injective sequence of length $k$ using
symbols from $[n]$. The image of $v$ (its set of active symbols) is therefore
a subset of $[n]$ of size exactly $k$. Define the support projection $\phi$:

$$\phi(V') = \{ \text{image}(v) \mid v \in V' \} \subseteq \mathcal{P}_k([n])$$

In Lean, for any $V' : \text{Finset } (A(n,k))$, its support projection
$\phi(V')$ is a set family of $k$-sets, which satisfies the uniform size
constraint: `Set.Sized k ↑(\phi V')`.

### 2. Standard Shadows and Arrangement Graph Collisions

Mathlib's standard shadow of $\phi(V')$, denoted $\partial(\phi(V'))$,
consists of all $(k-1)$-element subsets obtained by removing one element
from a subset in $\phi(V')$. The claimed (refuted) identity linking the two
universes was:

$$\text{sum\_unique\_roots}(V') \le k \cdot |\phi(V')| - |\partial(\phi(V'))|$$

**This inequality is false as stated** — refuted by a direct singleton
counterexample.

### 3. Applying Mathlib's Kruskal-Katona As-Is (never actually connected)

The idea was to invoke Mathlib's verified Kruskal-Katona theorem directly:

```lean
theorem Finset.kruskal_katona {n r : ℕ} {𝒜 𝒞 : Finset (Finset (Fin n))}
    (h𝒜r : Set.Sized r ↑𝒜) (h𝒞𝒜 : 𝒞.card ≤ 𝒜.card) (h𝒞 : Colex.IsInitSeg 𝒞 r) :
     𝒞.shadow.card ≤ 𝒜.shadow.card
```

But: the colex correspondence between $\phi(V')$ and a Kruskal-Katona initial
segment was never established, and the pullback from the shadow bound back to
`sum_unique_roots`/`external_neighbors_collision_bound` was never derived.
Neither the shadow inequality above nor this connection should be presented
as proven or in-progress; if pursued again, it needs to be redone from
scratch with the counterexample in hand.
