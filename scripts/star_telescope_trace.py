#!/usr/bin/env python3
"""Trace the max-DP split lineage of one capacity-valid Star Graph.

This is a diagnostic for the recursive closure of one Star seed.  The
reported values are the maximum-DP ceilings from ``run_dp``; they are not
minimal or forced profile-potential values.
"""

import argparse
from collections import defaultdict

from deficit_telescope_probe import compute_state, run_dp, star_graph


def fiber_closure(seed, k):
    """Return the closure of ``seed`` under all nontrivial coordinate fibers."""
    pool = {seed}
    queue = [seed]
    while queue:
        vertices = queue.pop()
        for position in range(k):
            fibers = defaultdict(list)
            for vertex in vertices:
                fibers[vertex[position]].append(vertex)
            if len(fibers) < 2:
                continue
            for fiber in fibers.values():
                child = tuple(sorted(fiber))
                if child not in pool:
                    pool.add(child)
                    queue.append(child)
    return pool


def trace_lineage(n, k, size):
    """Print the best-split path, following the largest child DP ceiling."""
    seed = star_graph(n, k, size)
    if seed is None:
        raise ValueError(f"R={size} exceeds Star capacity in A({n},{k})")
    pool = fiber_closure(seed, k)
    ceilings, info, found_negative = run_dp(n, k, pool)
    if found_negative:
        raise RuntimeError("the max-DP ceiling is negative in this closure")
    info_by_set = {entry["set"]: entry for entry in info}

    print(f"A({n},{k}) Star R={size}; closure sets={len(pool)}")
    current = seed
    step = 0
    while len(current) > 1:
        entry = info_by_set[current]
        children = entry["children"]
        if not children:
            break
        child = max(children, key=lambda item: (ceilings[item], len(item), item))
        child_sizes = tuple(sorted(len(item) for item in children))
        state = compute_state(current, k)
        max_fiber = max(fiber_size for profile in state for fiber_size in profile)
        print(
            f"step={step} R={len(current)} ceiling={ceilings[current]}"
            f" max_fiber={max_fiber} p={entry['best_p']} gap={entry['gap']}"
            f" children={child_sizes} -> next_R={len(child)}"
            f" next_ceiling={ceilings[child]}"
        )
        print(f"  state={state}")
        current = child
        step += 1


def main():
    """Parse one Star configuration and trace its recursive closure."""
    parser = argparse.ArgumentParser()
    parser.add_argument("n", type=int)
    parser.add_argument("k", type=int)
    parser.add_argument("R", type=int)
    args = parser.parse_args()
    trace_lineage(args.n, args.k, args.R)


if __name__ == "__main__":
    main()
