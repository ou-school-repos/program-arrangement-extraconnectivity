# A budgeted phase-envelope framework for arrangement graphs

This note organizes the fixed-volume boundary problem by the invariants that
control it. It separates exact identities from the proposed finite-signature
reduction, and distinguishes the graph-connected catalogue already in the
repository from the distance-two-connected patterns needed for unrestricted
profiles.

## 1. Basic setup and the exact objective

Let `A(n,k)` be the graph whose vertices are injective `k`-tuples from an
`n`-symbol alphabet; two vertices are adjacent when they differ in one
coordinate. Write `m=n-k`. For a set `S` of `R` vertices, let `N(S)` be its
external vertex boundary.

For coordinate `i`, delete coordinate `i` from every member of `S`, and let
`U_i(S)` be the number of distinct resulting roots. Set

$$
 U(S)=\sum_{i=1}^k U_i(S),\qquad D(S)=Rk-U(S).
$$

For each coordinate `i`, let `B_i(S)` be the external vertices reached from `S`
by changing coordinate `i`. A vertex can lie in more than one `B_i`. Define its
directional multiplicity

$$
 t_S(w)=|\{i:w\in B_i(S)\}|,
$$

and define the cross-collision excess

$$
 X(S)=\sum_{w\in N(S)}(t_S(w)-1) =\sum_i|B_i(S)|-|N(S)|.
$$

Each root supports exactly `m+1` vertices in its coordinate line. Summing
external incidences over all roots gives `U(S)(m+1)-Rk`. The same incidences
count each external vertex `t_S(w)` times. Therefore

$$
 |N(S)|+X(S)=U(S)(m+1)-Rk,
$$

and hence

$$
 \boxed{|N(S)|=Rkm-(m+1)D(S)-X(S)}. \tag{1}
$$

Equivalently, `|N(S)|=(Rk-D)(m+1)-Rk-X`. This is the exact objective
decomposition: at fixed `R,k,m`, a smaller boundary is exactly a larger score
`(m+1)D+X`. The parameter `m+1` is the weight on defect; collision excess has
weight one.

The repository formalizes the root-count defect bound `D(S) <= E(R)`, the
boundary identity, and exact pairwise penalty identities in Lean
(`ArrangementExtraconnectivity.lean`, `PenaltyExact.lean`, and `Phase.lean`).

## 2. Signatures and the embedding budget

For a finite pattern, call a coordinate active if it varies on the pattern. Let
`p` be the number of active coordinates. Among symbols appearing in active
coordinates, let `s_a` be the number of distinct symbols, and put `e=s_a-p`.

The `p` active coordinates use `p` distinct symbols in any chosen base vertex.
The other `e` symbols are additional symbols needed to realize the pattern. The
pattern embeds in `A(n,k)` exactly when

$$
 p\le k,\qquad e=s_a-p\le m. \tag{2}
$$

For sufficiency, map the base symbols to `p` distinct target symbols, map the
`e` additional symbols to `e` symbols outside that base, and fill the other
`k-p` coordinates with distinct symbols unused by the active pattern. The second
inequality is precisely `n-s_a >= k-p`. Necessity follows because those `e`
additional symbols must occupy positions outside the `p` base symbols, of which
the target alphabet has only `m` spare symbols.

The signature

$$
 \sigma(S)=(D(S),X(S),p(S),e(S))
$$

therefore determines both its boundary line (1) and its feasibility region (2).
A signature `a` safely dominates signature `b` for every budget only if

$$
 D_a\ge D_b,\quad X_a\ge X_b,\quad p_a\le p_b,\quad e_a\le e_b,
$$

with at least one strict inequality. Dominance in `(D,X)` alone is not enough:
the better-scoring pattern might require more coordinates or symbols and be
infeasible in the cell under consideration.

## 3. The finite-envelope theorem, with its actual scope

Let `C_R` be a class of `R`-vertex patterns for which a complete list of
signatures has been obtained. For each signature define

$$
 b_\sigma(n,k) = Rkm-(m+1)D_\sigma-X_\sigma.
$$

Then the minimum boundary over that class is exactly

$$
 \min_{\substack{\sigma\in\Sigma_R(C)\\p_\sigma\le k,\ e_\sigma\le m}}
b_\sigma(n,k). \tag{3}
$$

This follows directly from (1), provided the signature list is complete and each
listed signature has a representative that embeds whenever (2) holds. As `m`
varies, each candidate is an affine line of slope `Rk-D`; the lower boundary
envelope changes at line crossings and when a signature first becomes feasible
at its symbol budget. In score coordinates, the same statement is that the
winner maximizes `(m+1)D+X` among budget-feasible points.

Equation (3) is an exact representation, not yet a closed-form solution: the
hard part is determining the complete signature frontier and proving which
signatures can be omitted.

### Which connectedness notion is being catalogued?

There are two relevant classes:

- `graph-connected`: connected using edges of `A(n,k)` (one-coordinate changes);
- `distance-two-connected`: connected in the graph joining two arrangement
  vertices whenever their arrangement-graph distance is at most two.

The checked-in `src/pattern_catalogue.cpp` enumerates graph-connected sets. Its
prediction is exact for the graph-connected profile if enumeration is complete;
it is also a valid witness upper bound for the unrestricted profile. It does not
enumerate every distance-two-connected set and therefore does not by itself
certify the unrestricted profile.

There is a universal finite-host argument for either class:

- A graph-connected `R`-set has a spanning tree with `R-1` edges. Each edge
  changes one coordinate and introduces at most one new symbol. Thus `p<=R-1`,
  `e<=R-1`, and `A(2R-2,R-1)` is a sufficient host.
- A distance-two-connected `R`-set has a spanning tree whose edges change at
  most two coordinates and introduce at most two new symbols. Thus `p<=2(R-1)`,
  `e<=2(R-1)`, and `A(4R-4,2R-2)` is a sufficient host.

These are completeness bounds, not runtime bounds. The host vertex count grows
factorially, so the universal-host argument does not make the enumeration
practical. For unrestricted profiles, the second host or a more efficient
canonical pattern generator is needed.

## 4. A sharper collision bound: proof and status

The previous Lean development proved the collision estimate

$$
 X(S)\le 2(R^2-R),
$$

using ordered overlap charging. A multiplicity refinement now proves the
factor-one estimate

$$
 \boxed{X(S)\le R(R-1)}. \tag{4}
$$

This stronger estimate is Lean-proved as `cross_collision_bound_factor_one`.

For a finite family of finite sets `A_i`, let `t(w)` be the number of members
containing `w`. Its excess incidence count is
`sum_i |A_i| - |union_i A_i| = sum_w (t(w)-1)`, while its ordered pair-overlap
count is `sum_w t(w)(t(w)-1)`. Pointwise, `2(t-1) <= t(t-1)`, so

$$
 2\left(\sum_i |A_i|-\left|\bigcup_i A_i\right|\right) \le \sum_{i\ne
j}|A_i\cap A_j|. \tag{5}
$$

Take `A_i=B_i(S)`. The left side is `2X(S)` and the right side is the ordered
coordinate-overlap count. The repository proves that this count is at most
`2(R^2-R)` by charging overlaps to ordered source pairs together with an
orientation bit. Combining the inequalities proves (4).

The generic finite-family inequality is formalized as
`card_biUnion_excess_two_le_ordered_overlap`. Its proof adds the sets one at a
time: the increase in excess is the size of the new set's intersection with the
previous union, at most the sum of its intersections with prior sets; the
ordered overlap count increases by twice that sum. This method does not assume a
unique source in each coordinate direction. Such uniqueness is generally false
when several members of `S` lie on the same coordinate root.

The factor-one claim for the **ordered overlap count itself** is false; the
repository records a counterexample. The factor one appears after comparing
ordered overlaps with excess multiplicity. The old theorem
`cross_collision_bound_global` remains available as a weaker consequence, but
the capstone now uses the sharper estimate.

## 5. What the large-slack theorem really says

Assume the Boolean Hamming witness is feasible, so its dimension
`d=bit_length(R-1)` satisfies `d<=k` and `d<=m`. It has defect `E(R)` and
collision excess `X_H=C(R)-E(R)`. Since every set has `D<=E(R)`, sufficiently
large `m` forces every profile minimizer to have maximum defect. The previous
Lean threshold was

$$
 m+1>2(R^2-R).
$$

The factor-one theorem now gives the sharper sufficient threshold

$$
 m+1>R(R-1). \tag{6}
$$

Indeed, a set with defect at least one below `E(R)` loses at least `m+1` in the
defect score, while its collision excess is at most `R(R-1)` and the Hamming
witness has nonnegative collision excess. Above (6), no lower-defect set can
minimize.

After defect is fixed at `E(R)`, the winner is the set with largest `X` among
maximum-defect sets. Consequently, in this regime,

$$
 \Phi_{n,k}(R)=Rkm-(m+1)E(R)-X_*,
$$

where `X_*` is the maximum collision excess among feasible maximum-defect sets.
Hamming exactness is equivalent to `X_*=X_H`; it is **not** automatic. The
existing Lean theorem `eventual_hamming_exact_iff` proves this equivalence under
the improved threshold. It does not prove that the Hamming ball is
collision-maximal for every `R`.

Also, “large `n`” alone is not enough if `k` is fixed below `d`: the Hamming
pattern then remains infeasible. The maximum-defect conclusion applies to the
maximum defect among patterns that fit the coordinate budget; the global `E(R)`
conclusion needs the embedding gate.

## 6. From distance-two components to the unrestricted profile

Let `Psi_{n,k}(r)` be the minimum boundary of an `r`-set connected in the
distance-two graph. If `S` has distance-two components `C_1,...,C_t`, then
distinct components are at arrangement-graph distance at least three. Their
external boundaries are disjoint and avoid the other components. Therefore

$$
 |N(S)|=\sum_{i=1}^t |N(C_i)|.
$$

It follows that

$$
 L_{n,k}(R):=\min_{\lambda\vdash R}\sum_{r\in\lambda}\Psi_{n,k}(r) \le
\Phi_{n,k}(R)\le\Psi_{n,k}(R). \tag{7}
$$

where `lambda` ranges over integer partitions of `R`. The left inequality is a
lower bound: the independently optimal parts in the sum may not be
simultaneously placeable at mutual distance at least three. The upper inequality
uses a distance-two-connected minimizer as a candidate. If
`L_{n,k}(R)=Psi_{n,k}(R)`, then (7) certifies `Phi_{n,k}(R)=Psi_{n,k}(R)` and
proves that no disconnected arrangement does better.

This is the right interpretation of the small split checks. Checking only
two-part splits is sufficient only after showing that every finer partition has
at least the same lower-bound sum; for the cited small examples, that can be
checked directly from the already certified smaller-volume values.

## 7. The separate bridge to extra-connectivity

The fixed-volume profile does not by itself determine `g`-extra connectivity.
Under the usual convention, a `g`-extra cut disconnects the graph and every
remaining component has at least `g+1` vertices.

For any such cut `F`, choose a smallest component `C` of `A(n,k)-F`. Then
`g+1<=|C|<=|V(A(n,k))|/2`, and `N(C)` is contained in `F`. Hence

$$
 \kappa_g(A(n,k))\ge \min_{g+1\le r\le |V(A(n,k))|/2}\Phi_{n,k}(r). \tag{8}
$$

For an upper bound, a candidate `S` must satisfy more than having small
`|N(S)|`: after deleting `N(S)`, the component `S` and every other surviving
component must each have at least `g+1` vertices. A residual-connectivity lemma
for a particular family, such as partial stars, can provide this validity check.
There is no general theorem that an isoperimetric minimizer automatically gives
a valid extra cut.

## 8. Status and proof plan

### Established in the repository

1. The exact boundary/defect/collision identity and pairwise comparison.
2. The defect bound `D<=E(R)` and an explicit Hamming-ball witness when the
   embedding gate is open.
3. The factor-one collision bound `X<=R(R-1)` and the resulting sharper Hamming
   sandwich.
4. The eventual maximum-defect theorem and the equivalence between eventual
   Hamming exactness and collision maximality.
5. Small computational profiles with separate distance-two split checks.

### Still open

1. Build a complete, budget-aware signature catalogue for distance-two-connected
   patterns, or prove a smaller complete structural classification.
2. Prove which signatures lie on the feasible upper hull. The Hamming,
   folded-box, Star, and hybrid families are candidates, not a complete
   classification.
3. Establish unrestricted profile values by combining the distance-two envelope
   with split bounds such as (7).
4. Prove residual component-size/connectivity conditions and combine them with
   (8) for extra-connectivity.

### Recommended order

1. State a Lean finite-signature envelope theorem abstractly: given a finite
   list and a completeness/realizability hypothesis, the minimum is the feasible
   minimum of its affine costs. This formalizes the arithmetic without claiming
   the catalogue is complete.
2. Change or supplement the C++ catalogue to enumerate distance-two-connected
   patterns, and verify its signatures against direct boundary recounts and the
   existing exact-profile enumerator on small cells.
3. Use integer-partition split bounds to certify unrestricted minima.
4. Prove a separate valid-cut lemma before drawing any conclusion about
   `kappa_g`.

The finite-envelope representation is therefore a useful general theory of _how_
phases are compared. A closed-form phase diagram still requires the structural
classification and completeness results in items 2–4.
