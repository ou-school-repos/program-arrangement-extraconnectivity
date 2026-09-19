# Star sweep epic 9/18/26

## Overall plan

```log
› which commands / frontiers shall i explore? list the commands i shall run and why they are worth (briefly) exploring.


• Explored
  └ Search Usage:|boundary-star|--boundary|--audit|--verify|--verify-range|--csv in predict.cpp
    Read merge_frontier_chunks.cpp

• Recommended exploration order:

  ./bin/validate_extra_cut_plan 28 7
  ./bin/predict --boundary-star 28 7
  ./bin/search_hybrids 28 7 100000 4

  Why: A(28,7) is the highest-priority unknown soft candidate. The planner confirms feasibility, the predictor gives the exact Star/Hamming comparison, and the hybrid search tests
  nearby structures cheaply before any global BFS.

  Then compare neighboring (k=7) cases:

  ./bin/validate_extra_cut_plan 26 7
  ./bin/validate_extra_cut_plan 27 7
  ./bin/validate_extra_cut_plan 28 7

  ./bin/predict --boundary-star 26 7
  ./bin/predict --boundary-star 27 7
  ./bin/predict --boundary-star 28 7

  ./bin/search_hybrids 26 7 100000 4
  ./bin/search_hybrids 27 7 100000 4
  ./bin/search_hybrids 28 7 100000 4

  These reveal whether the apparent transition from hard to soft candidates at (n=26,27,28) is structural or merely a theorem-gate artifact.

  For a deeper local search after the short runs:

  ./bin/search_hybrids 28 7 5000000 8

  Only after the local search is uninteresting should you launch the expensive validator:

  time ./bin/validate_extra_cut_bitmap 28 7 \
    --disk-backed ./state_A28_7

  Other worthwhile frontier cases, in descending priority:

  ./bin/validate_extra_cut_plan 16 8
  ./bin/validate_extra_cut_plan 16 10
  ./bin/validate_extra_cut_plan 17 7
  ./bin/validate_extra_cut_plan 15 8
  ./bin/validate_extra_cut_plan 14 10

  Use the planner first for each. The global bitmap validator should only be attempted when the bitset footprint and expected runtime are acceptable.

  done 7:53 PM

```

---

## Survey: `A(28,7)`

### Preview / survey (case 1)

```shell
$ ./bin/validate_extra_cut_plan 28 7
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-466-gfa578f8d6dfa-dirty)
Planning full-Star A(28,7)
No graph/BFS state allocated; this is an arithmetic plan.
m = 21
degree = 147
|V| = nP_k = 5967561600
n^k = 13492928512
|E| = |V|*degree/2 = 438615777600
R = 148, g = 147
d = ceil(log2(R)) = 8
|N(S)| = 9702
Hamming baseline = 10520
Delta = Hamming - Star = 818
Embedding gate: closed
Ranked uint32 frontier: does not fit
Ranked uint64 frontier: fits
One bitset:        745945200 bytes [711 MiB (binary)]
Three bitmap states:       2237835600 bytes [2 GiB (binary)]
Flat n^k guard:      13492928512 slots
Star classification: soft-counterexample candidate
Note: connectivity and extra-cut validity still require the validator.

shane@coffeelake:~/Documents/school/ou-papers/program-cheng-connectivity-asymptote$ ./bin/predict --boundary-star 28 7
A(28,7): R=148 (full Star volume)
  Hamming baseline: 10520
  Star boundary:    9702
  Delta:            818
```

### Neighbor (`k=7`) cases

```shell
$ ./bin/validate_extra_cut_plan 26 7
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-476-ge645c49320ce)
Planning full-Star A(26,7)
No graph/BFS state allocated; this is an arithmetic plan.
m =                       19
degree =                  133
|V| = nP_k =              3315312000
n^k =                     8031810176
|E| = |V|*degree/2 =      220468248000
R =                       134, g = 133
d = ceil(log2(R)) =       8
|N(S)| =                  7980
Hamming baseline =        8574
Delta = Hamming - Star =  594
Embedding gate: closed
Ranked uint32 frontier: fits
Ranked uint64 frontier: fits
One bitset:                 414414000 bytes [395 MiB (binary)]
Three bitmap states:       1243242000 bytes [1 GiB (binary)]
Flat n^k guard:            8031810176 slots
Star classification: soft-counterexample candidate
Note: connectivity and extra-cut validity still require the validator.

$ ./bin/validate_extra_cut_plan 27 7
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-476-ge645c49320ce)
Planning full-Star A(27,7)
No graph/BFS state allocated; this is an arithmetic plan.
m =                       20
degree =                  140
|V| = nP_k =              4475671200
n^k =                     10460353203
|E| = |V|*degree/2 =      313296984000
R =                       141, g = 140
d = ceil(log2(R)) =       8
|N(S)| =                  8820
Hamming baseline =        9550
Delta = Hamming - Star =  730
Embedding gate: closed
Ranked uint32 frontier: does not fit
Ranked uint64 frontier: fits
One bitset:                 559458900 bytes [533 MiB (binary)]
Three bitmap states:       1678376700 bytes [1 GiB (binary)]
Flat n^k guard:           10460353203 slots
Star classification: soft-counterexample candidate
Note: connectivity and extra-cut validity still require the validator.


$ ./bin/validate_extra_cut_plan 28 7
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-476-ge645c49320ce)
Planning full-Star A(28,7)
No graph/BFS state allocated; this is an arithmetic plan.
m =                       21
degree =                  147
|V| = nP_k =              5967561600
n^k =                     13492928512
|E| = |V|*degree/2 =      438615777600
R =                       148, g = 147
d = ceil(log2(R)) =       8
|N(S)| =                  9702
Hamming baseline =        10520
Delta = Hamming - Star =  818
Embedding gate: closed
Ranked uint32 frontier: does not fit
Ranked uint64 frontier: fits
One bitset:                 745945200 bytes [711 MiB (binary)]
Three bitmap states:       2237835600 bytes [2 GiB (binary)]
Flat n^k guard:           13492928512 slots
Star classification: soft-counterexample candidate
Note: connectivity and extra-cut validity still require the validator.



$ ./bin/predict --boundary-star 26 7
A(26,7): R=134 (full Star volume)
  Hamming baseline: 8574
  Star boundary:    7980
  Delta:            594
$ ./bin/predict --boundary-star 27 7
A(27,7): R=141 (full Star volume)
  Hamming baseline: 9550
  Star boundary:    8820
  Delta:            730
$ ./bin/predict --boundary-star 28 7
A(28,7): R=148 (full Star volume)
  Hamming baseline: 10520
  Star boundary:    9702
  Delta:            818



$ ./bin/search_hybrids 26 7 100000 4
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-467-g74bdf9abda7d-dirty)
Local hybrid search for A(26,7)
Target subset size: 134
Degree: 133
Full-Star internal edges: 1330
Full-Star boundary: 15162
Iterations per restart: 100000
Restarts: 4
Restart 1/4: initial boundary 15054
NEW BEST: internal edges 1384, boundary 15054
Restart 2/4: initial boundary 14946
NEW BEST: internal edges 1438, boundary 14946
Restart 3/4: initial boundary 15054
Restart 4/4: initial boundary 14946
Best observed internal edges: 1438
Best observed boundary: 14946
Potential smaller-boundary structure found; this is not a proof of optimality.
Witness subset (134 vertices):
  [0,1,2,3,4,5,6]
  [7,1,2,3,4,5,6]
  [8,1,2,3,4,5,6]
  [9,1,2,3,4,5,6]
  [10,1,2,3,4,5,6]
  [11,1,2,3,4,5,6]
  [12,1,2,3,4,5,6]
  [13,1,2,3,4,5,6]
  [14,1,2,3,4,5,6]
  [15,1,2,3,4,5,6]
  [16,1,2,3,4,5,6]
  [17,1,2,3,4,5,6]
  [18,1,2,3,4,5,6]
  [19,1,2,3,4,5,6]
  [20,1,2,3,4,5,6]
  [21,1,2,3,4,5,6]
  [22,1,2,3,4,5,6]
  [23,1,2,3,4,5,6]
  [24,1,2,3,4,5,6]
  [25,1,2,3,4,5,6]
  [8,1,2,3,4,5,25]
  [8,1,2,3,4,5,7]
  [8,1,2,3,4,5,0]
  [8,1,2,3,4,5,24]
  [8,1,2,3,4,5,23]
  [8,1,2,3,4,5,22]
  [8,1,2,3,4,5,21]
  [8,1,2,3,4,5,20]
  [8,1,2,3,4,5,19]
  [8,1,2,3,4,5,18]
  [8,1,2,3,4,5,17]
  [8,1,2,3,4,5,16]
  [8,1,2,3,4,5,15]
  [8,1,2,3,4,5,14]
  [8,1,2,3,4,5,13]
  [8,1,2,3,4,5,12]
  [8,1,2,3,4,5,11]
  [8,1,2,3,4,5,10]
  [8,1,2,3,4,5,9]
  [0,1,2,3,4,5,7]
  [0,1,2,3,4,5,9]
  [0,1,2,3,4,5,10]
  [0,1,2,3,4,5,11]
  [0,1,2,3,4,5,12]
  [0,1,2,3,4,5,13]
  [0,1,2,3,4,5,14]
  [0,1,2,3,4,5,15]
  [0,1,2,3,4,5,16]
  [0,1,2,3,4,5,17]
  [0,1,2,3,4,5,18]
  [0,1,2,3,4,5,19]
  [0,1,2,3,4,5,20]
  [0,1,2,3,4,5,21]
  [0,1,2,3,4,5,22]
  [0,1,2,3,4,5,23]
  [0,1,2,3,4,5,24]
  [0,1,2,3,4,5,25]
  [0,1,2,3,4,5,8]
  [7,1,2,3,4,5,9]
  [7,1,2,3,4,5,10]
  [7,1,2,3,4,5,11]
  [7,1,2,3,4,5,12]
  [7,1,2,3,4,5,13]
  [7,1,2,3,4,5,14]
  [7,1,2,3,4,5,15]
  [7,1,2,3,4,5,16]
  [7,1,2,3,4,5,17]
  [7,1,2,3,4,5,18]
  [7,1,2,3,4,5,19]
  [7,1,2,3,4,5,20]
  [7,1,2,3,4,5,21]
  [7,1,2,3,4,5,22]
  [7,1,2,3,4,5,23]
  [7,1,2,3,4,5,24]
  [7,1,2,3,4,5,25]
  [7,1,2,3,4,5,0]
  [7,1,2,3,4,5,8]
  [8,1,2,3,4,25,6]
  [8,1,2,3,4,24,6]
  [8,1,2,3,4,23,6]
  [8,1,2,3,4,22,6]
  [8,1,2,3,4,21,6]
  [8,1,2,3,4,20,6]
  [8,1,2,3,4,19,6]
  [8,1,2,3,4,18,6]
  [8,1,2,3,4,17,6]
  [8,1,2,3,4,16,6]
  [8,1,2,3,4,15,6]
  [8,1,2,3,4,14,6]
  [8,1,2,3,4,13,6]
  [8,1,2,3,4,12,6]
  [8,1,2,3,4,11,6]
  [8,1,2,3,4,10,6]
  [8,1,2,3,4,9,6]
  [8,1,2,3,4,7,6]
  [8,1,2,3,4,0,6]
  [0,1,2,3,4,7,6]
  [0,1,2,3,4,9,6]
  [0,1,2,3,4,10,6]
  [0,1,2,3,4,11,6]
  [0,1,2,3,4,12,6]
  [0,1,2,3,4,13,6]
  [0,1,2,3,4,14,6]
  [0,1,2,3,4,15,6]
  [0,1,2,3,4,16,6]
  [0,1,2,3,4,17,6]
  [0,1,2,3,4,18,6]
  [0,1,2,3,4,19,6]
  [0,1,2,3,4,20,6]
  [0,1,2,3,4,21,6]
  [0,1,2,3,4,22,6]
  [0,1,2,3,4,23,6]
  [0,1,2,3,4,24,6]
  [0,1,2,3,4,25,6]
  [0,1,2,3,4,8,6]
  [7,1,2,3,4,9,6]
  [7,1,2,3,4,10,6]
  [7,1,2,3,4,11,6]
  [7,1,2,3,4,12,6]
  [7,1,2,3,4,13,6]
  [7,1,2,3,4,14,6]
  [7,1,2,3,4,15,6]
  [7,1,2,3,4,16,6]
  [7,1,2,3,4,17,6]
  [7,1,2,3,4,18,6]
  [7,1,2,3,4,19,6]
  [7,1,2,3,4,20,6]
  [7,1,2,3,4,21,6]
  [7,1,2,3,4,22,6]
  [7,1,2,3,4,23,6]
  [7,1,2,3,4,24,6]
  [7,1,2,3,4,25,6]
  [7,1,2,3,4,0,6]
  [7,1,2,3,4,8,6]


$ ./bin/search_hybrids 28 7 5000000 8
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-473-g86673d67b80d-dirty)
Local hybrid search for A(28,7)
Target subset size: 148
Degree: 147
Full-Star internal edges: 1617
Full-Star boundary: 9702
Iterations per restart: 5000000
Restarts: 8
Restart 1/8: initial boundary 13422
  improvement at iteration 171: vertex boundary 13404
  improvement at iteration 214: vertex boundary 13386
  improvement at iteration 253: vertex boundary 13369
  improvement at iteration 300: vertex boundary 13353
  improvement at iteration 317: vertex boundary 13335
  improvement at iteration 368: vertex boundary 13319
  improvement at iteration 441: vertex boundary 13317
  improvement at iteration 516: vertex boundary 13300
  improvement at iteration 606: vertex boundary 13284
  improvement at iteration 742: vertex boundary 13268
  improvement at iteration 796: vertex boundary 13267
  improvement at iteration 860: vertex boundary 13250
  improvement at iteration 898: vertex boundary 13237
  improvement at iteration 900: vertex boundary 13230
  improvement at iteration 901: vertex boundary 13214
  improvement at iteration 977: vertex boundary 13212
  improvement at iteration 1043: vertex boundary 13211
  improvement at iteration 1047: vertex boundary 13208
  improvement at iteration 1065: vertex boundary 13196
  improvement at iteration 1135: vertex boundary 13182
  improvement at iteration 1137: vertex boundary 13180
  improvement at iteration 1148: vertex boundary 13162
  improvement at iteration 1171: vertex boundary 13149
  improvement at iteration 1177: vertex boundary 13136
  improvement at iteration 1183: vertex boundary 13135
  improvement at iteration 1207: vertex boundary 13124
  improvement at iteration 1235: vertex boundary 13121
  improvement at iteration 1279: vertex boundary 13120
  improvement at iteration 1294: vertex boundary 13112
  improvement at iteration 1331: vertex boundary 13099
  improvement at iteration 1374: vertex boundary 13080
  improvement at iteration 1443: vertex boundary 13063
  improvement at iteration 1468: vertex boundary 13059
  improvement at iteration 1520: vertex boundary 13057
  improvement at iteration 1532: vertex boundary 13041
  improvement at iteration 1533: vertex boundary 13029
  improvement at iteration 1552: vertex boundary 13022
  improvement at iteration 1555: vertex boundary 13017
  improvement at iteration 1569: vertex boundary 12999
  improvement at iteration 1571: vertex boundary 12991
  improvement at iteration 1588: vertex boundary 12970
  improvement at iteration 1594: vertex boundary 12965
  improvement at iteration 1707: vertex boundary 12964
  improvement at iteration 1717: vertex boundary 12951
  improvement at iteration 1739: vertex boundary 12937
  improvement at iteration 1842: vertex boundary 12936
  improvement at iteration 1851: vertex boundary 12932
  improvement at iteration 1863: vertex boundary 12921
  improvement at iteration 2142: vertex boundary 12912
  improvement at iteration 2146: vertex boundary 12886
  improvement at iteration 2197: vertex boundary 12874
  improvement at iteration 2224: vertex boundary 12871
  improvement at iteration 2271: vertex boundary 12855
  improvement at iteration 2288: vertex boundary 12850
  improvement at iteration 2345: vertex boundary 12815
  improvement at iteration 2383: vertex boundary 12813
  improvement at iteration 2385: vertex boundary 12781
  improvement at iteration 2392: vertex boundary 12770
  improvement at iteration 2525: vertex boundary 12766
  improvement at iteration 2527: vertex boundary 12729
  improvement at iteration 2586: vertex boundary 12698
  improvement at iteration 2638: vertex boundary 12697
  improvement at iteration 2663: vertex boundary 12688
  improvement at iteration 2709: vertex boundary 12660
  improvement at iteration 2750: vertex boundary 12653
  improvement at iteration 2842: vertex boundary 12650
  improvement at iteration 2887: vertex boundary 12617
  improvement at iteration 2898: vertex boundary 12604
  improvement at iteration 2900: vertex boundary 12602
  improvement at iteration 2918: vertex boundary 12562
  improvement at iteration 3060: vertex boundary 12554
  improvement at iteration 3092: vertex boundary 12547
  improvement at iteration 3138: vertex boundary 12543
  improvement at iteration 3139: vertex boundary 12504
  improvement at iteration 3145: vertex boundary 12499
  improvement at iteration 3157: vertex boundary 12496
  improvement at iteration 3250: vertex boundary 12494
  improvement at iteration 3358: vertex boundary 12492
  improvement at iteration 3373: vertex boundary 12450
  improvement at iteration 3376: vertex boundary 12406
  improvement at iteration 3416: vertex boundary 12405
  improvement at iteration 3442: vertex boundary 12404
  improvement at iteration 3564: vertex boundary 12402
  improvement at iteration 3626: vertex boundary 12401
  improvement at iteration 3653: vertex boundary 12398
  improvement at iteration 3705: vertex boundary 12396
  improvement at iteration 3757: vertex boundary 12394
  improvement at iteration 3775: vertex boundary 12393
  improvement at iteration 3796: vertex boundary 12356
  improvement at iteration 3863: vertex boundary 12311
  improvement at iteration 3873: vertex boundary 12310
  improvement at iteration 3884: vertex boundary 12309
  improvement at iteration 3886: vertex boundary 12271
  improvement at iteration 3895: vertex boundary 12225
  improvement at iteration 3985: vertex boundary 12189
  improvement at iteration 4056: vertex boundary 12140
  improvement at iteration 4073: vertex boundary 12139
  improvement at iteration 4076: vertex boundary 12092
  improvement at iteration 4233: vertex boundary 12041
  improvement at iteration 4238: vertex boundary 11989
  improvement at iteration 4240: vertex boundary 11938
  improvement at iteration 4288: vertex boundary 11933
  improvement at iteration 4374: vertex boundary 11881
  improvement at iteration 4466: vertex boundary 11821
  improvement at iteration 4536: vertex boundary 11768
  improvement at iteration 4564: vertex boundary 11710
  improvement at iteration 4655: vertex boundary 11707
  improvement at iteration 4669: vertex boundary 11647
  improvement at iteration 4726: vertex boundary 11580
  improvement at iteration 4908: vertex boundary 11574
  improvement at iteration 5011: vertex boundary 11571
  improvement at iteration 5212: vertex boundary 11569
  improvement at iteration 5290: vertex boundary 11568
  improvement at iteration 5304: vertex boundary 11504
  improvement at iteration 5377: vertex boundary 11503
  improvement at iteration 5481: vertex boundary 11438
  improvement at iteration 5594: vertex boundary 11437
  improvement at iteration 5629: vertex boundary 11435
  improvement at iteration 5678: vertex boundary 11434
  improvement at iteration 5715: vertex boundary 11431
  improvement at iteration 5744: vertex boundary 11430
  improvement at iteration 6000: vertex boundary 11429
  improvement at iteration 6052: vertex boundary 11428
  improvement at iteration 6068: vertex boundary 11366
  improvement at iteration 6077: vertex boundary 11363
  improvement at iteration 6272: vertex boundary 11295
  improvement at iteration 6365: vertex boundary 11225
  improvement at iteration 6609: vertex boundary 11224
  improvement at iteration 6819: vertex boundary 11223
  improvement at iteration 7190: vertex boundary 11153
  improvement at iteration 7354: vertex boundary 11079
  improvement at iteration 7666: vertex boundary 11078
  improvement at iteration 7672: vertex boundary 11077
  improvement at iteration 7902: vertex boundary 11005
  improvement at iteration 8075: vertex boundary 10930
  improvement at iteration 8471: vertex boundary 10850
  improvement at iteration 8543: vertex boundary 10771
  improvement at iteration 8977: vertex boundary 10691
  improvement at iteration 11002: vertex boundary 10610
  improvement at iteration 14110: vertex boundary 10609
  improvement at iteration 16647: vertex boundary 10526
  improvement at iteration 16730: vertex boundary 10441
  improvement at iteration 18077: vertex boundary 10355
  improvement at iteration 18135: vertex boundary 10267
  improvement at iteration 18223: vertex boundary 10266
  improvement at iteration 21165: vertex boundary 10177
  improvement at iteration 21977: vertex boundary 10086
  improvement at iteration 24880: vertex boundary 9993
  improvement at iteration 28225: vertex boundary 9898
  improvement at iteration 32379: vertex boundary 9801
  improvement at iteration 41668: vertex boundary 9702
```
