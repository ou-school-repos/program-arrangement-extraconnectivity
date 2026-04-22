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
  1/6 branches | 1 evaluated | 0.00287525s
  2/6 branches | 2 evaluated | 0.00290457s
  3/6 branches | 3 evaluated | 0.00291655s
  4/6 branches | 4 evaluated | 0.00293408s
  5/6 branches | 5 evaluated | 0.00294712s
  6/6 branches | 6 evaluated | 0.00296168s
(3nk-2) (n-k)-3, EX: ABC DBC AEC
  verify(n=6,k=3): |N(V')| = (3·3-2)(6-3)-3 = 7·3 - 3 = 18
  brute-force neighbor count: 18 ✓
Done: 0.003s, 6 generated, 6 evaluated, 0 iso-pruned, 0 exact-pruned, 0 local-pruned
✓ Verified.

Searching R=4 (nauty depth limit: 3)  ver[0]=ABCD  ver[1]=EBCD
  1/6 branches | 12 evaluated | 0.00384929s
  2/6 branches | 22 evaluated | 0.00390469s
  3/6 branches | 40 evaluated | 0.00393358s
  4/6 branches | 40 evaluated | 0.00395854s
  5/6 branches | 40 evaluated | 0.00398756s
  6/6 branches | 40 evaluated | 0.00401038s
(4nk-3) (n-k)-6, EX: ABCD EBCD AFCD ABGD
  verify(n=8,k=4): |N(V')| = (4·4-3)(8-4)-6 = 13·4 - 6 = 46
  brute-force neighbor count: 46 ✓
(4nk-4) (n-k)-4, EX: ABCD EBCD AFCD EFCD
  verify(n=8,k=4): |N(V')| = (4·4-4)(8-4)-4 = 12·4 - 4 = 44
  brute-force neighbor count: 44 ✓
Done: 0.004s, 46 generated, 40 evaluated, 3 iso-pruned, 0 exact-pruned, 9 local-pruned
✓ Verified.

Searching R=5 (nauty depth limit: 4)  ver[0]=ABCDE  ver[1]=FBCDE
  1/6 branches | 176 evaluated | 0.00474304s
  2/6 branches | 224 evaluated | 0.0050081s
  3/6 branches | 402 evaluated | 0.00543929s
  4/6 branches | 402 evaluated | 0.00547289s
  5/6 branches | 402 evaluated | 0.00552289s
  6/6 branches | 402 evaluated | 0.00555784s
(5nk-4) (n-k)-10, EX: ABCDE FBCDE AGCDE ABHDE ABCIE
  verify(n=10,k=5): |N(V')| = (5·5-4)(10-5)-10 = 21·5 - 10 = 95
  brute-force neighbor count: 95 ✓
(5nk-5) (n-k)- 7, EX: ABCDE FBCDE AGCDE ABHDE FGCDE
  verify(n=10,k=5): |N(V')| = (5·5-5)(10-5)-7 = 20·5 - 7 = 93
  brute-force neighbor count: 93 ✓
Done: 0.006s, 448 generated, 402 evaluated, 29 iso-pruned, 0 exact-pruned, 107 local-pruned
✓ Verified.

Searching R=6 (nauty depth limit: 5)  ver[0]=ABCDEF  ver[1]=GBCDEF
  1/6 branches | 3602 evaluated | 0.0116404s
  2/6 branches | 4181 evaluated | 0.0147582s
  3/6 branches | 5436 evaluated | 0.0220111s
  4/6 branches | 5436 evaluated | 0.0220747s
  5/6 branches | 5436 evaluated | 0.022162s
  6/6 branches | 5436 evaluated | 0.0222252s
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
  1/6 branches | 90621 evaluated | 0.210585s
  2/6 branches | 99245 evaluated | 0.258777s
  3/6 branches | 120172 evaluated | 0.34141s
  4/6 branches | 120172 evaluated | 0.341519s
  5/6 branches | 120172 evaluated | 0.341646s
  6/6 branches | 120172 evaluated | 0.341735s
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
Done: 0.342s, 126056 generated, 120172 evaluated, 4265 iso-pruned, 0 exact-pruned, 28373 local-pruned
✓ Verified.

Searching R=8 (nauty depth limit: 7)  ver[0]=ABCDEFGH  ver[1]=IBCDEFGH
  1/6 branches | 3132048 evaluated | 7.6s
  2/6 branches | 3355354 evaluated | 8.6s
  3/6 branches | 3827325 evaluated | 10.4s
  4/6 branches | 3827325 evaluated | 10.4s
  5/6 branches | 3827325 evaluated | 10.4s
  6/6 branches | 3827325 evaluated | 10.4s
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
Done: 10.392s, 3953381 generated, 3827325 evaluated, 91824 iso-pruned, 0 exact-pruned, 838218 local-pruned
✓ Verified.

Searching R=9 (nauty depth limit: 8)  ver[0]=ABCDEFGHI  ver[1]=JBCDEFGHI
  1/6 branches | 165314750 evaluated | 538.6s
  2/6 branches | 175213668 evaluated | 586.4s
  3/6 branches | 194499500 evaluated | 667.2s
  4/6 branches | 194499500 evaluated | 667.2s
  5/6 branches | 194499500 evaluated | 667.2s
  6/6 branches | 194499500 evaluated | 667.2s
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
Done: 667.225s, 199291099 generated, 194499500 evaluated, 3760050 iso-pruned, 0 exact-pruned
✓ Verified.

```

\newpage

### Summary table

| R   | (R-1)-extraconnectivity | Paper reference     | \|N(V')\| at n=2R, k=R       |
| --- | ----------------------- | ------------------- | ---------------------------- |
| 2   | (2k−1)(n−k) − 1         | Theorem 1           | (3)(2) − 1 = 5               |
| 3   | (3k−2)(n−k) − 3         | Theorem 2           | (7)(3) − 3 = 18              |
| 4   | (4k−4)(n−k) − 4         | Theorem 3           | (12)(4) − 4 = 44             |
| 5   | (5k−5)(n−k) − 7         | Theorem 5           | (20)(5) − 7 = 93             |
| 6   | (6k−7)(n−k) − 9         | Theorem 6           | (29)(6) − 9 = 165            |
| 7   | (7k−9)(n−k) − 11        | Theorem 7           | (40)(7) − 11 = 269           |
| 8   | **(8k−12)(n−k) − 12**   | **New (this work)** | **(52)(8) − 12 = 404**       |
| 9   | **(9k−13)(n−k) − 16**   | **New (this work)** | **(68)(9) − 16 = 596**       |

> **Note:** For R=5..7, the minimum follows ((r+1)k−(2r−3))(n−k)−(2r−1).
> R=8 and R=9 **break this pattern** — the (n-k) coefficient jumps by more than 2,
> and the intermediate classes are absent.
>
> Each formula is independently verified by brute-force neighbor enumeration
> in A(2R, R). The full output above shows the programmatic check for each
> result class.

## Reference

E. Cheng, L. Lipták, D. Tian. "On the Extraconnectivity of Arrangement Graphs."
_Springer Proceedings in Mathematics & Statistics_ 388, pp. 275–282, 2022.

---
*Computational verification by Shane Jaroch.*
