# Hypercube Edge Isoperimetry in Arrangement Graphs

## Summary of Results: Proven vs. Empirical

We maintain a rigorous distinction between formal proofs, axiomatized results,
and computer-asssted findings.

### ✓ Checked in Lean (public components)

1. **Hypercube Edge Count:** Formal inductive proof (`hypercube_edges_form` in
   `HypercubeEdges.lean`) that a $d$-dimensional Boolean hypercube contains
   exactly $E(d) = d \cdot 2^{d-1}$ internal edges.
2. **Popcount Equivalence:** $A000788(R)$ exactly matches $E(d)$ at every power
   of 2 ($R=2^d$).
3. **E_seq Subadditivity:** `E_add_min_le` proves
   $E(x) + E(y) + \min(x,y) \le E(x+y)$ for all $x,y$.
4. **Generalized Partition Bound:** `E_seq_list_sum_le` extends binary
   subadditivity to arbitrary partitions.
5. **Defect Fiber Bound:** The 7-step topological decomposition
   `defect_fiber_bound` proving $D(V') \le \sum D(F_s) + R - y$ via root
   disjointness.
6. **Universal Defect Bound:** `sum_unique_roots_lower_bound` proves
   $D(V') \le E_{seq}(R)$ for ALL $R$-element subsets of $A(n,k)$, by strong
   induction.
7. **Formal Definition of $A(n,k)$:** `ArrVertex n k` defines vertices as
   injective $k$-sequences from $\{0..n-1\}$ with `Fintype` and `DecidableEq`
   instances. Adjacency (`arr_adjacent`) and external neighbors
   (`external_neighbors`) are computable.

### Remaining interfaces and conditional results

1. **Restricted lower bound** (`RestrictedLowerBound`): This remains an explicit
   per-instance hypothesis. The embedding condition only guarantees that the
   Hamming-ball witness fits; it does not prove the lower bound.
2. **Hamming-ball evaluation** is proved by `CrossTop`; it is a witness
   statement, not a proof that the witness is globally minimizing.

### ✓ Proven by Computer-Assisted Search

1. **Finite audits:** Exhaustive searches and the certificate/oracle bundle
   verify selected small cells; they do not establish a universal theorem or
   uniqueness.
2. **Coefficient/constant checks:** The formulas match the tested witnesses,
   subject to the documented finite ranges.

---

## Architecture: Why Harper's Theorem is Not Used

A key architectural discovery: the capstone theorem's proof chain **does not
reference** Harper's Edge Isoperimetric Theorem (`harpers_edge_isoperimetry`),
even though it is fully proven.

The proof takes a stronger route: instead of "embed hypercube → apply Harper →
transfer to $A(n,k)$", it directly proves the Defect bound
$D(V') \le E_{seq}(|V'|)$ on arrangement graph vertices by strong induction,
using the algebraic subadditivity of $E_{seq}$.

This is advantageous because:

- It works directly on $A(n,k)$ without needing to transfer results from
  hypercubes
- It handles the Triangle Anomaly (cliques like $K_3$ have 3 edges >
  $E_{seq}(3) = 2$) via the Defect invariant
- Harper's theorem remains available in `unstable/ArrangementGraphUtils.lean` as
  a standalone mathematical result

---

## Remaining Theoretical Gaps

### Collision Constant $C(R)$ (Axiomatized)

The constant
$C(R) = (R-1) + \sum_{i=1}^{R-1} \text{bit\_length}(i) - A000788(R)$ bounds
symbol collisions. Proving its minimality requires Kruskal-Katona shadow
operators not yet in Mathlib. See
[collision-axiom-roadmap.md](collision-axiom-roadmap.md). TODO(review): do not
present the shadow-operator reduction as completed until the support-projection
bridge is repaired or removed.

### Uniqueness (not established; blanket version is false)

The framework does not prove uniqueness, and the blanket claim is false for
defect maximization: three vertices on one arrangement line have defect $2=E(3)$
but form a triangle rather than a path. Any future equality theorem must impose
additional hypotheses and classify embedded configurations.

---

## Conditional growth strategy

The squeeze composition is conditional on `RestrictedLowerBound`; it does not
establish a globally optimal growth strategy unconditionally.

Under that supplied hypothesis it provides the corresponding conditional
boundary statement:

- It is not merely a collection of bounds for "perfect" hypercubes ($R=2^d$).
- The formula is tight for the Hamming-ball witness in the embeddable range.
- No unconditional claim excludes a competing clique, path, Star, or other
  topology.

---

## Conclusion

The combination of the Lean 4 formalization and the finite C++
certificate/oracle audits provides evidence for the Hamming-ball construction in
the documented cells. The universal lower-bound interface remains open; the
predictor alone does not verify it.
