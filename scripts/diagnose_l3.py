#!/usr/bin/env python3
"""Diagnose L3 violations: understand the bound direction for
sum_s f(c_{p,s}) vs f(Delta_D_p + 1).

For each subset where L3 fails, print the exact fiber partition and check:
1. Is sum_s f(c_{p,s}) > f(Delta_D_p + 1)? (L3 bound is wrong direction)
2. Is sum_s f(c_{p,s}) < f(Delta_D_p + 1)? (L3 bound is correct direction)
3. What is the concentrated partition bound f(R)?
"""

import itertools
import math

from lib import arithmetic_potential as P
from lib import coord_boundary_and_roots, f_func


def diagnose_l3(m_val):
    """Diagnose L3 violations for a given m."""
    k = m_val + m_val
    n = k + m_val
    m = m_val  # overhang

    print(f"\n=== DIAGNOSING L3 for A({n},{k}), m={m} ===")
    print(f"f(x) = sbl(x) + {m - 1}*E(x)")
    print(f"P(R) = (R-1) + sbl(R) + {m - 1}*E(R)")

    # Print f values for small x
    print("\nf values:")
    for x in range(1, 20):
        print(f"  f({x}) = {f_func(x, m)}")

    vertices_all = list(itertools.permutations(range(n), k))
    max_R = min(2**m, 8)  # Small R for diagnosis

    for R in range(2, max_R + 1):
        print(f"\n--- R={R} ---")
        total_combinations = math.comb(len(vertices_all), R)
        print(f"  Total subsets: {total_combinations}")

        # Sample a few subsets
        combos = list(
            itertools.combinations(vertices_all, min(R + 2, total_combinations))
        )

        l3_violations = 0
        l3_total = 0
        examples = []

        for vertices in combos:
            # Per-split data
            for p in range(k):
                fibers = {}
                for vertex in vertices:
                    symbol = vertex[p]
                    if symbol not in fibers:
                        fibers[symbol] = []
                    fibers[symbol].append(vertex)

                child_sizes = [len(f) for f in fibers.values()]
                _, roots_p = coord_boundary_and_roots(vertices, p, n)
                Delta_D_p = R - len(roots_p)

                sum_f_children = sum(f_func(cs, m) for cs in child_sizes)
                f_bound = f_func(Delta_D_p + 1, m)
                f_concentrated = f_func(R, m)

                l3_total += 1
                if sum_f_children > f_bound + 1e-9:
                    l3_violations += 1
                    if len(examples) < 5:
                        examples.append(
                            {
                                "R": R,
                                "p": p,
                                "child_sizes": child_sizes,
                                "Delta_D_p": Delta_D_p,
                                "sum_f_children": sum_f_children,
                                "f_bound": f_bound,
                                "f_concentrated": f_concentrated,
                            }
                        )

        print(f"  L3 violations: {l3_violations}/{l3_total}")
        print(f"  Violation rate: {l3_violations / l3_total * 100:.1f}%")

        # Print examples
        for i, ex in enumerate(examples):
            print(f"\n  Example {i + 1}:")
            print(f"    Partition: {ex['child_sizes']}")
            print(f"    Delta_D_p = {ex['Delta_D_p']}")
            print(f"    sum_s f(c_{{p,s}}) = {ex['sum_f_children']}")
            print(f"    f(Delta_D_p + 1) = {ex['f_bound']}")
            print(f"    f(R) = {ex['f_concentrated']}")
            ratio = ex["sum_f_children"] / max(ex["f_bound"], 1)
            print(f"    Ratio sum/f_bound = {ratio:.2f}")
            print(f"    L3 holds? {ex['sum_f_children'] <= ex['f_bound'] + 1e-9}")

        # Check: is f superadditive?
        print("\n  Superadditivity check:")
        for a in range(1, R):
            b = R - a
            fa = f_func(a, m)
            fb = f_func(b, m)
            fab = f_func(R, m)
            print(
                f"    f({a}) + f({b}) = {fa + fb}, "
                f"f({a}+{b}) = {fab}, superadditive? {fa + fb <= fab}"
            )

        # Check: what is the concentrated partition bound?
        print("\n  Concentrated partition bound:")
        print(f"    f(R) = f({R}) = {f_func(R, m)}")
        print(
            f"    f(Delta_D_p + 1) for Delta_D_p = R-1 = {R - 1}: f({R}) = {f_func(R, m)}"
        )
        print("    So f(R) = f(Delta_D_p + 1) for concentrated partition")


def check_master_inequality_forms(m_val):
    """Check both forms of the master inequality."""
    k = m_val + m_val
    n = k + m_val
    m = m_val

    print(f"\n=== CHECKING MASTER INEQUALITY FORMS for A({n},{k}), m={m} ===")

    vertices_all = list(itertools.permutations(range(n), k))
    max_R = min(2**m, 8)

    for R in range(2, max_R + 1):
        combos = list(
            itertools.combinations(
                vertices_all, min(R + 2, math.comb(len(vertices_all), R))
            )
        )

        exact_violations = 0
        user_violations = 0
        total = 0

        for vertices in combos:
            # Compute D, X, mu_sum
            D = 0
            all_roots = []
            for p in range(k):
                roots = set()
                for vertex in vertices:
                    roots.add(vertex[:p] + vertex[p + 1 :])
                all_roots.append(roots)
                D += R - len(roots)

            # Compute mu_sum
            multiplicity = {}
            for p in range(k):
                ext, _ = coord_boundary_and_roots(vertices, p, n)
                for w in ext:
                    multiplicity[w] = multiplicity.get(w, 0) + 1
            mu_sum = sum(mu for mu in multiplicity.values() if mu >= 2)

            # Compute P1
            P1 = 0
            for u, v in itertools.combinations(vertices, 2):
                if sum(1 for i in range(k) if u[i] != v[i]) == 1:
                    P1 += 1

            # Per-split data
            split_f_sum = 0
            f_bound_sum = 0
            for p in range(k):
                fibers = {}
                for vertex in vertices:
                    symbol = vertex[p]
                    if symbol not in fibers:
                        fibers[symbol] = []
                    fibers[symbol].append(vertex)

                child_sizes = [len(f) for f in fibers.values()]
                _, roots_p = coord_boundary_and_roots(vertices, p, n)
                Delta_D_p = R - len(roots_p)

                split_f_sum += sum(f_func(cs, m) for cs in child_sizes)
                f_bound_sum += f_func(Delta_D_p + 1, m)

            # Exact master: mu_sum + (m+1)*D + sum_p sum_s P(c_{p,s}, m) <= k * P(R, m)
            # Which simplifies to: mu_sum + (m+2)*D + sum_p sum_s f(c_{p,s}, m)
            # <= k * P(R, m)
            lhs_exact = mu_sum + (m + 2) * D + split_f_sum
            rhs_exact = k * P(R, m)

            # User's master: 2R(R-1) + max(0, 2(m-3)*P_1) + (m+2)*D
            # + sum_p f(Delta_D_p + 1) <= k * P(R)
            bound_mu = 2 * R * (R - 1) + 2 * (m - 3) * P1
            lhs_user = bound_mu + (m + 2) * D + f_bound_sum
            rhs_user = k * P(R, m)

            total += 1
            if lhs_exact > rhs_exact + 1e-9:
                exact_violations += 1
            if lhs_user > rhs_user + 1e-9:
                user_violations += 1

        print(
            f"  R={R}: exact_viol={exact_violations}/{total}, "
            f"user_viol={user_violations}/{total}"
        )


if __name__ == "__main__":
    for overhang in [2, 3, 4]:
        diagnose_l3(overhang)
        check_master_inequality_forms(overhang)
