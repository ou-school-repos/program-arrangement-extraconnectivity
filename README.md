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

```
Searching R=2  ver[0]=AB  ver[1]=CB
(2nk-1) (n-k)-1, EX: AB CB 
Done: 2.4806e-05s, 1 evaluated, 0 pruned

Searching R=3  ver[0]=ABC  ver[1]=DBC
(3nk-2) (n-k)-3, EX: ABC DBC AEC 
Done: 2.5409e-05s, 5 evaluated, 1 pruned

Searching R=4  ver[0]=ABCD  ver[1]=EBCD
(4nk-3) (n-k)- 6, EX: ABCD EBCD AFCD ABGD 
(4nk-4) (n-k)- 4, EX: ABCD EBCD AFCD EFCD 
Done: 6.0481e-05s, 60 evaluated, 27 pruned

Searching R=5  ver[0]=ABCDE  ver[1]=FBCDE
(5nk-4) (n-k)-10, EX: ABCDE FBCDE AGCDE ABHDE ABCIE 
(5nk-5) (n-k)- 7, EX: ABCDE FBCDE AGCDE ABHDE FGCDE 
Done: 0.000920653s, 1310 evaluated, 931 pruned

Searching R=6  ver[0]=ABCDEF  ver[1]=GBCDEF
(6nk-5) (n-k)-15, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF ABCDKF 
(6nk-6) (n-k)-11, EX: ABCDEF GBCDEF AHCDEF ABIDEF ABCJEF GHCDEF 
(6nk-7) (n-k)- 9, EX: ABCDEF GBCDEF AHCDEF ABIDEF GHCDEF GBIDEF 
Done: 0.0455033s, 44860 evaluated, 42792 pruned

Searching R=7  ver[0]=ABCDEFG  ver[1]=HBCDEFG
(7nk-6) (n-k)-21, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG ABCDEMG 
(7nk-7) (n-k)-16, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG ABCDLFG HICDEFG 
(7nk-8) (n-k)-13, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG ABCKEFG HICDEFG HBJDEFG 
(7nk-9) (n-k)-11, EX: ABCDEFG HBCDEFG AICDEFG ABJDEFG HICDEFG HBJDEFG AIJDEFG 
Done: 3.3146s, 2207784 evaluated, 2612060 pruned
```

### Summary table

| R | (R-1)-extraconnectivity | Paper reference |
|---|---|---|
| 2 | (2k−1)(n−k) − 1 | Theorem 1 |
| 3 | (3k−2)(n−k) − 3 | Theorem 2 |
| 4 | (4k−4)(n−k) − 4 | Theorem 3 |
| 5 | (5k−5)(n−k) − 7 | Theorem 5 |
| 6 | (6k−7)(n−k) − 9 | Theorem 6 |
| 7 | (7k−9)(n−k) − 11 | Theorem 7 |

## Building & running

```sh
make build/opt        # compile optimized binary
make benchmark        # run R=2..7 (override with R=N)
make benchmark R=8    # run R=2..8
make run/opt R=7      # run single R value
make test/opt         # verify against baseline
make lint             # cppcheck + clang-tidy
```

## Files

- `arrangementoptimized.cpp` — optimized search with bitwise vertex packing
  and canonical set deduplication
- `cheng/arrangement.cpp` — baseline C++ port of the original paper's Java code
- `cheng/cheng.java` — original Java code from the paper appendix

## Reference

E. Cheng, L. Lipták, D. Tian. "On the Extraconnectivity of Arrangement Graphs."
*Springer Proceedings in Mathematics & Statistics* 388, pp. 275–282, 2022.
