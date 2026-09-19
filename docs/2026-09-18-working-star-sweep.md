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
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-466-gfa578f8d6dfa-dirty)
Planning full-Star A(26,7)
No graph/BFS state allocated; this is an arithmetic plan.
m = 19
degree = 133
|V| = nP_k = 3315312000
n^k = 8031810176
|E| = |V|*degree/2 = 220468248000
R = 134, g = 133
d = ceil(log2(R)) = 8
|N(S)| = 7980
Hamming baseline = 8574
Delta = Hamming - Star = 594
Embedding gate: closed
Ranked uint32 frontier: fits
Ranked uint64 frontier: fits
One bitset:        414414000 bytes [395 MiB (binary)]
Three bitmap states:       1243242000 bytes [1 GiB (binary)]
Flat n^k guard:       8031810176 slots
Star classification: soft-counterexample candidate
Note: connectivity and extra-cut validity still require the validator.

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


$ ./bin/search_hybrids 27 7 100000 4
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-467-g74bdf9abda7d-dirty)
Local hybrid search for A(27,7)
Target subset size: 141
Degree: 140
Full-Star internal edges: 1470
Full-Star boundary: 16800
Iterations per restart: 100000
Restarts: 4
Restart 1/4: initial boundary 16686
NEW BEST: internal edges 1527, boundary 16686
Restart 2/4: initial boundary 16572
NEW BEST: internal edges 1584, boundary 16572
Restart 3/4: initial boundary 16686
Restart 4/4: initial boundary 16572
Best observed internal edges: 1584
Best observed boundary: 16572
Potential smaller-boundary structure found; this is not a proof of optimality.
Witness subset (141 vertices):
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
  [26,1,2,3,4,5,6]
  [8,1,2,3,4,5,26]
  [8,1,2,3,4,5,7]
  [8,1,2,3,4,5,0]
  [8,1,2,3,4,5,25]
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
  [0,1,2,3,4,5,26]
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
  [7,1,2,3,4,5,26]
  [7,1,2,3,4,5,0]
  [7,1,2,3,4,5,8]
  [8,1,2,3,4,26,6]
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
  [0,1,2,3,4,26,6]
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
  [7,1,2,3,4,26,6]
  [7,1,2,3,4,0,6]
  [7,1,2,3,4,8,6]


$ ./bin/search_hybrids 28 7 100000 4
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-467-g74bdf9abda7d-dirty)
Local hybrid search for A(28,7)
Target subset size: 148
Degree: 147
Full-Star internal edges: 1617
Full-Star boundary: 18522
Iterations per restart: 100000
Restarts: 4
Restart 1/4: initial boundary 18402
NEW BEST: internal edges 1677, boundary 18402
Restart 2/4: initial boundary 18282
NEW BEST: internal edges 1737, boundary 18282
Restart 3/4: initial boundary 18402
Restart 4/4: initial boundary 18282
Best observed internal edges: 1737
Best observed boundary: 18282
Potential smaller-boundary structure found; this is not a proof of optimality.
Witness subset (148 vertices):
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
  [26,1,2,3,4,5,6]
  [27,1,2,3,4,5,6]
  [8,1,2,3,4,5,27]
  [8,1,2,3,4,5,7]
  [8,1,2,3,4,5,0]
  [8,1,2,3,4,5,26]
  [8,1,2,3,4,5,25]
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
  [0,1,2,3,4,5,26]
  [0,1,2,3,4,5,27]
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
  [7,1,2,3,4,5,26]
  [7,1,2,3,4,5,27]
  [7,1,2,3,4,5,0]
  [7,1,2,3,4,5,8]
  [8,1,2,3,4,27,6]
  [8,1,2,3,4,26,6]
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
  [0,1,2,3,4,26,6]
  [0,1,2,3,4,27,6]
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
  [7,1,2,3,4,26,6]
  [7,1,2,3,4,27,6]
  [7,1,2,3,4,0,6]
  [7,1,2,3,4,8,6]


$ ./bin/search_hybrids 28 7 5000000 8
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-467-g74bdf9abda7d-dirty)
Local hybrid search for A(28,7)
Target subset size: 148
Degree: 147
Full-Star internal edges: 1617
Full-Star boundary: 18522
Iterations per restart: 5000000
Restarts: 8
Restart 1/8: initial boundary 18402
NEW BEST: internal edges 1677, boundary 18402
Restart 2/8: initial boundary 18282
```
