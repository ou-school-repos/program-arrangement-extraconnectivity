# Extraconnectivity of Arrangement Graphs

Computational verification of g-extraconnectivity bounds for arrangement graphs
A(n,k), based on Cheng, Lipták & Tian (2022).

## Background

The **arrangement graph** A(n,k) has vertices corresponding to k-permutations of
{1,...,n}, with edges between permutations differing in exactly one position.
The **(r-1)-extraconnectivity** is the minimum vertex cut such that every remaining
component has at least r vertices.

**Proposition 6** (Cheng et al.): As k and n-k → ∞, the r-extraconnectivity of
A(n,k) is asymptotically **(r+1)·k·(n-k)**.

This program enumerates all connected subgraphs of R vertices and computes their
neighbor set sizes, establishing exact extraconnectivity formulas for each R.

## Results

Each line reports a distinct neighbor-set formula `(coefficient)(n-k) - constant`
for a connected subgraph of R vertices. The **minimum** line (last for each R) gives
the (R-1)-extraconnectivity, matching the paper's Theorems 1–7.

```text
Searching R=2 (nauty depth limit: 1)  ver[0]=AB  ver[1]=CB
(2nk-1) (n-k)-1, EX: AB CB
Done: 0.002s, 1 generated, 1 evaluated, 0 iso-pruned, 0 exact-pruned
✓ Verified.

Searching R=3 (nauty depth limit: 2)  ver[0]=ABC  ver[1]=DBC
  1/6 branches | 1 evaluated | 0.00298997s
  2/6 branches | 2 evaluated | 0.0030235s
  3/6 branches | 3 evaluated | 0.00303572s
  4/6 branches | 4 evaluated | 0.00304719s
  5/6 branches | 5 evaluated | 0.00305851s
  6/6 branches | 6 evaluated | 0.00307241s
(3nk-2) (n-k)-3, EX: ABC DBC AEC
Done: 0.003s, 6 generated, 6 evaluated, 0 iso-pruned, 0 exact-pruned
✓ Verified.

Searching R=4 (nauty depth limit: 3)  ver[0]=ABCD  ver[1]=EBCD
  1/6 branches | 14 evaluated | 0.00381072s
  2/6 branches | 26 evaluated | 0.00388743s
  3/6 branches | 49 evaluated | 0.00391881s
  4/6 branches | 49 evaluated | 0.00394388s
  5/6 branches | 49 evaluated | 0.00397348s
  6/6 branches | 49 evaluated | 0.00400043s
(4nk-3) (n-k)-6, EX: ABCD EBCD AFCD ABGD
(4nk-4) (n-k)-4, EX: ABCD EBCD AFCD EFCD
Done: 0.004s, 55 generated, 49 evaluated, 3 iso-pruned, 0 exact-pruned
✓ Verified.

Searching R=5 (nauty depth limit: 4)  ver[0]=ABCDE  ver[1]=FBCDE
  1/6 branches | 212 evaluated | 0.0047076s
  2/6 branches | 272 evaluated | 0.00506034s
  3/6 branches | 500 evaluated | 0.00562892s
  4/6 branches | 500 evaluated | 0.00566241s
  5/6 branches | 500 evaluated | 0.00571256s
  6/6 branches | 500 evaluated | 0.00574736s
(5nk-4) (n-k)-10, EX: ABCDE FBCDE AGCDE ABHDE ABCIE
(5nk-5) (n-k)- 7, EX: ABCDE FBCDE AGCDE ABHDE FGCDE
Done: 0.006s, 555 generated, 500 evaluated, 38 iso-pruned, 0 exact-pruned
✓ Verified.

Searching R=6 (nauty depth limit: 5)  ver[0]=ABCDEF  ver[1]=GBCDEF
  1/6 branches | 4409 evaluated | 0.0137263s
  2/6 branches | 5151 evaluated | 0.0181101s
  3/6 branches | 6722 evaluated | 0.0283662s
  4/6 branches | 6722 evaluated | 0.0284346s
  5/6 branches | 6722 evaluated | 0.0285179s
  6/6 branches | 6722 evaluated | 0.0285823s
(6nk-5) (n-k)-15, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF ABCDKF
(6nk-6) (n-k)-11, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF GHCDEF
(6nk-7) (n-k)- 9, EX: ABCDEF GBCDEF AHCDEF ABIDEF GHCDEF GBIDEF
Done: 0.029s, 7277 generated, 6722 evaluated, 431 iso-pruned, 0 exact-pruned
✓ Verified.

Searching R=7 (nauty depth limit: 6)  ver[0]=ABCDEFG  ver[1]=HBCDEFG
  1/6 branches | 110456 evaluated | 0.279709s
  2/6 branches | 121324 evaluated | 0.346624s
  3/6 branches | 147152 evaluated | 0.454643s
  4/6 branches | 147152 evaluated | 0.454745s
  5/6 branches | 147152 evaluated | 0.45487s
  6/6 branches | 147152 evaluated | 0.454958s
(7nk-6) (n-k)-21, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG ABCDEMG
(7nk-7) (n-k)-16, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG HICDEFG
(7nk-8) (n-k)-13, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG HICDEFG HBJDEFG
(7nk-9) (n-k)-11, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG HICDEFG HBJDEFG AIJDEFG
Done: 0.455s, 154429 generated, 147152 evaluated, 5658 iso-pruned, 0 exact-pruned
✓ Verified.

Searching R=8 (nauty depth limit: 7)  ver[0]=ABCDEFGH  ver[1]=IBCDEFGH
  1/6 branches | 3786894 evaluated | 10.4s
  2/6 branches | 4063313 evaluated | 11.8s
  3/6 branches | 4637170 evaluated | 14.2s
  4/6 branches | 4637170 evaluated | 14.2s
  5/6 branches | 4637170 evaluated | 14.2s
  6/6 branches | 4637170 evaluated | 14.2s
(8nk- 7) (n-k)-28, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH ABCDENGH ABCDEFOH
(8nk- 8) (n-k)-22, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH ABCDENGH IJCDEFGH
(8nk- 9) (n-k)-18, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH ABCDMFGH IJCDEFGH IBKDEFGH
(8nk-10) (n-k)-16, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH ABCLEFGH IJCDEFGH IBKDEFGH IBCLEFGH
(8nk-12) (n-k)-12, EX: ABCDEFGH IBCDEFGH AJCDEFGH ABKDEFGH IJCDEFGH IBKDEFGH AJKDEFGH IJKDEFGH
Done: 14.180s, 4791599 generated, 4637170 evaluated, 120197 iso-pruned, 0 exact-pruned
✓ Verified.

Searching R=9 (nauty depth limit: 8)  ver[0]=ABCDEFGHI  ver[1]=JBCDEFGHI
  1/6 branches | 165314750 evaluated | 525.5s
  2/6 branches | 175213668 evaluated | 571.2s
  3/6 branches | 194499500 evaluated | 647.7s
  4/6 branches | 194499500 evaluated | 647.7s
  5/6 branches | 194499500 evaluated | 647.7s
  6/6 branches | 194499500 evaluated | 647.7s
(9nk- 8) (n-k)-36, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI ABCDEFPHI ABCDEFGQI
(9nk- 9) (n-k)-29, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI ABCDEFPHI JKCDEFGHI
(9nk-10) (n-k)-24, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI ABCDEOGHI JKCDEFGHI JBLDEFGHI
(9nk-11) (n-k)-21, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI ABCDNFGHI JKCDEFGHI JBLDEFGHI JBCMEFGHI
(9nk-12) (n-k)-18, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI JKCDEFGHI JBLDEFGHI JBCMEFGHI AKLDEFGHI
(9nk-13) (n-k)-16, EX: ABCDEFGHI JBCDEFGHI AKCDEFGHI ABLDEFGHI ABCMEFGHI JKCDEFGHI JBLDEFGHI AKLDEFGHI JKLDEFGHI
Done: 647.703s, 199291099 generated, 194499500 evaluated, 3760050 iso-pruned, 0 exact-pruned
✓ Verified.
```

### Summary table

| R   | (R-1)-extraconnectivity | Paper reference     |
| --- | ----------------------- | ------------------- |
| 2   | (2k−1)(n−k) − 1         | Theorem 1           |
| 3   | (3k−2)(n−k) − 3         | Theorem 2           |
| 4   | (4k−4)(n−k) − 4         | Theorem 3           |
| 5   | (5k−5)(n−k) − 7         | Theorem 5           |
| 6   | (6k−7)(n−k) − 9         | Theorem 6           |
| 7   | (7k−9)(n−k) − 11        | Theorem 7           |
| 8   | **(8k−12)(n−k) − 12**   | **New (this work)** |
| 9   | **(9k−13)(n−k) − 16**   | **New (this work)** |

> **Note:** For R=5..7, the minimum follows ((r+1)k−(2r−3))(n−k)−(2r−1).
> R=8 and R=9 **break this pattern** — the (n-k) coefficient jumps by more than 2,
> and the intermediate classes are absent.

## Reference

E. Cheng, L. Lipták, D. Tian. "On the Extraconnectivity of Arrangement Graphs."
_Springer Proceedings in Mathematics & Statistics_ 388, pp. 275–282, 2022.

---
*Computational verification by Shane Jaroch.*
