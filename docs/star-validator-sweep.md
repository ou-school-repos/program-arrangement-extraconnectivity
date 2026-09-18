# Full star validator sweep

Research log for `bin/validate_extra_cut`, recorded 2026-09-17.

The validator constructs the full radius-one Star in `A(n,k)`, computes its
external boundary, and checks the extra-cut condition after deleting that
boundary. A successful run proves _an_ explicit upper bound on extraconnectivity
that is crucially strictly better than the Hamming ball local optimum.

## Parameters and formulas

$$
\begin{aligned}
m &= n-k,\\
\lvert V(A(n,k))\rvert
  &= nP_k
   = \frac{n!}{(n-k)!},\\
\\
\deg(A(n,k)) &= km,\\
\lvert E(A(n,k))\rvert
  &= \frac{\lvert V(A(n,k))\rvert\,km}{2},\\
\\
R &= \lvert S\rvert = 1+km,\\
g &= R-1=km,\\
d &= \operatorname{BitLength}(R-1).
\end{aligned}
$$

For the full Star: $|N(S)| = k(k - 1)m(m + 1) / 2$, the Hamming comparison is:
$H(n,k,R) = (R*k - E(R)) * (n-k) - C(R)$.

Here `E(R)` and `C(R)` are Hamming-collision correction terms; `E(R)` is not the
graph edge count `|E(A(n,k))|` above.

The embedding gate is open exactly when $d \le k$ and $d \le n-k$.

## Classification

- `HARD COUNTEREXAMPLE`: the Star is a valid extra cut, beats the Hamming
  baseline, and the embedding gate is open.
- `SOFT COUNTEREXAMPLE`: the Star beats the formal Hamming baseline, but the
  embedding gate is closed.
- `SATISFIES HAMMING OPTIMALITY`: the valid Star cut does not beat the Hamming
  baseline.
- `INVALID EXTRA CUT`: deletion leaves a component of size at most `g`.

## Sweep map: Star extreme `S(n,k)`

Each `(n,k)` cell tests the full radius-one Star at $R = 1 + k(n-k)$, with
$g = R - 1$. This is one selected volume per graph parameter pair, not a sweep
over arbitrary `R` values.

Legend:

- `■` — hard counterexample to `RestrictedLowerBound`.
- `⊞` — soft counterexample to `UniversalLowerBound` only.
- `∘` — satisfies the Hamming comparison.
- `\` — invalid extra cut or outside `k < n`.
- `/` — not yet run, or presumed thus far intractable.
- `•` — unknown, but of interest.
- `?` — unknown, but of interest; previously attempted.

| `n \ k` |  3  |  4  |  5  |  6  |  7  |  8  |  9  | 10  | 11  | 12  | 13  | 14  | 15  |
| ------: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
|       4 |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  |
|       5 |  ∘  |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  |
|       6 |  ∘  |  ∘  |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  |
|       7 |  ∘  |  ∘  |  ∘  |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  |
|       8 |  ∘  |  ∘  |  ∘  |  ∘  |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  | \\  | \\  |
|       9 |  ∘  |  ∘  |  ∘  |  ⊞  |  ∘  |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  | \\  |
|      10 |  ∘  |  ∘  |  ∘  |  ⊞  |  ⊞  |  ⊞  |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  |
|      11 |  ∘  |  ∘  |  ∘  |  ■  |  ⊞  |  ⊞  |  ⊞  |  ∘  | \\  | \\  | \\  | \\  | \\  |
|      12 |  ∘  |  ∘  |  ∘  |  ■  |  ⊞  |  ⊞  |  ⊞  |  ⊞  |  ∘  | \\  | \\  | \\  | \\  |
|      13 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ⊞  |  ⊞  |  ⊞  |  ⊞  |  ∘  | \\  | \\  | \\  |
|      14 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ■  |  ⊞  |  ⊞  |  ∘  |  •  |  /  | \\  | \\  |
|      15 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ■  |  ■  |  ⊞  |  /  |  /  |  /  |  /  | \\  |
|      16 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ■  |  ■  |  •  |  /  |  /  |  /  |  /  |  /  |
|      17 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  ■  |  /  |  •  |  /  |  /  |  /  |  /  |  /  |
|      18 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      19 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      20 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      21 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      22 |  ∘  |  ∘  |  ∘  |  ∘  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      23 |  ∘  |  ∘  |  ∘  |  ∘  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      24 |  ∘  |  ∘  |  ∘  |  ∘  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      25 |  ∘  |  ∘  |  ∘  |  ∘  |  ■  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      26 |  ∘  |  ∘  |  ∘  |  ∘  |  ⊞  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      27 |  ∘  |  ∘  |  ∘  |  ∘  |  ⊞  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      28 |  ∘  |  ∘  |  ∘  |  ∘  |  •  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      29 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      30 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      31 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |
|      32 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |  /  |

<!-- Example commands -->

- `time for i in $(seq 11 14); do for j in $(seq 9 10); do ./bin/validate_extra_cut $i $j; done done`
- `time for i in $(seq 21 26); do for j in $(seq 1 6);  do ./bin/validate_extra_cut $i $j; done done`
- `time for i in $(seq 22 26); do time ./bin/validate_extra_cut_bitmap $i 7; done`

The smallest known hard counterexample, and the smallest hard counterexample
within the full-Star family, is `A(11,6)` with `R=31`. This does not prove that
no non-Star topology gives a smaller hard counterexample.

The `A(11,9)` case is now deeply confirmed as a soft counterexample after
raising the flat-space guard. The complete `A(14, 1..8)` row is now confirmed.
The feasible `A(15, 1..8)`, `A(16, 1..8)`, `A(17, 1..8)`, and `A(18, 1..8)`
cells are also confirmed; their remaining cells are marked `/` because they were
not run or were intractable. The `A(19,1..7)` and `A(20,1..7)` rows are now
confirmed; their remaining cells are likewise marked `/`.

The largest deeply certified hard counterexample so far is `A(25,7)`, with
`R=127`, `g=126`, and `|N(S)|=7182` (~6.5 minutes to compute).

## Representative hard counterexamples

Sorted by `k`, then valid-vertex count `|V| = nP_k`. The graph edge count is
$|E| = |V| * k (n-k) / 2$. Here $|∂S| = |N(S)|$ is the measured **Star
boundary**, while $|∂H|$ denotes the **Hamming baseline** for volume `R`.

|  #   | graph     |         \|V\| |           \|E\| |          `n^k` | `R` | `g` | \|∂S\| |  \|∂H\| |  `Δ` | `d,(k,n-k)` |
| :--: | --------- | ------------: | --------------: | -------------: | --: | --: | -----: | ------: | ---: | :---------- |
| 6.1  | `A(11,6)` |       332,640 |       4,989,600 |      1,771,561 |  31 |  30 |    450 |     476 |   26 | `5,(6,5)`   |
| 6.2  | `A(12,6)` |       665,280 |      11,975,040 |      2,985,984 |  37 |  36 |    630 |     687 |   57 | `6,(6,6)`   |
| 6.3  | `A(13,6)` |     1,235,520 |      25,945,920 |      4,826,809 |  43 |  42 |    840 |     921 |   81 | `6,(6,7)`   |
| 6.4  | `A(14,6)` |     2,162,160 |      51,891,840 |      7,529,536 |  49 |  48 |  1,080 |   1,163 |   83 | `6,(6,8)`   |
| 6.5  | `A(15,6)` |     3,603,600 |      97,297,200 |     11,390,625 |  55 |  54 |  1,350 |   1,441 |   91 | `6,(6,9)`   |
| 6.6  | `A(16,6)` |     5,765,760 |     172,972,800 |     16,777,216 |  61 |  60 |  1,650 |   1,713 |   63 | `6,(6,10)`  |
|      | **k=7**   |               |                 |                |     |     |        | **k=7** |      |             |
| 7.1  | `A(13,7)` |     8,648,640 |     181,621,440 |     62,748,517 |  43 |  42 |    882 |   1,029 |  147 | `6,(7,6)`   |
| 7.2  | `A(14,7)` |    17,297,280 |     423,783,360 |    105,413,504 |  50 |  49 |  1,176 |   1,366 |  190 | `6,(7,7)`   |
| 7.3  | `A(15,7)` |    32,432,400 |     908,107,200 |    170,859,375 |  57 |  56 |  1,512 |   1,744 |  232 | `6,(7,8)`   |
| 7.4  | `A(16,7)` |    57,657,600 |   1,816,214,400 |    268,435,456 |  64 |  63 |  1,890 |   2,112 |  222 | `6,(7,9)`   |
| 7.5  | `A(17,7)` |    98,017,920 |   3,430,627,200 |    410,338,673 |  71 |  70 |  2,310 |   2,658 |  348 | `7,(7,10)`  |
| 7.6  | `A(18,7)` |   160,392,960 |   6,175,128,960 |    612,220,032 |  78 |  77 |  2,772 |   3,200 |  428 | `7,(7,11)`  |
| 7.7  | `A(19,7)` |   253,955,520 |  10,666,131,840 |    893,871,739 |  85 |  84 |  3,276 |   3,783 |  507 | `7,(7,12)`  |
| 7.8  | `A(20,7)` |   390,700,800 |  17,776,886,400 |  1,280,000,000 |  92 |  91 |  3,822 |   4,356 |  534 | `7,(7,13)`  |
| 7.9  | `A(21,7)` |   586,051,200 |  28,716,508,800 |  1,801,088,541 |  99 |  98 |  4,410 |   4,982 |  572 | `7,(7,14)`  |
| 7.10 | `A(22,7)` |   859,541,760 |  45,125,942,400 |  2,494,357,888 | 106 | 105 |  5,040 |   5,664 |  624 | `7,(7,15)`  |
| 7.11 | `A(23,7)` | 1,235,591,280 |  69,193,111,680 |  3,404,825,447 | 113 | 112 |  5,712 |   6,315 |  603 | `7,(7,16)`  |
| 7.12 | `A(24,7)` | 1,744,364,160 | 103,789,667,520 |  4,586,471,424 | 120 | 119 |  6,426 |   6,984 |  558 | `7,(7,17)`  |
| 7.13 | `A(25,7)` | 2,422,728,000 | 152,631,864,000 |  6,103,515,625 | 127 | 126 |  7,182 |   7,617 |  435 | `7,(7,18)`  |
|      | **k=8**   |               |                 |                |     |     |        | **k=8** |      |             |
| 8.1  | `A(14,8)` |   121,080,960 |   2,905,943,040 |  1,475,789,056 |  49 |  48 |  1,176 |   1,423 |  247 | `6,(8,6)`   |
| 8.2  | `A(15,8)` |   259,459,200 |   7,264,857,600 |  2,562,890,625 |  57 |  56 |  1,568 |   1,903 |  335 | `6,(8,7)`   |
| 8.3  | `A(16,8)` |   518,918,400 |  16,605,388,800 |  4,294,967,296 |  65 |  64 |  2,016 |   2,417 |  401 | `7,(8,8)`   |
| 8.4  | `A(17,8)` |   980,179,200 |  35,286,451,200 |  6,975,757,441 |  73 |  72 |  2,520 |   3,088 |  568 | `7,(8,9)`   |
| 8.5  | `A(18,8)` | 1,764,322,560 |  70,572,902,400 | 11,019,960,576 |  81 |  80 |  3,080 |   3,782 |  702 | `7,(8,10)`  |
| 8.6  | `A(19,8)` | 3,047,466,240 | 134,088,514,560 | 16,983,563,041 |  89 |  88 |  3,696 |   4,538 |  842 | `7,(8,11)`  |
| 8.?  | `A(21,8)` | 8,204,716,800 | 426,645,273,600 | 37,822,859,361 | 105 | 104 |  5,096 |   6,188 | 1092 | `7,(8,13)`  |
|      | **k=9**   |               |                 |                |     |     |        | **k=9** |      |             |
| 9.1  | `A(15,9)` | 1,816,214,400 |  49,037,788,800 | 38,443,359,375 |  55 |  54 |  1,512 |   1,894 |  382 | `6,(9,6)`   |

## TODOs

```shell
gg@vps76:~$

cd program-cheng-connectivity-asymptote/
mkdir -p .tmp/
export n=14
export k=10
nohup /usr/bin/time -v   -o .tmp/star_A${n}_k${k}.time   ./bin/validate_extra_cut_ranked ${n} ${k} >.tmp/star_A${n}_k${k}.log 2>&1 &
```

For the ranked backend, the best next target is below.

### 1. A(13,10)

- (n^k = 13^{10}=13,785,849,184) — impossible flat
- (V=13P\_{10}=1,037,836,800)
- Visited bitset: ~124 MiB
- Degree: (10(3)=30)
- Directed neighbor visits: ~31.1 billion
- Estimated runtime: 10–20 minutes

This is probably the best balance of low memory and useful coverage.

<!-- ### **[DONE!!]** 2. A(21,7)

- (n^k=1,801,088,541)
- (V=586,051,200)
- Visited bitset: ~70 MiB
- Directed neighbor visits: ~57.4 billion
- Estimated runtime: 20–30 minutes

Less memory, but more neighbor work. -->

<!-- ### **[DONE!!]** 3. A(15,9)

- (V=1,816,214,400)
- Visited bitset: ~216 MiB
- Directed neighbor visits: ~98.1 billion
- Estimated runtime: 25–40 minutes -->

### 4. A(14,10)

- (V=3,632,428,800)
- Visited bitset: ~433 MiB
- Directed neighbor visits: ~145.3 billion
- Estimated runtime: 40–60 minutes

I would run them in this order:

```bash
nohup /usr/bin/time -v -o .tmp/A13_k10.time \
  ./bin/validate_extra_cut_ranked 13 10 \
  > .tmp/A13_k10.log 2>&1 &
```

Then A(21,7), followed by A(15,9).

A(19,8) and A(16,9) are substantially riskier because the frontier may exceed 16
GB even though the rank bitset itself is manageable.

---

## New result: `A(16,8)`

Command:

```bash
./bin/validate_extra_cut 16 8
```

Output summary, (commit `5c5c68e0fcaac8318e15e4e77c31d75c2e29ab16`):

```shell
$ time ./bin/validate_extra_cut 16 8
Building A(16,8)...
Total valid vertices: 518918400
R = 65
Candidate g = 64
|S| = 65
Subset S connectivity verified.
|N(S)| = 2016
Validating 64-extra cut properties...
component sizes after deletion:
  65
  518916319
valid 64-extra cut: yes
therefore kappa_64(A(16,8)) <= 2016
Hamming baseline: 2417; Star boundary: 2016
Embedding gate: d = 7, k = 8, n-k = 8 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound


real    3m59.012s
user    3m55.815s
sys     0m2.316s


$ time ./bin/validate_extra_cut 17 8
Building A(17,8)...
Total valid vertices: 980179200
R = 73
Candidate g = 72
|S| = 73
Subset S connectivity verified.
|N(S)| = 2520
Validating 72-extra cut properties...
component sizes after deletion:
  73
  980176607
valid 72-extra cut: yes
therefore kappa_72(A(17,8)) <= 2520
Hamming baseline: 3088; Star boundary: 2520
Embedding gate: d = 7, k = 8, n-k = 9 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound


real    7m52.232s
user    7m45.927s
sys     0m4.228s


$ time ./bin/validate_extra_cut 18 8
Building A(18,8)...
Total valid vertices: 1764322560
R = 81
Candidate g = 80
|S| = 81
Subset S connectivity verified.
|N(S)| = 3080
Validating 80-extra cut properties...
component sizes after deletion:
  81
  1764319399
valid 80-extra cut: yes
therefore kappa_80(A(18,8)) <= 3080
Hamming baseline: 3782; Star boundary: 3080
Embedding gate: d = 7, k = 8, n-k = 10 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound


real    14m17.189s
user    14m5.706s
sys     0m7.730s


$ ./bin/validate_extra_cut_ranked 15 9
Building rank-indexed A(15,9)...
Total valid vertices: 1816214400
Visited bitset: 227026800 bytes
R = 55
Candidate g = 54
Subset S connectivity verified.
|S| = 55
|N(S)| = 1512
Validating 54-extra cut properties...
BFS progress: 1816212833 / 1816212833 (100.0%)
component sizes after deletion:
  55
  1816212833
valid 54-extra cut: yes
therefore kappa_54(A(15,9)) <= 1512
Hamming baseline: 1894; Star boundary: 1512
Embedding gate: d = 6, k = 9, n-k = 6 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound


$ ./bin/validate_extra_cut_ranked 21 7
Building rank-indexed A(21,7)...
Total valid vertices: 586051200
Visited bitset: 73256400 bytes
R = 99
Candidate g = 98
Subset S connectivity verified.
|S| = 99
|N(S)| = 4410
Validating 98-extra cut properties...
BFS progress: 586046691 / 586046691 (100.0%)
component sizes after deletion:
  99
  586046691
valid 98-extra cut: yes
therefore kappa_98(A(21,7)) <= 4410
Hamming baseline: 4982; Star boundary: 4410
Embedding gate: d = 7, k = 7, n-k = 14 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound


$ ./bin/validate_extra_cut_bitmap 12 11  # gg@vps76
Building rank-indexed A(12,11)...
Total valid vertices: 479001600
Visited bitset: 59875200 bytes
R = 12
Candidate g = 11
Subset S connectivity verified.
|S| = 12
|N(S)| = 110
Validating 11-extra cut properties...
BFS progress: 479001478 / 479001478 (100.0%)
component sizes after deletion:
  12
  479001478
valid 11-extra cut: yes
therefore kappa_11(A(12,11)) <= 110
Hamming baseline: 88; Star boundary: 110
Embedding gate: d = 4, k = 11, n-k = 1 (closed)
SATISFIES HAMMING OPTIMALITY


$ time for i in $(seq 22 26); do time ./bin/validate_extra_cut_bitmap $i 7; done  # coffeelake
Building rank-indexed A(22,7)...
Total valid vertices: 859541760
Visited bitset: 107442720 bytes
R = 106
Candidate g = 105
Subset S connectivity verified.
|S| = 106
|N(S)| = 5040
Validating 105-extra cut properties...
BFS progress: 859536614 / 859536614 (100.0%)
component sizes after deletion:
  106
  859536614
valid 105-extra cut: yes
therefore kappa_105(A(22,7)) <= 5040
Hamming baseline: 5664; Star boundary: 5040
Embedding gate: d = 7, k = 7, n-k = 15 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound
real    15m32.037s
user    15m27.575s
sys     0m0.652s

Building rank-indexed A(23,7)...
Total valid vertices: 1235591280
Visited bitset: 154448910 bytes
R = 113
Candidate g = 112
Subset S connectivity verified.
|S| = 113
|N(S)| = 5712
Validating 112-extra cut properties...
BFS progress: 1235585455 / 1235585455 (100.0%)
component sizes after deletion:
  113
  1235585455
valid 112-extra cut: yes
therefore kappa_112(A(23,7)) <= 5712
Hamming baseline: 6315; Star boundary: 5712
Embedding gate: d = 7, k = 7, n-k = 16 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound
real    3m32.252s
user    12m2.083s
sys     0m1.294s

Building rank-indexed A(24,7)...
Total valid vertices: 1744364160
Visited bitset: 218045520 bytes
R = 120
Candidate g = 119
Subset S connectivity verified.
|S| = 120
|N(S)| = 6426
Validating 119-extra cut properties...
BFS progress: 1744357614 / 1744357614 (100.0%)
component sizes after deletion:
  120
  1744357614
valid 119-extra cut: yes
therefore kappa_119(A(24,7)) <= 6426
Hamming baseline: 6984; Star boundary: 6426
Embedding gate: d = 7, k = 7, n-k = 17 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound
real    4m45.784s
user    17m2.436s
sys     0m1.529s

Building rank-indexed A(25,7)...
Total valid vertices: 2422728000
Visited bitset: 302841000 bytes
R = 127
Candidate g = 126
Subset S connectivity verified.
|S| = 127
|N(S)| = 7182
Validating 126-extra cut properties...
BFS progress: 2422720691 / 2422720691 (100.0%)
component sizes after deletion:
  127
  2422720691
valid 126-extra cut: yes
therefore kappa_126(A(25,7)) <= 7182
Hamming baseline: 7617; Star boundary: 7182
Embedding gate: d = 7, k = 7, n-k = 18 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound
real    6m33.766s
user    23m33.747s
sys     0m2.240s

Building rank-indexed A(26,7)...
Total valid vertices: 3315312000
Visited bitset: 414414000 bytes
R = 134
Candidate g = 133
Subset S connectivity verified.
|S| = 134
|N(S)| = 7980
Validating 133-extra cut properties...
BFS progress: 3315303886 / 3315303886 (100.0%)
component sizes after deletion:
  134
  3315303886
valid 133-extra cut: yes
therefore kappa_133(A(26,7)) <= 7980
Hamming baseline: 8574; Star boundary: 7980
Embedding gate: d = 8, k = 7, n-k = 19 (closed)
SOFT COUNTEREXAMPLE: UniversalLowerBound only
real    9m11.720s
user    32m32.210s
sys     0m2.917s

real    39m35.560s
user    100m38.053s
sys     0m8.633s
$ time ./bin/validate_extra_cut_bitmap 27 7
Building rank-indexed A(27,7)...
Total valid vertices: 4475671200
Visited bitset: 559458900 bytes
R = 141
Candidate g = 140
Subset S connectivity verified.
|S| = 141
|N(S)| = 8820
Validating 140-extra cut properties...
BFS progress: 4475662239 / 4475662239 (100.0%)
component sizes after deletion:
  141
  4475662239
valid 140-extra cut: yes
therefore kappa_140(A(27,7)) <= 8820
Hamming baseline: 9550; Star boundary: 8820
Embedding gate: d = 8, k = 7, n-k = 20 (closed)
SOFT COUNTEREXAMPLE: UniversalLowerBound only

real    11m48.525s
user    43m51.140s
sys     0m1.920s

$ time ./bin/validate_extra_cut_bitmap 21 8
Building rank-indexed A(21,8)...
Total valid vertices: 8204716800
Visited bitset: 1025589600 bytes
R = 105
Candidate g = 104
Subset S connectivity verified.
|S| = 105
|N(S)| = 5096
Validating 104-extra cut properties...
BFS progress: 8204711599 / 8204711599 (100.0%)
component sizes after deletion:
  105
  8204711599
valid 104-extra cut: yes
therefore kappa_104(A(21,8)) <= 5096
Hamming baseline: 6188; Star boundary: 5096
Embedding gate: d = 7, k = 8, n-k = 13 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound

real    165m14.207s
user    163m38.736s
sys     0m7.611s
direnv: loading ~/Documents/school/ou-papers/program-cheng-connectivity-asymptote/.envrc
direnv: warning: proofs/.lake -> /run/media/shane/shane4tb-ent/.lake/program-cheng-connectivity-asymptote-proofs, not /path/to/some/device/.lake/program-cheng-connectivity-asymptote-proofs (not touching it)
direnv: export +LEAN_CACHE_DIR +OMP_DISPLAY_ENV +OMP_NUM_THREADS +ORTOOLS_CFLAGS +ORTOOLS_HOME +ORTOOLS_LIBS

$ ssh gg@nightly cat program-cheng-connectivity-asymptote/.tmp/star_A$n_k$k.log
nohup: ignoring input
Build: 0.1.0 (74cdb6fb2409)
Building rank-indexed A(19,8)...
Total valid vertices: 3047466240
Visited bitset: 380933280 bytes
R = 89
Candidate g = 88
Subset S connectivity verified.
|S| = 89
|N(S)| = 3696
Validating 88-extra cut properties...
BFS progress: 21921624 / 3047462455 (0.7%)
[Direction Optimized: Bottom-Up Scan Active]
BFS progress: 3047462455 / 3047462455 (100.0%)
component sizes after deletion:
  89
  3047462455
valid 88-extra cut: yes
therefore kappa_88(A(19,8)) <= 3696
Hamming baseline: 4538; Star boundary: 3696; Delta: 842
Embedding gate: d = 7, k = 8, n-k = 11 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound

$ ./bin/validate_extra_cut_bitmap 16 9

OPENMP DISPLAY ENVIRONMENT BEGIN
  _OPENMP = '202111'
  [host] OMP_DYNAMIC = 'FALSE'
  [host] OMP_NESTED = 'FALSE'
  [host] OMP_NUM_THREADS = '1'
  [host] OMP_SCHEDULE = 'DYNAMIC'
  [host] OMP_PROC_BIND = 'FALSE'
  [host] OMP_PLACES = ''
  [host] OMP_STACKSIZE = '0'
  [host] OMP_WAIT_POLICY = 'PASSIVE'
  [host] OMP_THREAD_LIMIT = '4294967295'
  [host] OMP_MAX_ACTIVE_LEVELS = '1'
  [host] OMP_NUM_TEAMS = '0'
  [host] OMP_TEAMS_THREAD_LIMIT = '0'
  [all] OMP_CANCELLATION = 'FALSE'
  [all] OMP_DEFAULT_DEVICE = '0'
  [all] OMP_MAX_TASK_PRIORITY = '0'
  [all] OMP_DISPLAY_AFFINITY = 'FALSE'
  [host] OMP_AFFINITY_FORMAT = 'level %L thread %i affinity %A'
  [host] OMP_ALLOCATOR = 'omp_default_mem_alloc'
  [all] OMP_TARGET_OFFLOAD = 'DEFAULT'
OPENMP DISPLAY ENVIRONMENT END
Build: 0.1.0 (37e365e808eb)
Building rank-indexed A(16,9)...
Total valid vertices: 4151347200
Visited bitset: 518918400 bytes
R = 64
Candidate g = 63
Subset S connectivity verified.
|S| = 64
|N(S)| = 2016
Validating 63-extra cut properties...
BFS progress: 64051674 / 4151345120 (1.5%)
[Direction Optimized: Bottom-Up Scan Active]
Bottom-up scan: 64864800 / 64864800 words (100.0%)
Bottom-up scan: 64864800 / 64864800 words (100.0%)
Bottom-up scan: 64864800 / 64864800 words (100.0%)
Bottom-up scan: 64864800 / 64864800 words (100.0%)
BFS progress: 4151345120 / 4151345120 (100.0%)
component sizes after deletion:
  64
  4151345120
valid 63-extra cut: yes
therefore kappa_63(A(16,9)) <= 2016
Hamming baseline: 2496; Star boundary: 2016; Delta: 480
Embedding gate: d = 6, k = 9, n-k = 7 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound

$ ./bin/validate_extra_cut_bitmap 13 11

OPENMP DISPLAY ENVIRONMENT BEGIN
  _OPENMP = '202111'
  [host] OMP_DYNAMIC = 'FALSE'
  [host] OMP_NESTED = 'FALSE'
  [host] OMP_NUM_THREADS = '1'
  [host] OMP_SCHEDULE = 'DYNAMIC'
  [host] OMP_PROC_BIND = 'FALSE'
  [host] OMP_PLACES = ''
  [host] OMP_STACKSIZE = '0'
  [host] OMP_WAIT_POLICY = 'PASSIVE'
  [host] OMP_THREAD_LIMIT = '4294967295'
  [host] OMP_MAX_ACTIVE_LEVELS = '1'
  [host] OMP_NUM_TEAMS = '0'
  [host] OMP_TEAMS_THREAD_LIMIT = '0'
  [all] OMP_CANCELLATION = 'FALSE'
  [all] OMP_DEFAULT_DEVICE = '0'
  [all] OMP_MAX_TASK_PRIORITY = '0'
  [all] OMP_DISPLAY_AFFINITY = 'FALSE'
  [host] OMP_AFFINITY_FORMAT = 'level %L thread %i affinity %A'
  [host] OMP_ALLOCATOR = 'omp_default_mem_alloc'
  [all] OMP_TARGET_OFFLOAD = 'DEFAULT'
OPENMP DISPLAY ENVIRONMENT END
Build: 0.1.0 (74cdb6fb2409)
Building rank-indexed A(13,11)...
Total valid vertices: 3113510400
Visited bitset: 389188800 bytes
R = 23
Candidate g = 22
Subset S connectivity verified.
|S| = 23
|N(S)| = 330
Validating 22-extra cut properties...
BFS progress: 85608949 / 3113510047 (2.7%)
[Direction Optimized: Bottom-Up Scan Active]
Bottom-up scan: 48648600 / 48648600 words (100.0%)
Bottom-up scan: 48648600 / 48648600 words (100.0%)
Bottom-up scan: 48648600 / 48648600 words (100.0%)
Bottom-up scan: 48648600 / 48648600 words (100.0%)
Bottom-up scan: 48648600 / 48648600 words (100.0%)
BFS progress: 3113510047 / 3113510047 (100.0%)
component sizes after deletion:
  23
  3113510047
valid 22-extra cut: yes
therefore kappa_22(A(13,11)) <= 330
Hamming baseline: 352; Star boundary: 330; Delta: 22
Embedding gate: d = 5, k = 11, n-k = 2 (closed)
SOFT COUNTEREXAMPLE: UniversalLowerBound only

$ time ./bin/validate_extra_cut_bitmap 13 12

OPENMP DISPLAY ENVIRONMENT BEGIN
  _OPENMP = '202111'
  [host] OMP_DYNAMIC = 'FALSE'
  [host] OMP_NESTED = 'FALSE'
  [host] OMP_NUM_THREADS = '1'
  [host] OMP_SCHEDULE = 'DYNAMIC'
  [host] OMP_PROC_BIND = 'FALSE'
  [host] OMP_PLACES = ''
  [host] OMP_STACKSIZE = '0'
  [host] OMP_WAIT_POLICY = 'PASSIVE'
  [host] OMP_THREAD_LIMIT = '4294967295'
  [host] OMP_MAX_ACTIVE_LEVELS = '1'
  [host] OMP_NUM_TEAMS = '0'
  [host] OMP_TEAMS_THREAD_LIMIT = '0'
  [all] OMP_CANCELLATION = 'FALSE'
  [all] OMP_DEFAULT_DEVICE = '0'
  [all] OMP_MAX_TASK_PRIORITY = '0'
  [all] OMP_DISPLAY_AFFINITY = 'FALSE'
  [host] OMP_AFFINITY_FORMAT = 'level %L thread %i affinity %A'
  [host] OMP_ALLOCATOR = 'omp_default_mem_alloc'
  [all] OMP_TARGET_OFFLOAD = 'DEFAULT'
OPENMP DISPLAY ENVIRONMENT END
Build: 0.1.0 (37e365e808eb)
Building rank-indexed A(13,12)...
Total valid vertices: 6227020800
Visited bitset: 778377600 bytes
Storage: RAM
R = 13
Candidate g = 12
Subset S connectivity verified.
|S| = 13
|N(S)| = 132
Validating 12-extra cut properties...
BFS progress: 131131709 / 6227020655 (2.1%)
[Direction Optimized: Bottom-Up Scan Active]
Bottom-up scan: 97297200 / 97297200 words (100.0%)
Bottom-up scan: 97297200 / 97297200 words (100.0%)
Bottom-up scan: 97297200 / 97297200 words (100.0%)
Bottom-up scan: 97297200 / 97297200 words (100.0%)
Bottom-up scan: 97297200 / 97297200 words (100.0%)
BFS progress: 6227020655 / 6227020655 (100.0%)
component sizes after deletion:
  13
  6227020655
valid 12-extra cut: yes
therefore kappa_12(A(13,12)) <= 132
Hamming baseline: 107; Star boundary: 132; Delta: 18446744073709551591
Embedding gate: d = 4, k = 12, n-k = 1 (closed)
SATISFIES HAMMING OPTIMALITY

real    129m43.141s
user    129m30.498s
sys     0m1.319s

$ time ./bin/validate_extra_cut_bitmap 16 8 --disk-backed ./state_A16_8

OPENMP DISPLAY ENVIRONMENT BEGIN
  _OPENMP = '202111'
  [host] OMP_DYNAMIC = 'FALSE'
  [host] OMP_NESTED = 'FALSE'
  [host] OMP_NUM_THREADS = '1'
  [host] OMP_SCHEDULE = 'DYNAMIC'
  [host] OMP_PROC_BIND = 'FALSE'
  [host] OMP_PLACES = ''
  [host] OMP_STACKSIZE = '0'
  [host] OMP_WAIT_POLICY = 'PASSIVE'
  [host] OMP_THREAD_LIMIT = '4294967295'
  [host] OMP_MAX_ACTIVE_LEVELS = '1'
  [host] OMP_NUM_TEAMS = '0'
  [host] OMP_TEAMS_THREAD_LIMIT = '0'
  [all] OMP_CANCELLATION = 'FALSE'
  [all] OMP_DEFAULT_DEVICE = '0'
  [all] OMP_MAX_TASK_PRIORITY = '0'
  [all] OMP_DISPLAY_AFFINITY = 'FALSE'
  [host] OMP_AFFINITY_FORMAT = 'level %L thread %i affinity %A'
  [host] OMP_ALLOCATOR = 'omp_default_mem_alloc'
  [all] OMP_TARGET_OFFLOAD = 'DEFAULT'
OPENMP DISPLAY ENVIRONMENT END
Build: 0.1.0 (37e365e808eb)
Building rank-indexed A(16,8)...
Total valid vertices: 518918400
Visited bitset: 64864800 bytes
Storage: disk-backed (mmap)
R = 65
Candidate g = 64
Subset S connectivity verified.
|S| = 65
|N(S)| = 2016
Validating 64-extra cut properties...
BFS progress: 5897904 / 518916319 (1.1%)
[Direction Optimized: Bottom-Up Scan Active]
Bottom-up scan: 8108100 / 8108100 words (100.0%)
Bottom-up scan: 8108100 / 8108100 words (100.0%)
Bottom-up scan: 8108100 / 8108100 words (100.0%)
Bottom-up scan: 8108100 / 8108100 words (100.0%)
BFS progress: 518916319 / 518916319 (100.0%)
component sizes after deletion:
  65
  518916319
valid 64-extra cut: yes
therefore kappa_64(A(16,8)) <= 2016
Hamming baseline: 2417; Star boundary: 2016; Delta: 401
Embedding gate: d = 7, k = 8, n-k = 8 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound

real    13m19.257s
user    13m15.790s
sys     0m0.723s

$ time ./bin/validate_extra_cut_bitmap 16 9 --disk-backed ./state_A16_9  # coffeelake

OPENMP DISPLAY ENVIRONMENT BEGIN
  _OPENMP = '202111'
  [host] OMP_DYNAMIC = 'FALSE'
  [host] OMP_NESTED = 'FALSE'
  [host] OMP_NUM_THREADS = '1'
  [host] OMP_SCHEDULE = 'DYNAMIC'
  [host] OMP_PROC_BIND = 'FALSE'
  [host] OMP_PLACES = ''
  [host] OMP_STACKSIZE = '0'
  [host] OMP_WAIT_POLICY = 'PASSIVE'
  [host] OMP_THREAD_LIMIT = '4294967295'
  [host] OMP_MAX_ACTIVE_LEVELS = '1'
  [host] OMP_NUM_TEAMS = '0'
  [host] OMP_TEAMS_THREAD_LIMIT = '0'
  [all] OMP_CANCELLATION = 'FALSE'
  [all] OMP_DEFAULT_DEVICE = '0'
  [all] OMP_MAX_TASK_PRIORITY = '0'
  [all] OMP_DISPLAY_AFFINITY = 'FALSE'
  [host] OMP_AFFINITY_FORMAT = 'level %L thread %i affinity %A'
  [host] OMP_ALLOCATOR = 'omp_default_mem_alloc'
  [all] OMP_TARGET_OFFLOAD = 'DEFAULT'
OPENMP DISPLAY ENVIRONMENT END
Build: 0.1.0 (37e365e808eb)
Building rank-indexed A(16,9)...
Total valid vertices: 4151347200
Visited bitset: 518918400 bytes
Storage: disk-backed (mmap)
R = 64
Candidate g = 63
Subset S connectivity verified.
|S| = 64
|N(S)| = 2016
Validating 63-extra cut properties...
BFS progress: 64051674 / 4151345120 (1.5%)
[Direction Optimized: Bottom-Up Scan Active]
Bottom-up scan: 64864800 / 64864800 words (100.0%)
Bottom-up scan: 64864800 / 64864800 words (100.0%)
Bottom-up scan: 64864800 / 64864800 words (100.0%)
Bottom-up scan: 64864800 / 64864800 words (100.0%)
BFS progress: 4151345120 / 4151345120 (100.0%)
component sizes after deletion:
  64
  4151345120
valid 63-extra cut: yes
therefore kappa_63(A(16,9)) <= 2016
Hamming baseline: 2496; Star boundary: 2016; Delta: 480
Embedding gate: d = 6, k = 9, n-k = 7 (open)
HARD COUNTEREXAMPLE: RestrictedLowerBound

real    118m17.819s
user    117m34.612s
sys     0m8.120s

# shane@coffeelake:~/Documents/school/ou-papers/program-cheng-connectivity-asymptote
$ time ./bin/validate_extra_cut_bitmap 15 10 --disk-backed ./state_A15_10

OPENMP DISPLAY ENVIRONMENT BEGIN
  _OPENMP = '202111'
  [host] OMP_DYNAMIC = 'FALSE'
  [host] OMP_NESTED = 'FALSE'
  [host] OMP_NUM_THREADS = '1'
  [host] OMP_SCHEDULE = 'DYNAMIC'
  [host] OMP_PROC_BIND = 'FALSE'
  [host] OMP_PLACES = ''
  [host] OMP_STACKSIZE = '0'
  [host] OMP_WAIT_POLICY = 'PASSIVE'
  [host] OMP_THREAD_LIMIT = '4294967295'
  [host] OMP_MAX_ACTIVE_LEVELS = '1'
  [host] OMP_NUM_TEAMS = '0'
  [host] OMP_TEAMS_THREAD_LIMIT = '0'
  [all] OMP_CANCELLATION = 'FALSE'
  [all] OMP_DEFAULT_DEVICE = '0'
  [all] OMP_MAX_TASK_PRIORITY = '0'
  [all] OMP_DISPLAY_AFFINITY = 'FALSE'
  [host] OMP_AFFINITY_FORMAT = 'level %L thread %i affinity %A'
  [host] OMP_ALLOCATOR = 'omp_default_mem_alloc'
  [all] OMP_TARGET_OFFLOAD = 'DEFAULT'
OPENMP DISPLAY ENVIRONMENT END
Build: 0.1.0 (draft-2026-07-21-emailed-laszlo-eddie-435-g2b77f21b7db7-dirty)
Building rank-indexed A(15,10)...
Total valid vertices: 10897286400
Visited bitset: 1362160800 bytes
Storage: disk-backed (mmap)
R = 51
Candidate g = 50
Subset S connectivity verified.
|S| = 51
|N(S)| = 1350
Validating 50-extra cut properties...
BFS progress: 242374803 / 10897284999 (2.2%)
[Direction Optimized: Bottom-Up Scan Active]
Bottom-up scan: [layer 9/?] 170270100 / 170270100 words (100.0%)
Bottom-up scan: [layer 10/?] 170270100 / 170270100 words (100.0%)
Bottom-up scan: [layer 11/?] 170270100 / 170270100 words (100.0%)
Bottom-up scan: [layer 12/?] 170270100 / 170270100 words (100.0%)
BFS progress: 10897284999 / 10897284999 (100.0%)
component sizes after deletion:
  51
  10897284999
valid 50-extra cut: yes
therefore kappa_50(A(15,10)) <= 1350
Hamming baseline: 1713; Star boundary: 1350; Delta: 363
Embedding gate: d = 6, k = 10, n-k = 5 (closed)
SOFT COUNTEREXAMPLE: UniversalLowerBound only

real    313m38.746s
user    310m15.734s
sys     0m31.582s
```

The comparison gap is `2417 - 2016 = 401`. Therefore the Star gives

```text
kappa_64(A(16,8)) <= 2016
```

and refutes the embedding-gated Hamming lower-bound hypothesis at
`(R,n,k) = (65,16,8)`. This isn't necessarily the lowest possible bound.

For `A(17,8)`, the comparison gap is `3088 - 2520 = 568`, giving

```text
kappa_72(A(17,8)) <= 2520
```

at `(R,n,k) = (73,17,8)`.

For `A(18,8)`, the comparison gap is `3782 - 3080 = 702`, giving

```text
kappa_80(A(18,8)) <= 3080
```

at `(R,n,k) = (81,18,8)`.

For `A(25,7)`, the comparison gap is `7617 - 7182 = 435`, giving

```text
kappa_126(A(25,7)) <= 7182
```

at `(R,n,k) = (127,25,7)`. This is the largest deeply certified hard
counterexample in the current sweep by valid-vertex count.

## Fixed-`k` pattern

For fixed `k`, `R-1 = k(n-k)`. For `k=6`, the gate is open through `n=16` and
closes at `n=17`, so the full-Star family gives hard counterexamples through
`A(16,6)` and soft counterexamples thereafter when the Star remains below the
Hamming baseline.

For `k=7`, `A(17,7)` has `m=10`, `R-1=70`, and `d=7`, so both gate inequalities
hold. Its Hamming gap is `2658 - 2310 = 348`. The later `A(19,7)` run has
`m=12`, `R-1=84`, and gap `3783 - 3276 = 507`. The arithmetic gate remains open
while `7(n-7) < 2^7`, subject to the validator being able to construct the
graph.

## Reproduction commands

One log file per `n`, with progress and validator output together:

```bash
mkdir -p .tmp
for n in 8 9 10 11 12 13; do
    ./bin/validate_extra_cut "$n" 1 2>&1 \
        | tee ".tmp/star_A${n}.txt"
    for k in $(seq 2 $((n - 1))); do
        printf '\n' | tee -a ".tmp/star_A${n}.txt"
        ./bin/validate_extra_cut "$n" "$k" 2>&1 \
            | tee -a ".tmp/star_A${n}.txt"
    done
done
```

For a larger fixed-`k` sweep:

```bash
for n in 14 15 16 17 18 19; do
    ./bin/validate_extra_cut "$n" 6 2>&1 \
        | tee ".tmp/star_A${n}_k6.txt"
done

./bin/validate_extra_cut 19 7 2>&1 \
    | tee ".tmp/star_A19_k7.txt"
```

The validator's flat code-space guard is a computational resource limit, not a
mathematical claim about instances it rejects.
