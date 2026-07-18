# Project Summary: Arrangement Graph Extraconnectivity

## 1. Abstract

This project generalizes and extends the 2022 case-by-case analysis of Cheng, Lipták & Tian for arrangement graph $(R-1)$-extraconnectivity. Their published formulas for $R=2..7$ are **correct** — and we discovered that their ad-hoc constructions are actually **Hamming balls** embedded in hypercubes. By scaling exhaustive search to $R=10$ and connecting the problem to Harper-style edge isoperimetry, we derive a closed-form candidate formula based on the **OEIS A000788** sequence (cumulative popcount) that subsumes all published cases and extends to arbitrary $R$, conditional on the remaining extremal-combinatorics hypotheses. We introduce an $O(R^4)$ Hamming ball predictor that replaces the super-exponential exhaustive search for the computational side of the project.

## 2. Theory

- **Edge Isoperimetry & Harper's Theorem:** The project is organized around a Harper-style comparison with Hamming-ball topologies, but the stable Lean capstone does not currently derive the final arrangement-graph bound directly from Harper. The Hamming-ball optimality story is computationally strong and partially formalized, with the remaining universal/collision bounds isolated as explicit hypotheses.
- **The OEIS A000788 Sequence:** The maximum number of internal edges $E(R)$ exactly matches the cumulative popcount sequence (OEIS A000788). At powers of two ($R=2^d$), this yields the perfect hypercube closed-form edge count $E(2^d) = d \cdot 2^{d-1}$.
- **Neighbor-Set Closed-Form Expression:** The predicted extraconnectivity function $\kappa_R$ is
  $$ \kappa*R = (R \cdot k - \text{A000788}(R))(n-k) - C(R) $$
  where $C(R) = (R-1) + \sum*{x=1}^{R-1} L(x) - \text{A000788}(R)$ accounts for internal symbol collisions. This formula subsumes all published case-by-case results for $R=2..7$ and is computationally supported beyond that range; in the formal development, the exact universal lower bound and Hamming-ball collision evaluation remain explicit hypothesis interfaces.
- **Topological Phase Transition:** A Hamming ball of size $R$ requires exactly $\lceil \log_2 R \rceil$ fresh symbols to expand without permutation collisions. Thus, the hypercube optimally embeds into $A(n,k)$ if and only if $n - k \ge \lceil \log_2 R \rceil$. Remarkably, a smaller alphabet ($n-k < \lceil \log_2 R \rceil$) violates this embedding constraint, forbidding the hypercube cut. This forces the graph into a strictly sub-optimal topological structure, which unexpectedly _increases_ the minimum cut size and therefore enhances the fault tolerance of the network.

## 3. Methodology

- **Bridging $\Omega(R^{R-2})$ to $O(\log R)$:** We reduced the problem from a super-exponential Cayley tree search space ($\Omega(R^{R-2})$) down to an $O(R^4)$ structural predictor, and ultimately to an $O(\log R)$ halving recurrence for the leading A000788 coefficient.
- **C++ Algorithmic Micro-Optimizations:** The exhaustive search (`arrangement.cpp`) scaled to $\sim 25B$ nodes via:
  - **Hardware-Accelerated SWAR:** Packing symbols into 5-bit nibbles and using `__builtin_ctzll` for $O(1)$ vertex diffing.
  - **Nauty Symmetry Pruning:** Applying McKay's canonical labeling on a 4-colored bipartite graph to prune isomorphic $S_n \times S_R$ branches.
  - **Multi-Tier Deduplication:** Utilizing a fast local hash table for sibling nodes, 128-bit hashes for mid-level deduplication, and zero-allocation processing for leaf nodes to eliminate memory overhead.
- **Formal Verification (Lean 4):** The stable Lean build machine-verifies the algebraic defect framework, core arithmetic, and explicit Hamming-ball construction. The final extremal combinatorics step is not yet discharged unconditionally; it remains isolated behind `UniversalLowerBound` and `HBCrossCollisions`.

## 4. Conclusion

The project bridges empirical computational search and theoretical graph structure. By identifying that Cheng et al.'s case-by-case constructions are Hamming balls and by formalizing the algebraic defect machinery around them, it provides a unified computational framework and a substantial Lean 4 proof core. The predictor is end-to-end usable today; the final extremal-combinatorics claims remain conditional in the formal writeup.
