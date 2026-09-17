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
