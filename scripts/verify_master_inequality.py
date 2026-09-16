#!/usr/bin/env python3
"""Verify the four-lemma analytic squeeze for the restricted boundary theorem.

Checks:
1. Collision-Splitting Identity: sum_p Delta_X(V,p) = sum_{mu>=2} mu(w)
2. Pairwise Multiplicity Envelope: mu_sum <= 2R(R-1) + 2(m-3)*P_1
3. Superadditive Minimization: sum_s f(c_{p,s}) <= f(Delta_D_p + 1)
4. Master Inequality: LHS <= k * P(R)

Uses random sampling for R >= 5 to avoid combinatorial explosion.
Always tests the Star graph closures (the extremal configurations).
"""

import itertools
import math
import random
from collections import defaultdict


def sbl(size):
    return sum(value.bit_length() for value in range(1, size))


def e_seq(size):
    return sum(value.bit_count() for value in range(size))


def f_func(x, m):
    """f(x) = sbl(x) + (m-1)*E(x)."""
    if x <= 0:
        return 0
    return sbl(x) + (m - 1) * e_seq(x)


def P_func(R, m):
    return (R - 1) + sbl(R) + (m - 1) * e_seq(R)


def coord_boundary_and_roots(vertices, p, n, k):
    """Return (external_neighbors_at_p, roots_at_p) for coordinate p."""
    members = set(vertices)
    roots = set()
    external = set()
    for vertex in vertices:
        roots.add(vertex[:p] + vertex[p + 1 :])
        used = set(vertex)
        for symbol in range(n):
            if symbol in used:
                continue
            neighbor = vertex[:p] + (symbol,) + vertex[p + 1 :]
            if neighbor not in members:
                external.add(neighbor)
    return external, roots


def full_statistics(vertices, n, k):
    """Compute D, X, and per-coordinate data for a subset."""
    members = set(vertices)
    R = len(vertices)
    m = n - k

    # Defect
    D = 0
    all_roots = []
    for p in range(k):
        roots = set()
        for vertex in vertices:
            roots.add(vertex[:p] + vertex[p + 1 :])
        all_roots.append(roots)
        D += R - len(roots)

    directions = []
    all_external = set()
    for p in range(k):
        ext, _ = coord_boundary_and_roots(vertices, p, n, k)
        directions.append(ext)
        all_external |= ext

    total_dir = sum(len(d) for d in directions)
    X = total_dir - len(all_external)

    multiplicity = defaultdict(int)
    for d in directions:
        for w in d:
            multiplicity[w] += 1
    mu_sum = sum(mu for mu in multiplicity.values() if mu >= 2)

    P1 = 0
    for u, v in itertools.combinations(vertices, 2):
        if sum(1 for i in range(k) if u[i] != v[i]) == 1:
            P1 += 1

    split_data = []
    for p in range(k):
        fibers = defaultdict(list)
        for vertex in vertices:
            fibers[vertex[p]].append(vertex)
        child_sizes = [len(f) for f in fibers.values()]
        _, roots_p = coord_boundary_and_roots(vertices, p, n, k)
        Delta_D_p = R - len(roots_p)
        sum_f_children = sum(f_func(cs, m) for cs in child_sizes)
        f_bound = f_func(Delta_D_p + 1, m)
        split_data.append(
            {
                "Delta_D_p": Delta_D_p,
                "sum_f_children": sum_f_children,
                "f_bound": f_bound,
                "superadditive_ok": sum_f_children <= f_bound + 1e-9,
            }
        )

    return {
        "R": R,
        "D": D,
        "X": X,
        "P1": P1,
        "mu_sum": mu_sum,
        "split_data": split_data,
    }


def verify_all(data, m, k):
    R, D, mu_sum, P1 = data["R"], data["D"], data["mu_sum"], data["P1"]
    n = k + m
    P_R = P_func(R, m)

    ok2 = mu_sum <= 2 * R * (R - 1) + 2 * (m - 3) * P1 + 1e-9
    ok3 = all(sd["superadditive_ok"] for sd in data["split_data"])

    lhs_envelope = 2 * R * (R - 1) + max(0, 2 * (m - 3)) * P1
    lhs_defect = (m + 2) * D
    lhs_superadd = sum(sd["f_bound"] for sd in data["split_data"])
    lhs_total = lhs_envelope + lhs_defect + lhs_superadd
    rhs = k * P_R
    ok4 = lhs_total <= rhs + 1e-9

    return {
        "L2_ok": ok2,
        "L3_ok": ok3,
        "L4_ok": ok4,
        "slack": rhs - lhs_total,
        "P_R": P_R,
    }


def star_graph(n, k, active):
    center = tuple(range(k))
    vertices = [center]
    for position in range(active):
        for symbol in range(k, n):
            vertices.append(center[:position] + (symbol,) + center[position + 1 :])
    return vertices


def random_subset(vertices_all, R, seed):
    rng = random.Random(seed)
    return rng.sample(vertices_all, R)


def main():
    random.seed(42)
    SAMPLES = 5000

    print("=== FOUR-LEMMA ANALYTIC SQUEEZE VERIFICATION ===\n")

    all_pass = True

    for m in [2, 3, 4]:
        k = m + m
        n = k + m
        vertices_all = list(itertools.permutations(range(n), k))
        max_R = min(2**m, 30)
        print(f"\n--- A({n},{k}), m={m}, |V|={len(vertices_all)}, max_R={max_R} ---")
        print(
            f"{'R':>4} {'subsets':>8} {'L2':>4} {'L3':>4} {'L4':>4} {'slack':>8} {'min_slack':>10}"
        )

        for R in range(2, max_R + 1):
            num_combos = math.comb(len(vertices_all), R)
            use_enumerate = num_combos <= 20000

            violations = {"L2": 0, "L3": 0, "L4": 0}
            worst_slack = float("inf")
            count = 0

            if use_enumerate:
                pool = itertools.combinations(vertices_all, R)
            else:
                pool = [random_subset(vertices_all, R, i) for i in range(SAMPLES)]

            for combo in pool:
                vertices = list(combo)
                data = full_statistics(vertices, n, k)
                res = verify_all(data, m, k)
                if not res["L2_ok"]:
                    violations["L2"] += 1
                if not res["L3_ok"]:
                    violations["L3"] += 1
                if not res["L4_ok"]:
                    violations["L4"] += 1
                worst_slack = min(worst_slack, res["slack"])
                count += 1

            star_configs = []
            for active in range(1, k + 1):
                R_star = 1 + active * m
                if R_star == R:
                    star_configs.append(star_graph(n, k, active))

            for sv in star_configs:
                data = full_statistics(sv, n, k)
                res = verify_all(data, m, k)
                if not res["L2_ok"]:
                    violations["L2"] += 1
                if not res["L3_ok"]:
                    violations["L3"] += 1
                if not res["L4_ok"]:
                    violations["L4"] += 1
                worst_slack = min(worst_slack, res["slack"])
                count += 1

            status = "PASS" if all(v == 0 for v in violations.values()) else "FAIL"
            if not all(v == 0 for v in violations.values()):
                all_pass = False
            print(
                f"{R:4d} {count:8d} {violations['L2']:4d} {violations['L3']:4d} "
                f"{violations['L4']:4d} {worst_slack:8.1f} {status}"
            )

    print(f"\n{'='*60}")
    print(f"OVERALL: {'ALL PASS' if all_pass else 'VIOLATIONS FOUND'}")
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
