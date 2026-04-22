# Extraconnectivity of Arrangement Graphs

Computational verification of g-extraconnectivity bounds for arrangement graphs
A(n,k), based on Cheng, Lipták & Tian (2022).

## Background

The **arrangement graph** $A(n,k)$ consists of $k$-permutations of $\{1,\dots,n\}$ joined by edges if they differ in exactly one position. The **$(r-1)$-extraconnectivity** is the minimum vertex cut leaving all remaining components with at least $r$ vertices.

**Proposition 6** (Cheng et al.): As $k, n-k \to \infty$, $r$-extraconnectivity is asymptotically $(r+1)k(n-k)$.

This program enumerates all connected subgraphs of size $R$ to compute exact extraconnectivity formulas.

## Results

Each line reports a neighbor-set formula `(coeff)(n-k) - const` for a connected subgraph of $R$ vertices. The **minimum** (last line for each $R$) gives the $(R-1)$-extraconnectivity, verifying Theorems 1–7.

```text
Searching R=2 (nauty depth limit: 1)  ver[0]=AB  ver[1]=CB
(2nk-1) (n-k)-1, EX: AB CB
  verify(n=4,k=2): |N(V')| = (2·2-1)(4-2)-1 = 3·2 - 1 = 5
  brute-force neighbor count: 5 ✓
Done: 0.002s, 1 generated, 1 evaluated, 0 iso-pruned, 0 exact-pruned, 0 local-pruned
✓ Verified.

Searching R=3 (nauty depth limit: 2)  ver[0]=ABC  ver[1]=DBC
  1/6 branches | 1 evaluated | 0.00284127s
  2/6 branches | 2 evaluated | 0.00286678s
  3/6 branches | 3 evaluated | 0.00287872s
  4/6 branches | 4 evaluated | 0.00289065s
  5/6 branches | 5 evaluated | 0.00290198s
  6/6 branches | 6 evaluated | 0.00291571s
(3nk-2) (n-k)-3, EX: ABC DBC AEC
  verify(n=6,k=3): |N(V')| = (3·3-2)(6-3)-3 = 7·3 - 3 = 18
  brute-force neighbor count: 18 ✓
Done: 0.003s, 6 generated, 6 evaluated, 0 iso-pruned, 0 exact-pruned, 0 local-pruned
✓ Verified.

Searching R=4 (nauty depth limit: 3)  ver[0]=ABCD  ver[1]=EBCD
  1/6 branches | 12 evaluated | 0.0038284s
  2/6 branches | 22 evaluated | 0.00388459s
  3/6 branches | 40 evaluated | 0.00391383s
  4/6 branches | 40 evaluated | 0.00393623s
  5/6 branches | 40 evaluated | 0.00396536s
  6/6 branches | 40 evaluated | 0.00398838s
(4nk-3) (n-k)-6, EX: ABCD EBCD AFCD ABGD
  verify(n=8,k=4): |N(V')| = (4·4-3)(8-4)-6 = 13·4 - 6 = 46
  brute-force neighbor count: 46 ✓
(4nk-4) (n-k)-4, EX: ABCD EBCD AFCD EFCD
  verify(n=8,k=4): |N(V')| = (4·4-4)(8-4)-4 = 12·4 - 4 = 44
  brute-force neighbor count: 44 ✓
Done: 0.004s, 46 generated, 40 evaluated, 3 iso-pruned, 0 exact-pruned, 9 local-pruned
✓ Verified.

Searching R=5 (nauty depth limit: 4)  ver[0]=ABCDE  ver[1]=FBCDE
  1/6 branches | 176 evaluated | 0.00455884s
  2/6 branches | 224 evaluated | 0.0048273s
  3/6 branches | 402 evaluated | 0.00526092s
  4/6 branches | 402 evaluated | 0.00529409s
  5/6 branches | 402 evaluated | 0.00534502s
  6/6 branches | 402 evaluated | 0.00537986s
(5nk-4) (n-k)-10, EX: ABCDE FBCDE AGCDE ABHDE ABCIE
  verify(n=10,k=5): |N(V')| = (5·5-4)(10-5)-10 = 21·5 - 10 = 95
  brute-force neighbor count: 95 ✓
(5nk-5) (n-k)- 7, EX: ABCDE FBCDE AGCDE ABHDE FGCDE
  verify(n=10,k=5): |N(V')| = (5·5-5)(10-5)-7 = 20·5 - 7 = 93
  brute-force neighbor count: 93 ✓
Done: 0.005s, 448 generated, 402 evaluated, 29 iso-pruned, 0 exact-pruned, 107 local-pruned
✓ Verified.

Searching R=6 (nauty depth limit: 5)  ver[0]=ABCDEF  ver[1]=GBCDEF
  1/6 branches | 3602 evaluated | 0.0116042s
  2/6 branches | 4181 evaluated | 0.0147043s
  3/6 branches | 5436 evaluated | 0.0219102s
  4/6 branches | 5436 evaluated | 0.021977s
  5/6 branches | 5436 evaluated | 0.0220605s
  6/6 branches | 5436 evaluated | 0.0221209s
(6nk-5) (n-k)-15, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF ABCDKF
  verify(n=12,k=6): |N(V')| = (6·6-5)(12-6)-15 = 31·6 - 15 = 171
  brute-force neighbor count: 171 ✓
(6nk-6) (n-k)-11, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF GHCDEF
  verify(n=12,k=6): |N(V')| = (6·6-6)(12-6)-11 = 30·6 - 11 = 169
  brute-force neighbor count: 169 ✓
(6nk-7) (n-k)- 9, EX: ABCDEF GBCDEF AHCDEF ABIDEF GHCDEF GBIDEF
  verify(n=12,k=6): |N(V')| = (6·6-7)(12-6)-9 = 29·6 - 9 = 165
  brute-force neighbor count: 165 ✓
Done: 0.022s, 5884 generated, 5436 evaluated, 324 iso-pruned, 0 exact-pruned, 1393 local-pruned
✓ Verified.

Searching R=7 (nauty depth limit: 6)  ver[0]=ABCDEFG  ver[1]=HBCDEFG
  1/6 branches | 90621 evaluated | 0.20612s
  2/6 branches | 99245 evaluated | 0.25371s
  3/6 branches | 120172 evaluated | 0.334895s
  4/6 branches | 120172 evaluated | 0.335001s
  5/6 branches | 120172 evaluated | 0.335125s
  6/6 branches | 120172 evaluated | 0.335214s
(7nk-6) (n-k)-21, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG ABCDEMG
  verify(n=14,k=7): |N(V')| = (7·7-6)(14-7)-21 = 43·7 - 21 = 280
  brute-force neighbor count: 280 ✓
(7nk-7) (n-k)-16, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG HICDEFG
  verify(n=14,k=7): |N(V')| = (7·7-7)(14-7)-16 = 42·7 - 16 = 278
  brute-force neighbor count: 278 ✓
(7nk-8) (n-k)-13, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG HICDEFG HBJDEFG
  verify(n=14,k=7): |N(V')| = (7·7-8)(14-7)-13 = 41·7 - 13 = 274
  brute-force neighbor count: 274 ✓
(7nk-9) (n-k)-11, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG HICDEFG HBJDEFG AIJDEFG
  verify(n=14,k=7): |N(V')| = (7·7-9)(14-7)-11 = 40·7 - 11 = 269
  brute-force neighbor count: 269 ✓
Done: 0.335s, 126056 generated, 120172 evaluated, 4265 iso-pruned, 0 exact-pruned, 28373 local-pruned
✓ Verified.

Searching R=8 (nauty depth limit: 7)  ver[0]=ABCDEFGH  ver[1]=IBCDEFGH
  1/6 branches | 3132048 evaluated | 7.6s
  2/6 branches | 3355354 evaluated | 8.7s
  3/6 branches | 3827325 evaluated | 10.5s
  4/6 branches | 3827325 evaluated | 10.5s
  5/6 branches | 3827325 evaluated | 10.5s
  6/6 branches | 3827325 evaluated | 10.5s
(8nk- 7) (n-k)-28, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH ABCDENGH ABCDEFOH
  verify(n=16,k=8): |N(V')| = (8·8-7)(16-8)-28 = 57·8 - 28 = 428
  brute-force neighbor count: 428 ✓
(8nk- 8) (n-k)-22, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH ABCDENGH IJCDEFGH
  verify(n=16,k=8): |N(V')| = (8·8-8)(16-8)-22 = 56·8 - 22 = 426
  brute-force neighbor count: 426 ✓
(8nk- 9) (n-k)-18, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH IJCDEFGH IBKDEFGH
  verify(n=16,k=8): |N(V')| = (8·8-9)(16-8)-18 = 55·8 - 18 = 422
  brute-force neighbor count: 422 ✓
(8nk-10) (n-k)-16, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH IJCDEFGH IBKDEFGH IBCLEFGH
  verify(n=16,k=8): |N(V')| = (8·8-10)(16-8)-16 = 54·8 - 16 = 416
  brute-force neighbor count: 416 ✓
(8nk-12) (n-k)-12, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH IJCDEFGH IBKDEFGH AJKDEFGH IJKDEFGH
  verify(n=16,k=8): |N(V')| = (8·8-12)(16-8)-12 = 52·8 - 12 = 404
  brute-force neighbor count: 404 ✓
Done: 10.475s, 3953381 generated, 3827325 evaluated, 91824 iso-pruned, 0 exact-pruned, 838218 local-pruned
✓ Verified.

Searching R=9 (nauty depth limit: 8)  ver[0]=ABCDEFGHI  ver[1]=JBCDEFGHI
  1/6 branches | 137988829 evaluated | 396.1s
  2/6 branches | 146106345 evaluated | 431.0s
  3/6 branches | 162163337 evaluated | 490.9s
  4/6 branches | 162163337 evaluated | 490.9s
  5/6 branches | 162163337 evaluated | 490.9s
  6/6 branches | 162163337 evaluated | 490.9s
(9nk- 8) (n-k)-36, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI ABCDEFPHI ABCDEFGQI
  verify(n=18,k=9): |N(V')| = (9·9-8)(18-9)-36 = 73·9 - 36 = 621
  brute-force neighbor count: 621 ✓
(9nk- 9) (n-k)-29, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI ABCDEFPHI JKCDEFGHI
  verify(n=18,k=9): |N(V')| = (9·9-9)(18-9)-29 = 72·9 - 29 = 619
  brute-force neighbor count: 619 ✓
(9nk-10) (n-k)-24, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI JKCDEFGHI JBLDEFGHI
  verify(n=18,k=9): |N(V')| = (9·9-10)(18-9)-24 = 71·9 - 24 = 615
  brute-force neighbor count: 615 ✓
(9nk-11) (n-k)-21, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI JKCDEFGHI JBLDEFGHI JBCMEFGHI
  verify(n=18,k=9): |N(V')| = (9·9-11)(18-9)-21 = 70·9 - 21 = 609
  brute-force neighbor count: 609 ✓
(9nk-12) (n-k)-18, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI JKCDEFGHI JBLDEFGHI JBCMEFGHI AKLDEFGHI
  verify(n=18,k=9): |N(V')| = (9·9-12)(18-9)-18 = 69·9 - 18 = 603
  brute-force neighbor count: 603 ✓
(9nk-13) (n-k)-16, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI JKCDEFGHI JBLDEFGHI AKLDEFGHI JKLDEFGHI
  verify(n=18,k=9): |N(V')| = (9·9-13)(18-9)-16 = 68·9 - 16 = 596
  brute-force neighbor count: 596 ✓
Done: 490.892s, 166116718 generated, 162163337 evaluated, 2921832 iso-pruned, 0 exact-pruned, 33174381 local-pruned
✓ Verified.
```

\newpage

### Complexity

The exhaustive search evaluates all connected R-subgraphs with nauty-based dedup.
The search space is Ω(R^(R−2)) by Cayley's formula; observed growth is ×22 at R=7, ×32 at R=8, ×42 at R=9.

| Method                     | Complexity | What it computes                     |
| -------------------------- | ---------- | ------------------------------------ |
| Exhaustive search          | Ω(R^(R−2)) | All topological classes              |
| Hamming ball predictor     | O(R⁴)      | Full formula: coefficient + constant |
| A000788 halving recurrence | O(log R)   | Coefficient only (nk1 = E(R))        |

### Summary table

| R   | (R-1)-extraconnectivity | Paper reference         | \|N(V')\| at n=2R, k=R     | Runtime |
| --- | ----------------------- | ----------------------- | -------------------------- | ------- |
| 2   | (2k−1)(n−k) − 1         | Theorem 1               | (3)(2) − 1 = 5             | 0.002s  |
| 3   | (3k−2)(n−k) − 3         | Theorem 2               | (7)(3) − 3 = 18            | 0.003s  |
| 4   | (4k−4)(n−k) − 4         | Theorem 3               | (12)(4) − 4 = 44           | 0.004s  |
| 5   | (5k−5)(n−k) − 7         | Theorem 5               | (20)(5) − 7 = 93           | 0.005s  |
| 6   | (6k−7)(n−k) − 9         | Theorem 6               | (29)(6) − 9 = 165          | 0.02s   |
| 7   | (7k−9)(n−k) − 11        | Theorem 7               | (40)(7) − 11 = 269         | 0.3s    |
| 8   | **(8k−12)(n−k) − 12**   | **New (this work)**     | **(52)(8) − 12 = 404**     | 10.5s   |
| 9   | **(9k−13)(n−k) − 16**   | **New (this work)**     | **(68)(9) − 16 = 596**     | 8.2 min |
| 10  | **(10k−15)(n−k) − 19**  | **Predicted (A000788)** | **(85)(10) − 19 = 831**    | ~7h     |
| 11  | **(11k−18)(n−k) − 22**  | **Predicted**           | **(103)(11) − 22 = 1111**  | ~20d    |
| 12  | **(12k−20)(n−k) − 24**  | **Predicted**           | **(124)(12) − 24 = 1464**  | ~4y     |
| 13  | **(13k−22)(n−k) − 27**  | **Predicted**           | **(147)(13) − 27 = 1884**  | ~300y   |
| 14  | **(14k−25)(n−k) − 29**  | **Predicted**           | **(171)(14) − 29 = 2365**  | ~30Ky   |
| 15  | **(15k−28)(n−k) − 31**  | **Predicted**           | **(197)(15) − 31 = 2924**  | ~3My    |
| 16  | **(16k−32)(n−k) − 32**  | **Predicted (4-cube)**  | **(224)(16) − 32 = 3552**  | infeas. |
| 24  | **(24k−52)(n−k) − 60**  | **Predicted**           | **(524)(24) − 60 = 12516** | infeas. |
| 32  | **(32k−80)(n−k) − 80**  | **Predicted (5-cube)**  | **(944)(32) − 80 = 30128** | infeas. |

† R≥10 predicted via Hamming ball construction (`predict.cpp`), brute-force verified for R≤20.

Predictions for R=2..1024 are available in [`docs/predictions.csv`](docs/predictions.csv) (`make csv`):

| Column          | Meaning                                              |
| --------------- | ---------------------------------------------------- |
| `R`             | Number of vertices in the subgraph                   |
| `nk1`           | Internal edges = A000788(R) (cumulative popcount)    |
| `constant`      | Formula constant C(R) = (R−1) + Σ zero-bits(1..R−1)  |
| `coeff`         | R·k − nk1 (the leading coefficient at k=R)           |
| `formula_at_2R` | \|N(V')\| evaluated at n=2R, k=R: coeff·R − constant |

### Internal edge divergence: OEIS A000788 vs. linear prediction

The paper extrapolated E(R) = 2R−5 internal edges from R=5,6,7. The true
sequence is [OEIS A000788](https://oeis.org/A000788) (cumulative binary weight),
which coincides at R=5,6,7 but diverges at powers of 2:

| R   | A000788 (true E) | Paper's 2R-5 | Winner                        |
| --- | ---------------- | ------------ | ----------------------------- |
| 4   | **4**            | 3            | Hypercube wins                |
| 5   | 5                | 5            | _Tie_                         |
| 6   | 7                | 7            | _Tie_                         |
| 7   | 9                | 9            | _Tie_                         |
| 8   | **12**           | 11           | **Hypercube wins (3-cube)**   |
| 9   | 13               | 13           | _Tie_                         |
| 10  | 15               | 15           | _Tie_                         |
| 12  | 20               | 19           | Hypercube wins                |
| 16  | **32**           | 27           | **Total divergence (4-cube)** |

The closed form E(2^d) = d · 2^{d-1} is proven in `proofs/HypercubeEdges.lean`.

> **Note:** For R=5..7, the minimum follows ((r+1)k−(2r−3))(n−k)−(2r−1).
> **Pattern break at R=8.** For R=5..7, the minimum follows
> ((r+1)k−(2r−3))(n−k)−(2r−1), which assumes tree-like vertex cuts with R−1
> internal edges. At R=8, the vertices lock into a 3-dimensional hypercube
> (12 internal edges vs. the tree-predicted 7), causing the coefficient to
> drop from (8k−7) to **(8k−12)**. The internal edge count E(R) matches
> [OEIS A000788](https://oeis.org/A000788) — the cumulative binary weight —
> which predicts R=10 will give **(10k−15)** without running the search.
>
> Each formula is independently verified by brute-force neighbor enumeration
> in A(2R, R). See [docs/hypercube-isoperimetry.md](docs/hypercube-isoperimetry.md)
> for the full mathematical analysis.

## Reference

E. Cheng, L. Lipták, D. Tian. "On the Extraconnectivity of Arrangement Graphs."
_Springer Proceedings in Mathematics & Statistics_ 388, pp. 275–282, 2022.

---

_Computational verification by Shane Jaroch._
