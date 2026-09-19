# Project Summary: Arrangement Graph Extraconnectivity

## 1. Abstract

This project studies fixed-volume boundary profiles and extra-connectivity in
arrangement graphs. The Lean development proves an unconditional
Hamming-ball-based boundary sandwich with an explicit volume-only additive
error, plus an exact Hamming-ball witness when its embedding gate holds. It does
not prove that Hamming balls are always optimal or give a general formula for
extra-connectivity. Small exact-profile computations and explicit
counterexamples show that Stars, folded boxes, and hybrids can compete with or
beat the Hamming witness.

## 2. Theory

- **Edge Isoperimetry & Harper's Theorem:** The proved capstone is an
  additive-error sandwich, not an exact Hamming-ball optimality theorem.
  Arrangement-graph counterexamples rule out the blanket zero-error claim.
- **The OEIS A000788 Sequence:** The maximum arrangement defect $E(R)$ exactly
  matches the cumulative popcount sequence (OEIS A000788). It is not a bound on
  internal edge count, because arrangement lines can contain triangles. At
  powers of two ($R=2^d$), it yields the perfect hypercube closed-form defect
  count $E(2^d) = d \cdot 2^{d-1}$.
- **Neighbor-Set Expression:** The embedded Hamming witness has boundary $$ (R
  \cdot k - \text{A000788}(R))(n-k) - C(R), $$ where
  $$
  C(R) = (R-1) + \sum\_{x=1}^{R-1} L(x) - \text{A000788}(R)
  $$
  accounts for internal symbol collisions. This formula subsumes all published
  case-by-case values in the tested embedding range. The Lean lower bound has an
  explicit additive error; this expression is not claimed to equal the
  unrestricted minimum in general.
- **Topological Phase Transition:** A Hamming ball of size $R$ requires exactly
  $\lceil \log_2 R \rceil$ fresh symbols to embed the candidate construction.
  This is an embedding condition, not a proof of optimality: the restricted
  boundary conjecture is refuted by Star examples even when $R\le 2^{n-k}$ for
  $n-k\ge5$. The resulting crossover and its effect on fault tolerance remain
  open.

## 3. Methodology

- **Bridging $\Omega(R^{R-2})$ to $O(\log R)$:** We reduced the problem from a
  super-exponential Cayley tree search space ($\Omega(R^{R-2})$) down to an
  $O(R^4)$ structural predictor, and ultimately to an $O(\log R)$ halving
  recurrence for the leading A000788 coefficient.
- **C++ Algorithmic Micro-Optimizations:** The exhaustive search
  (`arrangement.cpp`) scaled to $\sim 25B$ nodes via:
  - **Hardware-Accelerated SWAR:** Packing symbols into 5-bit nibbles and using
    `__builtin_ctzll` for $O(1)$ vertex diffing.
  - **Nauty Symmetry Pruning:** Applying McKay's canonical labeling on a
    4-colored bipartite graph to prune isomorphic $S_n \times S_R$ branches.
  - **Multi-Tier Deduplication:** Utilizing a fast local hash table for sibling
    nodes, 128-bit hashes for mid-level deduplication, and zero-allocation
    processing for leaf nodes to eliminate memory overhead.
- **Formal Verification (Lean 4):** The stable Lean build verifies the defect
  framework, collision charging bound, explicit Hamming-ball construction, and
  the unconditional additive-error boundary sandwich. It does not prove exact
  profile optimality or an extra-connectivity formula.

## 4. Conclusion

The project combines exact small-instance computation, pattern searches, and a
formal fixed-volume boundary sandwich. The exact unrestricted profile and its
relationship to extra-connectivity remain open beyond the certified finite
cases.
