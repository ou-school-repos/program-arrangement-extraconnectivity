# Profiling Analysis

## R=8 gprof flat profile (2026-04-22)

Build: `g++ -std=c++17 -O2 -pg`, single-threaded.

<!-- markdownlint-disable MD013 -->

| Function                | % Time    | Self (s) | Calls     | Description                     |
| ----------------------- | --------- | -------- | --------- | ------------------------------- |
| `calc_step()`           | **63.5%** | 0.68     | 3,953,382 | Incremental neighbor-set metric |
| `solve()`               | 36.5%     | 0.39     | 6         | Nauty + dedup + candidate gen   |
| `vertex_to_string()`    | 0.0%      | 0.00     | 373       | String formatting for results   |
| `verify_neighbor_set()` | 0.0%      | 0.00     | 7         | Post-search brute-force check   |

<!-- markdownlint-enable MD013 -->

### Key finding

**`calc_step` dominates at 2:1 over nauty + everything else combined.**

It's called once per surviving candidate (after local dedup, before recursion).
But the recursive `solve()` call may immediately prune via nauty/sorted-set
dedup — meaning `calc_step` was wasted for every pruned node.

### Optimization opportunity

Move `calc_step` from the candidate loop into `solve()`, after dedup passes.
This defers the O(R) work until we know the node survives dedup.

**Expected impact:** For R=8, ~92K nodes are iso-pruned out of ~3.95M generated.
That's only ~2.3%, so the direct savings from deferral are modest at R=8.
However, at R=9 with ~2.9M iso-pruned out of ~166M (1.8%), the absolute savings
are ~2.9M × O(9) operations ≈ measurable.

The bigger win is that `calc_step` itself can be optimized further — it's a hot
loop scanning all previous vertices and could benefit from XOR-based diff
detection or SIMD intrinsics for large R.

## Gamma audit cutoff: A(6,3), R=7 (2026-09-17)

Command:

```text
./empirical_gamma_bound 6 3 7 /tmp/gamma-6-3-r7.csv \
  --phi=0,9,14,18,20,23,24,25
```

The run generated all of the following canonical-subset counts:

| R   | Canonical subsets |
| --- | ----------------: |
| 2   |                 3 |
| 3   |                48 |
| 4   |             1,561 |
| 5   |            39,839 |
| 6   |           820,149 |
| 7   |        13,677,666 |

At $R=7$, the exhaustive bipartition phase would require approximately
$13,677,666\cdot63=861,692,958$ partition checks. After roughly two hours, the
run was stopped before producing a final gamma maximum. This is recorded as an
incomplete audit: the canonical enumeration completed, but no claim about the
$R=7$ gamma bound follows from it.

The structural defect/collision diagnostic is now the preferred instrument for
the proof because it tests the two local inequalities directly and avoids this
partition explosion.

### Canonical-orbit audit correction (2026-09-17)

An independent Burnside count found that the previous size-two canonicalization
shortcut was unsound: coordinate agreement alone does not determine a subset
orbit. For $A(6,3)$ the correct orbit counts through $R=7$ are:

```text
./audit_orbits 6 3 7
A(6,3) Burnside subset-orbit counts:
[1, 1, 9, 101, 2181, 45362, 852968, 13801074]
```

The shortcut has been removed from the shared canonicalizer, the subset profile
implementation, and the gamma representative path. Earlier gamma/profile counts
produced with that shortcut are withdrawn until regenerated. The direct
origin-pinned scans in `diagnose_defect_collision` do not use canonical keys and
are unaffected.

## Agreement-class audit: A(7,3), R=5 (2026-09-17)

Command:

```text
time ./diagnose_defect_collision 7 3 5
```

The scan covered 77,238,876 origin-pinned subsets in 3m05.9s. The defect,
collision, one-vertex peeling, and unrestricted pair-peeling diagnostics all
passed with minimum margin zero. By coordinate agreement $c$ of the selected
pair, the best available pair margin was:

| $c$ | Pair margin |
| --: | ----------: |
|   0 |          -1 |
|   1 |          -2 |
|   2 |           0 |

Thus the pair recurrence succeeds existentially, but not for arbitrary pair
types. The $c=2$ pairs are adjacent in this $k=3$ case; nonadjacent pairs also
tie the overall zero margin. This supports studying the full pair-drop quantity
rather than treating coordinate agreement as a sufficient statistic.

## Full-Star pair-peeling shoreline: A(10,8) (2026-09-17)

The native `diagnose_star_clusters` probe evaluates the established radius-one
full-Star spine $S_j$ without constructing the full arrangement. It stores only
the $O(k(n-k))$ Star vertices and derives the local fiber counts and boundary
from them. For $A(10,8)$, where $m=2$ and $|S_j|=1+2j$:

```text
time ./diagnose_star_clusters 10 8 8
```

The pair-peeling margins along the spine were:

| $j$ | $R$ | Pair margin | Boundary slack |
| --: | --: | ----------: | -------------: |
|   6 |  13 |           0 |              5 |
|   7 |  15 |          -1 |              3 |
|   8 |  17 |          -3 |             -1 |

Thus pair peeling fails at $R=15$, before the surrogate boundary inequality
fails at the known full-Star counterexample $R=17$. Pair peeling is therefore a
sufficient safe-regime mechanism, not an exact characterization of the
surrogate's failure threshold. The C++ probe uses the radius-one Star definition
from `scripts/full_star_spine.py`, not a full coordinate subspace.

The formula-only implementation was then run on larger parameters without the
factorial-sized arrangement allocation:

```text
time ./diagnose_star_clusters 12 10 10
time ./diagnose_star_clusters 15 12 12
```

For $A(12,10)$, the surrogate first fails at $R=17$ and the pair margin is
already $-1$ there. For $A(15,12)$, the surrogate first fails at $R=19$ and the
pair margin is $-7$. Both runs complete in under one second, confirming that the
previous multi-gigabyte behavior was caused entirely by unnecessary full-graph
construction.

## Full-Star dual sweep by $m=n-k$ (2026-09-17)

The formula-only diagnostic was swept over $m\in\{2,3,4,5\}$ and several values
of $k$. For fixed $m$ and Star branch count $j$, the margins were identical
across all tested $k$. Thus $k$ affects the reported volume $R=1+jm$, but not
the Star's defect, collision, boundary slack, or pair margin.

| $m$ | First negative pair margin | First negative boundary slack |
| --: | -------------------------: | ----------------------------: |
|   2 |              $j=7$, $R=15$ |                 $j=8$, $R=17$ |
|   3 |              $j=6$, $R=19$ |                 $j=6$, $R=19$ |
|   4 |              $j=4$, $R=17$ |                 $j=6$, $R=25$ |
|   5 |              $j=4$, $R=21$ |                 $j=6$, $R=31$ |

The balanced slice (m=k) extends this pattern:

| $m=k$ | First negative pair margin | First negative boundary slack |
| ----: | -------------------------: | ----------------------------: |
|     6 |              $j=3$, $R=19$ |                 $j=6$, $R=37$ |
|     8 |              $j=4$, $R=33$ |                 $j=6$, $R=49$ |
|    10 |              $j=4$, $R=41$ |                 $j=6$, $R=61$ |
|    12 |              $j=6$, $R=73$ |                 $j=6$, $R=73$ |
|    15 |              $j=5$, $R=76$ |                 $j=6$, $R=91$ |
|    20 |              $j=4$, $R=81$ |                $j=7$, $R=141$ |

This is evidence that the Star shoreline can be parameterized by $(m,j)$. The
volume is $R=1+jm$, while $k$ only constrains the construction through $j\le k$.
It also separates the pair-peeling threshold from the final surrogate failure
threshold: for $m=2$ and $m=5$, pair peeling fails strictly earlier.
