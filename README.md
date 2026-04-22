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
Searching R=2 (nauty limit: 3)
  ver[0]=AB  ver[1]=CB
(2nk-1) (n-k)-1, EX: AB CB
Done: 0.000s | Gen: 1 | Eval: 1
Pruned | Iso: 0 | Exact: 0 | Local: 0
  [brute-force] |N(V')| = 5 ✓
  formula(n=4,k=2): |N(V')| = 3·2 - 1 = 5

Searching R=3 (nauty limit: 3)
  ver[0]=ABC  ver[1]=DBC
(3nk-2) (n-k)-3, EX: ABC DBC AEC
Done: 0.000s | Gen: 6 | Eval: 5
Pruned | Iso: 0 | Exact: 0 | Local: 1
  [brute-force] |N(V')| = 18 ✓
  formula(n=6,k=3): |N(V')| = 7·3 - 3 = 18

Searching R=4 (nauty limit: 3)
  ver[0]=ABCD  ver[1]=EBCD
(4nk-3) (n-k)-6, EX: ABCD EBCD AFCD ABGD
(4nk-4) (n-k)-4, EX: ABCD EBCD AFCD EFCD
Done: 0.000s | Gen: 46 | Eval: 40
Pruned | Iso: 2 | Exact: 0 | Local: 10
  [brute-force] |N(V')| = 44 ✓
  formula(n=8,k=4): |N(V')| = 12·4 - 4 = 44

Searching R=5 (nauty limit: 3)
  ver[0]=ABCDE  ver[1]=FBCDE
(5nk-4) (n-k)-10, EX: ABCDE FBCDE AGCDE ABHDE ABCIE
(5nk-5) (n-k)- 7, EX: ABCDE FBCDE AGCDE ABHDE FGCDE
Done: 0.000s | Gen: 1102 | Eval: 1056
Pruned | Iso: 2 | Exact: 3 | Local: 268
  [brute-force] |N(V')| = 93 ✓
  formula(n=10,k=5): |N(V')| = 20·5 - 7 = 93

Searching R=6 (nauty limit: 4)
  ver[0]=ABCDEF  ver[1]=GBCDEF
(6nk-5) (n-k)-15, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF ABCDKF
(6nk-6) (n-k)-11, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF GHCDEF
(6nk-7) (n-k)- 9, EX: ABCDEF GBCDEF AHCDEF ABIDEF GHCDEF GBIDEF
Done: 0.004s | Gen: 19507 | Eval: 19059
Pruned | Iso: 28 | Exact: 36 | Local: 4676
  [brute-force] |N(V')| = 165 ✓
  formula(n=12,k=6): |N(V')| = 29·6 - 9 = 165

Searching R=7 (nauty limit: 5)
  ver[0]=ABCDEFG  ver[1]=HBCDEFG
(7nk-6) (n-k)-21, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG ABCDEMG
(7nk-7) (n-k)-16, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG HICDEFG
(7nk-8) (n-k)-13, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG HICDEFG HBJDEFG
(7nk-9) (n-k)-11, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG HICDEFG HBJDEFG AIJDEFG
Done: 0.067s | Gen: 394905 | Eval: 389021
Pruned | Iso: 323 | Exact: 729 | Local: 88799
  [brute-force] |N(V')| = 269 ✓
  formula(n=14,k=7): |N(V')| = 40·7 - 11 = 269

Searching R=8 (nauty limit: 6)
  ver[0]=ABCDEFGH  ver[1]=IBCDEFGH
(8nk- 7) (n-k)-28, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH ABCDENGH ABCDEFOH
(8nk- 8) (n-k)-22, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH ABCDENGH IJCDEFGH
(8nk- 9) (n-k)-18, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH IJCDEFGH IBKDEFGH
(8nk-10) (n-k)-16, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH IJCDEFGH IBKDEFGH IBCLEFGH
(8nk-12) (n-k)-12, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH IJCDEFGH IBKDEFGH AJKDEFGH IJKDEFGH
Done: 1.764s | Gen: 12053012 | Eval: 11926956
Pruned | Iso: 4264 | Exact: 21554 | Local: 2544542
  [brute-force] |N(V')| = 404 ✓
  formula(n=16,k=8): |N(V')| = 52·8 - 12 = 404

Searching R=9 (nauty limit: 7)
  ver[0]=ABCDEFGHI  ver[1]=JBCDEFGHI
(9nk- 8) (n-k)-36, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI ABCDEFPHI ABCDEFGQI
(9nk- 9) (n-k)-29, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI ABCDEFPHI JKCDEFGHI
(9nk-10) (n-k)-24, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI JKCDEFGHI JBLDEFGHI
(9nk-11) (n-k)-21, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI JKCDEFGHI JBLDEFGHI JBCMEFGHI
(9nk-12) (n-k)-18, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI JKCDEFGHI JBLDEFGHI JBCMEFGHI AKLDEFGHI
(9nk-13) (n-k)-16, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI JKCDEFGHI JBLDEFGHI AKLDEFGHI JKLDEFGHI
Done: 71.470s | Gen: 502684606 | Eval: 498731225
Pruned | Iso: 91823 | Exact: 846078 | Local: 99682702
  [brute-force] |N(V')| = 596 ✓
  formula(n=18,k=9): |N(V')| = 68·9 - 16 = 596
```

\newpage

### Complexity

The exhaustive search evaluates all connected R-subgraphs with nauty-based dedup.
The search space is Ω(R^(R−2)) by Cayley's formula; observed growth is ×22 at R=7, ×32 at R=8, ×42 at R=9.

| Method                     | Complexity | What it computes                     |
| -------------------------- | ---------- | ------------------------------------ |
| Exhaustive search          | Ω(R^(R−2)) | All topological classes              |
| Hamming ball predictor     | O(R^4)     | Full formula: coefficient + constant |
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

† R>=10 predicted via Hamming ball construction (`predict.cpp`), brute-force verified for R<=20.

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
