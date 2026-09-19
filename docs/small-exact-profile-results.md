# Small exact boundary-profile results

This note records the small arrangement-graph vertex-boundary profiles computed
so far. For a graph (A(n,k)), write

\[ \Phi\_{n,k}(R)=\min\{|N(S)|:S\subseteq V(A(n,k)),\ |S|=R\}, \]

where (N(S)) is the external vertex boundary. The table entries below are
computational results, not Lean theorems.

## Profile values

The entries are reported as exact unrestricted values, using exhaustive
enumeration of sets connected in the distance-at-most-two graph and a separate
check of the possible disconnected splits.

| Graph    | (m=n-k) | (R) values | (Phi\_{n,k}(R)), in order       | Hamming-ball witness available and sharp |
| -------- | ------: | ---------: | ------------------------------- | ---------------------------------------- |
| (A(6,3)) |       3 |       1--7 | 9, 14, 18, 20, 23, 24, 25       | Yes, all listed (R)                      |
| (A(5,3)) |       2 |       1--7 | 6, 9, 11, 12, 14, 15, **17**    | Yes, (R=1\)--4                           |
| (A(5,4)) |       1 |       1--8 | 4, 6, 8, 10, **11**, 12, 14, 16 | Yes, (R=1\)--2                           |
| (A(6,4)) |       2 |       1--6 | 8, 13, 17, 20, 24, 27           | Yes, (R=1\)--4                           |

“Hamming-ball witness available” means the stated Boolean-cube embedding gate
holds; “sharp” means the computed profile equals the boundary of that witness.
Outside the gate, the formal Hamming expression is only a number, not an
available construction. Bold entries are below the radius-one Star boundary at
the Star's volume.

## Star-volume comparisons and observed shapes

The radius-one Star has volume (R\_\*=1+k(n-k)) and boundary \[
|N(B_1(v))|=\binom{k}{2}(n-k)(n-k+1). \]

| Graph    | Star volume (R\_\*) | Computed profile | Star boundary | Observation                                                      |
| -------- | ------------------: | ---------------: | ------------: | ---------------------------------------------------------------- |
| (A(5,3)) |                   7 |           **17** |            18 | Two hybrid types attain 17; one is a folded-cube-plus-line shape |
| (A(5,4)) |                   5 |           **11** |            12 | A 5-vertex path in a copy of (A(3,2)\cong C_6)                   |
| (A(6,4)) |                   9 |     Not computed |            36 | Star optimality at this volume is open computationally           |
| (A(6,3)) |                  10 |     Not computed |            36 | Star optimality at this volume is open computationally           |

Further observed minimizers in (A(6,4)):

| (R) | Boundary | (D) | (X) | Symbol support (s) | Shape note                                                                        |
| --: | -------: | --: | --: | -----------------: | --------------------------------------------------------------------------------- |
|   5 |       24 |   5 |   1 |                  6 | Witness is a budget-limited cube-like configuration                               |
|   6 |       27 |   7 |   0 |                  6 | Folded cube: a (2\times2\times2) product with the two non-injective words omitted |

Here (D=Rk-U) is the root defect, (X) is the collision excess, and (s) is the
number of alphabet symbols used by the set. These are observed witness
statistics, not a classification of all minimizers.

For (A(6,4)), the formal Hamming values at (R=5,6) are 23 and 25, respectively,
while the computed boundaries are 24 and 27. The embedding gate is closed in
both cases, so neither formal value is a Hamming-ball witness.

## How disconnected sets are accounted for

Connect two vertices when their distance in the arrangement graph is at most
two, and consider the connected components of the induced distance-two graph on
(S). Distinct components are at graph distance at least three. Their external
boundaries are disjoint and avoid the other components, hence \[ |N(S)|=\sum_i
|N(C_i)|. \]

Let \(\psi*{n,k}(r)\) be the minimum boundary among (r\)-sets connected in this
distance-two graph. For a disconnected (S) with component sizes
\(r_1,\ldots,r_t\), \[ |N(S)|\geq \sum_i \psi*{n,k}(r_i). \] Thus a
disconnected-split calculation gives a lower bound on the unrestricted profile.
It is not automatically an equality: independently minimizing components need
not be placeable at mutual distance at least three. To certify that a
distance-two-connected minimizer is also unrestricted, it suffices to check that
every nontrivial integer split has lower-bound sum greater than or equal to that
minimizer's boundary.

For example, the (A(6,4)) values at (R=1,\ldots,5) are \(8,13,17,20,24\). At
(R=6), the least two-part split lower bound is \(8+24=32\) (the other two-part
possibilities give 33 and 34); splits into more parts are no smaller. Since the
distance-two-connected minimum is 27, this certifies \(\Phi\_{6,4}(6)=27\)
against disconnected sets as well.

## Enumeration and reproducibility notes

[`src/exact_profile_d2.cpp`](../src/exact_profile_d2.cpp) exhaustively
enumerates distance-two-connected sets of one requested size, rooted at a fixed
vertex using vertex transitivity. It reports the minimum, one witness, and its
((D,X,s)) statistics. It does **not** itself perform the split calculation
above. The program currently supports graphs with at most 512 vertices; this is
a storage limit, not a runtime guarantee. The search remains exponential in the
requested volume.

Reported runs include:

| Case          | Enumerated distance-two-connected sets |        Runtime | Minimum boundary |
| ------------- | -------------------------------------: | -------------: | ---------------: |
| (A(5,3), R=7) |                             30,372,391 |        1.471 s |               17 |
| (A(6,4), R=5) |                              5,832,295 |        0.884 s |               24 |
| (A(6,4), R=6) |                            299,133,660 |       41.851 s |               27 |
| (A(6,3), R=7) |                          1,903,137,054 |      134.642 s |               25 |

Representative commands:

```sh
./bin/exact_profile_d2 5 3 7
./bin/exact_profile_d2 6 4 5
./bin/exact_profile_d2 6 4 6
```

The (A(6,4),R=7) run was interrupted, and (A(6,4),R=9) and (A(6,3),R=10) have
not been computed. These are the Star-volume cases of particular interest in
this small collection.
