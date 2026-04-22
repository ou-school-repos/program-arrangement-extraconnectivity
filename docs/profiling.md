# Profiling Analysis

## R=8 gprof flat profile (2026-04-22)

Build: `g++ -std=c++17 -O2 -pg`, single-threaded.

| Function                | % Time    | Self (s) | Calls     | Description                     |
| ----------------------- | --------- | -------- | --------- | ------------------------------- |
| `calc_step()`           | **63.5%** | 0.68     | 3,953,382 | Incremental neighbor-set metric |
| `solve()`               | 36.5%     | 0.39     | 6         | Nauty + dedup + candidate gen   |
| `vertex_to_string()`    | 0.0%      | 0.00     | 373       | String formatting for results   |
| `verify_neighbor_set()` | 0.0%      | 0.00     | 7         | Post-search brute-force check   |

### Key finding

**`calc_step` dominates at 2:1 over nauty + everything else combined.**

It's called once per surviving candidate (after local dedup, before recursion).
But the recursive `solve()` call may immediately prune via nauty/sorted-set dedup
— meaning `calc_step` was wasted for every pruned node.

### Optimization opportunity

Move `calc_step` from the candidate loop into `solve()`, after dedup passes.
This defers the O(R) work until we know the node survives dedup.

**Expected impact:** For R=8, ~92K nodes are iso-pruned out of ~3.95M generated.
That's only ~2.3%, so the direct savings from deferral are modest at R=8.
However, at R=9 with ~2.9M iso-pruned out of ~166M (1.8%), the absolute
savings are ~2.9M × O(9) operations ≈ measurable.

The bigger win is that `calc_step` itself can be optimized further —
it's a hot loop scanning all previous vertices and could benefit from
XOR-based diff detection or SIMD intrinsics for large R.
