# Distance-two pattern sweeps

These C++ tools enumerate candidate boundary signatures and evaluate their
profiles. A completed catalogue gives the exact minimum over sets connected in
the arrangement graph's distance-at-most-two graph. The split sweep then checks
whether integer-partition lower bounds already rule out a better disconnected
set. It does not construct tight component placements.

## 1. Canonical pattern catalogue

Build and run:

```sh
make bin/d2_pattern_catalogue
./bin/d2_pattern_catalogue 4 --all-patterns /tmp/d2-r4-classes.txt \
  > /tmp/d2-r4-signatures.txt
```

The catalogue canonicalizes the four-partite row-coordinate-symbol-cell
incidence graph with nauty. It adds rows only at arrangement distance one or
two; for Hamming distance two it explicitly checks whether at least one of the
two coordinatewise intermediate words is injective. Thus blocked swaps do not
count as distance-two edges.

Canonical augmentation uses an isomorphism-invariant parent: among row deletions
that leave the pattern distance-two connected, choose the deletion whose
remaining pattern has the lexicographically least nauty canonical key. Accept a
child only when that parent key matches the class currently being expanded. A
connected pattern has a non-cut row, so this rule gives every isomorphism class
a parent. Duplicate child keys are retained once per level.

The default state limit is 100,000 retained classes at a level. Reaching it
returns status 3 and marks the run incomplete; raise it with `--max-states N`
only when memory allows. `--all-patterns PATH` writes one canonical row-matrix
representative per class. The standard output groups classes by `(D,X,p,e)` and
retains one representative per signature.

Here `p` is the number of varying coordinates, `e=s_a-p`, and
`D=R*p-sum_i |projection_i(S)|`. Constant coordinates contribute `R` roots each,
so this is also the full defect `R*k-U` after embedding. `X` is the
cross-collision excess; it is computed from active coordinates and symbols. The
embedding budgets are `p <= k` and `e <= m`, where `m=n-k`.

The completeness bound used by the generator is `p <= 2(R-1)`: take a
distance-two spanning tree and charge at most two changed coordinates to each
tree edge. The symbol bound is obtained by allowing two new symbols per tree
edge, plus distinct fillers for constant coordinates.

## 2. Envelope sweep

For a completed signature file, evaluate all budgets for a fixed `R` and `k`:

```sh
make bin/envelope_sweep
./bin/envelope_sweep /tmp/d2-r4-signatures.txt 4 3 8
```

The arguments after the file are `R`, `k`, and the maximum slack `m`. The
boundary identity is

```text
|N(S)| = (R*k-D)*m - D - X.
```

The output lists all tied minimum signatures and whether a minimizer has maximum
feasible defect and maximum collision excess within that defect.

## 3. Split sweep

Prepare a two-column table with one exact distance-two-connected profile per
volume for the same graph:

```text
1 9
2 14
3 18
4 20
```

Then compare the connected profile at volume `R` against all nontrivial
integer-partition lower bounds:

```sh
make bin/split_sweep
./bin/split_sweep /tmp/profile.txt 4 20
```

If every partition sum is at least the connected value, the connected witness is
also unrestricted-optimal. If a sum is smaller, the result is inconclusive until
those component patterns are placed at pairwise arrangement distance at least
three. That placement/CSP fallback is not implemented by this numeric sweep.

## Validation status

The signatures reproduce the independent `exact_profile_d2` minimum for `R=1..4`
in `A(6,3)`; the `R=4` result also matches in `A(6,4)`. The `R=5` catalogue has
not completed in the current run, so the implementation should not yet be cited
as differentially validated through `R=5`. A catalogue run that hits its state
cap or is interrupted is not an exhaustive result.
