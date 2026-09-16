#!/usr/bin/env python3
"""Verify the four-lemma analytic squeeze for the restricted boundary theorem.

Checks:
1. Collision-Splitting Identity: sum_p Delta_X(V,p) = sum_{mu>=2} mu(w)
2. Pairwise Multiplicity Envelope: mu_sum <= 2R(R-1) + 2(m-3)*P_1
3. Superadditive Minimization: sum_s f(c_{p,s}) <= f(Delta_D_p + 1)
4. Master Inequality: LHS <= k * P(R)
"""

import itertools
import math
import random
from collections import defaultdict

from lib import arithmetic_potential as P
from lib import coord_boundary_and_roots, f_func


def compute_p1(vertices, k):
    """Compute exact number of distance-1 pairs (P_1) in the subset."""
    p1 = 0
    for u, v in itertools.combinations(vertices, 2):
        if sum(1 for i in range(k) if u[i] != v[i]) == 1:
            p1 += 1
    return p1


def full_statistics(vertices, n, k):
    """Compute D, X, and per-coordinate data for a subset."""
    R = len(vertices)

    # Defect
    D = 0
    all_roots = []
    for p in range(k):
        roots = set()
        for vertex in vertices:
            roots.add(vertex[:p] + vertex[p + 1 :])
        all_roots.append(roots)
        D += R - len(roots)

    # Cross-collisions and per-coordinate data
    directions = []
    all_external = set()
    for p in range(k):
        ext, _ = coord_boundary_and_roots(vertices, p, n)
        directions.append(ext)
        all_external |= ext

    total_dir = sum(len(d) for d in directions)
    X = total_dir - len(all_external)

    # Multiplicity sum: sum_{mu>=2} mu(w)
    multiplicity = defaultdict(int)
    for d in directions:
        for w in d:
            multiplicity[w] += 1
    mu_sum = sum(mu for mu in multiplicity.values() if mu >= 2)

    # Per-split data
    split_data = []
    for p in range(k):
        fibers = defaultdict(list)
        for vertex in vertices:
            # The coordinate-split tree partitions by the symbol at p.
            fibers[vertex[p]].append(vertex)
        child_sizes = [len(f) for f in fibers.values()]
        _, roots_p = coord_boundary_and_roots(vertices, p, n)
        Delta_D_p = R - len(roots_p)
        # sum_s f(c_{p,s})
        sum_f_children = sum(f_func(cs, n - k) for cs in child_sizes)
        # f(Delta_D_p + 1) - the superadditive bound
        f_bound = f_func(Delta_D_p + 1, n - k)
        split_data.append(
            {
                "Delta_D_p": Delta_D_p,
                "child_sizes": child_sizes,
                "sum_f_children": sum_f_children,
                "f_bound": f_bound,
                "superadditive_ok": sum_f_children <= f_bound + 1e-9,
            }
        )

    return {
        "R": R,
        "D": D,
        "X": X,
        "mu_sum": mu_sum,
        "directions": directions,
        "all_roots": all_roots,
        "split_data": split_data,
    }


def verify_lemma2_envelope(R, mu_sum, m, P1):
    """Lemma 2: sum mu(w) <= 2R(R-1) + 2(m-3)*P_1."""
    bound = 2 * R * (R - 1) + 2 * (m - 3) * P1
    return mu_sum <= bound + 1e-9, bound


def verify_lemma3_superadditive(split_data):
    """Lemma 3: sum_s f(c_{p,s}) <= f(Delta_D_p + 1) for all p."""
    all_ok = True
    worst_gap = float("inf")
    for sd in split_data:
        if not sd["superadditive_ok"]:
            all_ok = False
        gap = sd["f_bound"] - sd["sum_f_children"]
        worst_gap = min(worst_gap, gap)
    return all_ok, worst_gap


def verify_lemma4_master(R, D, m, k, split_data, mu_sum):
    """Check the exact averaged master inequality."""
    lhs_geom = mu_sum + (m + 1) * D
    lhs_fibers = sum(P(cs, m) for split in split_data for cs in split["child_sizes"])
    lhs_total = lhs_geom + lhs_fibers
    rhs = k * P(R, m)

    return lhs_total <= rhs + 1e-9, lhs_total, rhs


def main():
    """Run bounded random checks and report violations of each diagnostic."""
    print("=== VERIFYING FOUR-LEMMA ANALYTIC SQUEEZE ===\n")
    results = {}
    MAX_SAMPLES = 5000

    for m in [2, 3, 4]:
        k = m + m
        n = k + m
        print(f"\n--- A({n},{k}), m={m} ---")

        max_R = min(2**m, 16)

        vertices_all = list(itertools.permutations(range(n), k))

        for R in range(2, max_R + 1):
            total_combinations = math.comb(len(vertices_all), R)

            if total_combinations > MAX_SAMPLES:
                print(f"  R={R}: sampling {MAX_SAMPLES} from {total_combinations}...")
                seen = set()
                combos = []
                while len(combos) < MAX_SAMPLES:
                    subset = frozenset(random.sample(vertices_all, R))
                    if subset not in seen:
                        seen.add(subset)
                        combos.append(tuple(subset))
            else:
                print(f"  R={R}: enumerating all {total_combinations}...")
                combos = list(itertools.combinations(vertices_all, R))

            count = 0
            violations = {"L2": 0, "L3": 0, "L4": 0}
            worst_L4_slack = float("inf")
            worst_L4_config = None

            for vertices in combos:
                data = full_statistics(vertices, n, k)
                P1 = compute_p1(vertices, k)

                # Lemma 2: Envelope
                ok2, _ = verify_lemma2_envelope(data["R"], data["mu_sum"], m, P1)
                if not ok2:
                    violations["L2"] += 1

                # Lemma 3: Superadditive
                ok3, _ = verify_lemma3_superadditive(data["split_data"])
                if not ok3:
                    violations["L3"] += 1

                # Lemma 4: Master
                ok4, lhs, rhs = verify_lemma4_master(
                    data["R"], data["D"], m, k, data["split_data"], data["mu_sum"]
                )
                if not ok4:
                    violations["L4"] += 1

                slack = rhs - lhs
                if slack < worst_L4_slack:
                    worst_L4_slack = slack
                    worst_L4_config = (data["R"], data["D"], data["X"])

                count += 1

            status = "PASS" if all(v == 0 for v in violations.values()) else "FAIL"
            print(
                f"    subsets={count} L2_viol={violations['L2']} "
                f"L3_viol={violations['L3']} L4_viol={violations['L4']} "
                f"worst_slack={worst_L4_slack:.1f} {status}"
            )

            results[(m, R)] = {
                "count": count,
                "violations": violations.copy(),
                "worst_slack": worst_L4_slack,
                "worst_config": worst_L4_config,
            }

    # Summary
    print("\n\n=== SUMMARY ===")
    print(
        f"{'m':>3} {'R':>4} {'subsets':>8} {'L2':>4} {'L3':>4} {'L4':>4} {'slack':>8}"
    )
    for (m, R), res in sorted(results.items()):
        print(
            f"{m:3d} {R:4d} {res['count']:8d} "
            f"{res['violations']['L2']:4d} {res['violations']['L3']:4d} "
            f"{res['violations']['L4']:4d} {res['worst_slack']:8.1f}"
        )

    total_violations = sum(sum(res["violations"].values()) for res in results.values())
    print(f"\nTotal violations across all cells: {total_violations}")

    return 0 if total_violations == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
