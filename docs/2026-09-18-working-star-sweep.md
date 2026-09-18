# Star sweep epic 9/18/26

```shell
shane@coffeelake:~/Documents/school/ou-papers/program-cheng-connectivity-asymptote$ ./bin/validate_extra_cut_plan 28 7
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
