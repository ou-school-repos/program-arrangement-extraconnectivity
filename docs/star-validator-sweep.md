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

## Sweep map

```shell
time for i in $(seq 11 14); do for j in $(seq 9 10); do ./bin/validate_extra_cut $i $j; done done
```

Each `(n,k)` cell tests the full radius-one Star at $R = 1 + k(n-k)$, with
$g = R - 1$. This is one selected volume per graph parameter pair, not a sweep
over arbitrary `R` values.

Legend:

- `■` — hard counterexample to `RestrictedLowerBound`.
- `⊞` — soft counterexample to `UniversalLowerBound` only.
- `∘` — satisfies the Hamming comparison.
- `\` — invalid extra cut or outside `k < n`.
- `/` — not yet run or thus far intractable.

| `n \ k` |  3  |  4  |  5  |  6  |  7  |  8  |  9  | 10  |
| ------: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
|       4 |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  | \\  |
|       5 |  ∘  |  ∘  | \\  | \\  | \\  | \\  | \\  | \\  |
|       6 |  ∘  |  ∘  |  ∘  | \\  | \\  | \\  | \\  | \\  |
|       7 |  ∘  |  ∘  |  ∘  |  ∘  | \\  | \\  | \\  | \\  |
|       8 |  ∘  |  ∘  |  ∘  |  ∘  |  ∘  | \\  | \\  | \\  |
|       9 |  ∘  |  ∘  |  ∘  |  ⊞  |  ∘  |  ∘  | \\  | \\  |
|      10 |  ∘  |  ∘  |  ∘  |  ⊞  |  ⊞  |  ⊞  |  ∘  | \\  |
|      11 |  ∘  |  ∘  |  ∘  |  ■  |  ⊞  |  ⊞  |  ⊞  |  ∘  |
|      12 |  ∘  |  ∘  |  ∘  |  ■  |  ⊞  |  ⊞  |  ⊞  |  ⊞  |
|      13 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ⊞  |  ⊞  |  /  |
|      14 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ■  |  ⊞  |  /  |
|      15 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ■  |  ■  |  /  |
|      16 |  ∘  |  ∘  |  ∘  |  ■  |  ■  |  ■  |  /  |  /  |
|      17 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  ■  |  /  |  /  |
|      18 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  ■  |  /  |  /  |
|      19 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  /  |  /  |  /  |
|      20 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  /  |  /  |  /  |
|      21 |  ∘  |  ∘  |  ∘  |  ⊞  |  ■  |  /  |  /  |  /  |
|      22 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      23 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      24 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      25 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      26 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      27 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      28 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      29 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      30 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      31 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |
|      32 |  ∘  |  ∘  |  ∘  |  ∘  |  /  |  /  |  /  |  /  |

`time for i in $(seq 21 26); do for j in $(seq 1 6); do ./bin/validate_extra_cut $i $j; done done`

The smallest known hard counterexample, and the smallest hard counterexample
within the full-Star family, is `A(11,6)` with `R=31`. This does not prove that
no non-Star topology gives a smaller hard counterexample.

The `A(11,9)` case is now deeply confirmed as a soft counterexample after
raising the flat-space guard. The complete `A(14, 1..8)` row is now confirmed.
The feasible `A(15, 1..8)`, `A(16, 1..8)`, `A(17, 1..8)`, and `A(18, 1..8)`
cells are also confirmed; their remaining cells are marked `/` because they were
not run or were intractable. The `A(19,1..7)` and `A(20,1..7)` rows are now
confirmed; their remaining cells are likewise marked `/`.

The largest deeply certified hard counterexample so far is `A(18,8)`, with
`R=81`, `g=80`, and `|N(S)|=3080` (~14 minutes to compute).

## Representative hard counterexamples

Sorted by `k`, then valid-vertex count `|V| = nP_k`. The graph edge count is
$|E| = |V| * k (n-k) / 2$. Here $|∂S| = |N(S)|$ is the measured **Star
boundary**, while $|∂H|$ denotes the **Hamming baseline** for volume `R`.

| graph     |         \|V\| |          \|E\| |          `n^k` | `R` | `g` | \|∂S\| | \|∂H\| | `Δ` | `d,(k,n-k)` |
| --------- | ------------: | -------------: | -------------: | --: | --: | -----: | -----: | --: | :---------- |
| `A(11,6)` |       332,640 |      4,989,600 |      1,771,561 |  31 |  30 |    450 |    476 |  26 | `5,(6,5)`   |
| `A(12,6)` |       665,280 |     11,975,040 |      2,985,984 |  37 |  36 |    630 |    687 |  57 | `6,(6,6)`   |
| `A(13,6)` |     1,235,520 |     25,945,920 |      4,826,809 |  43 |  42 |    840 |    921 |  81 | `6,(6,7)`   |
| `A(14,6)` |     2,162,160 |     51,891,840 |      7,529,536 |  49 |  48 |  1,080 |  1,163 |  83 | `6,(6,8)`   |
| `A(15,6)` |     3,603,600 |     97,297,200 |     11,390,625 |  55 |  54 |  1,350 |  1,441 |  91 | `6,(6,9)`   |
| `A(16,6)` |     5,765,760 |    172,972,800 |     16,777,216 |  61 |  60 |  1,650 |  1,713 |  63 | `6,(6,10)`  |
| **k=7**   |               |                |                |     |     |        |        |     |             |
| `A(13,7)` |     8,648,640 |    181,621,440 |     62,748,517 |  43 |  42 |    882 |  1,029 | 147 | `6,(7,6)`   |
| `A(14,7)` |    17,297,280 |    423,783,360 |    105,413,504 |  50 |  49 |  1,176 |  1,366 | 190 | `6,(7,7)`   |
| `A(15,7)` |    32,432,400 |    908,107,200 |    170,859,375 |  57 |  56 |  1,512 |  1,744 | 232 | `6,(7,8)`   |
| `A(16,7)` |    57,657,600 |  1,816,214,400 |    268,435,456 |  64 |  63 |  1,890 |  2,112 | 222 | `6,(7,9)`   |
| `A(17,7)` |    98,017,920 |  3,430,627,200 |    410,338,673 |  71 |  70 |  2,310 |  2,658 | 348 | `7,(7,10)`  |
| `A(18,7)` |   160,392,960 |  6,175,128,960 |    612,220,032 |  78 |  77 |  2,772 |  3,200 | 428 | `7,(7,11)`  |
| `A(19,7)` |   253,955,520 | 10,666,131,840 |    893,871,739 |  85 |  84 |  3,276 |  3,783 | 507 | `7,(7,12)`  |
| `A(20,7)` |   390,700,800 | 17,776,886,400 |  1,280,000,000 |  92 |  91 |  3,822 |  4,356 | 534 | `7,(7,13)`  |
| `A(21,7)` |   586,051,200 | 15,823,382,400 |  1,801,088,541 |  99 |  98 |  4,410 |  4,982 | 572 | `7,(14,7)`  |
| **k=8**   |               |                |                |     |     |        |        |     |             |
| `A(14,8)` |   121,080,960 |  2,905,943,040 |  1,475,789,056 |  49 |  48 |  1,176 |  1,423 | 247 | `6,(8,6)`   |
| `A(15,8)` |   259,459,200 |  7,264,857,600 |  2,562,890,625 |  57 |  56 |  1,568 |  1,903 | 335 | `6,(8,7)`   |
| `A(16,8)` |   518,918,400 | 16,605,388,800 |  4,294,967,296 |  65 |  64 |  2,016 |  2,417 | 401 | `7,(8,8)`   |
| `A(17,8)` |   980,179,200 | 35,286,451,200 |  6,975,757,441 |  73 |  72 |  2,520 |  3,088 | 568 | `7,(8,9)`   |
| `A(18,8)` | 1,764,322,560 | 70,572,902,400 | 11,019,960,576 |  81 |  80 |  3,080 |  3,782 | 702 | `7,(8,10)`  |
| **k=9**   |               |                |                |     |     |        |        |     |             |
| `A(15,9)` | 1,816,214,400 | 49,037,788,800 | 38,443,359,375 |  55 |  54 |  1,512 |  1,894 | 382 | `6,(9,6)`   |

## TODOs

For the ranked backend, the best next target is below.

### 1. A(13,10)

- (n^k = 13^{10}=13,785,849,184) — impossible flat
- (V=13P\_{10}=1,037,836,800)
- Visited bitset: ~124 MiB
- Degree: (10(3)=30)
- Directed neighbor visits: ~31.1 billion
- Estimated runtime: 10–20 minutes

This is probably the best balance of low memory and useful coverage.

### 2. A(21,7)

- (n^k=1,801,088,541)
- (V=586,051,200)
- Visited bitset: ~70 MiB
- Directed neighbor visits: ~57.4 billion
- Estimated runtime: 20–30 minutes

Less memory, but more neighbor work.

### 3. A(15,9)

- (V=1,816,214,400)
- Visited bitset: ~216 MiB
- Directed neighbor visits: ~98.1 billion
- Estimated runtime: 25–40 minutes

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

at `(R,n,k) = (81,18,8)`. This is the largest deeply certified hard
counterexample in the current sweep.

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
