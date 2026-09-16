#!/usr/bin/env python3
"""Exact DP evaluation of the maximum possible telescoping potential G(V).

To prove Phi(V) <= P(|V|) by induction without giving up slack, we need a
state function G(V) >= 0 such that at SOME valid coordinate split p:
    Phi(V) + G(V) <= P(|V|) + sum(G(F_s))
which mathematically rearranges to:
    G(V) <= gap(V, p) + sum(G(F_s))

By defining G(V) as the MAXIMUM over all p of [gap(V, p) + sum(G(F_s))],
we find the absolute ceiling for any valid telescoping potential.
If G(V) drops below 0 for any set, it mathematically proves that NO
state function G (exotic or otherwise) can save the exact zero-slack bound.

Scope: This certifies the exact bound for the tested Star-closure instances
only — not all subsets, not profile-only potentials, not parameter-uniform
theorems.  The certificate is split-tree-dependent.
"""

from collections import defaultdict


def sbl(size):
    """Sum of bit_length(i) for 1 <= i < size."""
    return sum(value.bit_length() for value in range(1, size))


def e_seq(size):
    """Cumulative popcount, OEIS A000788."""
    return sum(value.bit_count() for value in range(size))


def c_constant(size):
    """Hamming-ball collision constant C(size)."""
    if size == 0:
        return 0
    return size - 1 + sbl(size) - e_seq(size)


def potential(size, overhang):
    """P(R) = C(R) + m * E(R) where m = overhang."""
    return c_constant(size) + overhang * e_seq(size)


def neighbors(vertex, alphabet_size):
    """Neighbors in the arrangement graph."""
    used = set(vertex)
    result = set()
    for position in range(len(vertex)):
        for symbol in range(alphabet_size):
            if symbol not in used:
                result.add(vertex[:position] + (symbol,) + vertex[position + 1 :])
    return result


def cross_collisions(vertices, alphabet_size, dimension):
    """Root-incidence excess over the external vertex boundary."""
    vertex_set = set(vertices)
    coordinate_boundary_size = 0
    external = set()
    for position in range(dimension):
        directional = set()
        for vertex in vertices:
            used = set(vertex)
            for symbol in range(alphabet_size):
                if symbol not in used:
                    candidate = vertex[:position] + (symbol,) + vertex[position + 1 :]
                    if candidate not in vertex_set:
                        directional.add(candidate)
        coordinate_boundary_size += len(directional)
        external.update(directional)
    return coordinate_boundary_size - len(external)


def defect(vertices, dimension):
    """R*k minus the total number of occupied coordinate roots."""
    root_count = 0
    for position in range(dimension):
        root_count += len(
            {vertex[:position] + vertex[position + 1 :] for vertex in vertices}
        )
    return len(vertices) * dimension - root_count


def phi(vertices, n, k, m):
    """Phi = cross_collisions + (m+1) * defect."""
    return cross_collisions(vertices, n, k) + (m + 1) * defect(vertices, k)


def main():
    """Run DP for each config and report whether G stays non-negative."""
    configs = [(5, 3), (6, 3), (7, 4), (8, 4), (8, 5)]

    for n, k in configs:
        m = n - k
        print(f"\n=== Evaluating A({n},{k}) m={m} ===")

        # 1. Generate seed sets (Star graphs)
        seeds = []
        for sz in range(2, 21):
            center = tuple(range(k))
            avail = sorted(neighbors(center, n))
            if sz - 1 <= len(avail):
                seeds.append(tuple(sorted([center] + avail[: sz - 1])))

        # 2. Closure under fiber splitting
        pool = set(seeds)
        queue = list(seeds)
        while queue:
            V = queue.pop()
            if len(V) < 2:
                continue
            for p_val in range(k):
                fibers = defaultdict(list)
                for v in V:
                    fibers[v[p_val]].append(v)
                if len(fibers) > 1:
                    for fiber_verts in fibers.values():
                        ft = tuple(sorted(fiber_verts))
                        if ft not in pool:
                            pool.add(ft)
                            queue.append(ft)

        # Group by size for bottom-up DAG processing
        by_size = defaultdict(list)
        for V in pool:
            by_size[len(V)].append(V)

        G = {}
        # Base case: singletons have G = 0
        for V in by_size[1]:
            G[V] = 0

        max_R = max(by_size.keys()) if by_size else 0
        found_negative = False

        for R in range(2, max_R + 1):
            if R not in by_size:
                continue

            for V in by_size[R]:
                best_val = -float("inf")

                phi_V = phi(V, n, k, m)
                P_R = potential(R, m)

                for p_val in range(k):
                    fibers = defaultdict(list)
                    for v in V:
                        fibers[v[p_val]].append(v)
                    if len(fibers) < 2:
                        continue

                    sum_phi_F = sum(phi(fv, n, k, m) for fv in fibers.values())
                    sum_P_c = sum(potential(len(fv), m) for fv in fibers.values())

                    overhead = phi_V - sum_phi_F
                    P_surplus = P_R - sum_P_c
                    gap = P_surplus - overhead

                    # DP transition: G(V) <= gap + sum(G(F_s))
                    val = gap + sum(G[tuple(sorted(fv))] for fv in fibers.values())
                    best_val = max(best_val, val)

                G[V] = best_val

                if best_val < 0 and not found_negative:
                    print("  [!] MATHEMATICAL PROOF OF SLACK:")
                    print(f"      Set of size {R} has max G(V) = {best_val}")
                    print(
                        "      Because G(V) < 0 is forced by the"
                        " transition DAG, NO state"
                    )
                    print("      function can save the exact", "Phi <= P(R) bound.")
                    found_negative = True

        if not found_negative:
            print(
                "  All evaluated sets maintained G(V) >= 0!"
                " Exact bound might be possible."
            )

        # Report max G across all sets
        if G:
            max_g = max(G.values())
            min_g = min(G.values())
            print(f"  G range: [{min_g:.1f}, {max_g:.1f}]")

        # Profile-dependence analysis: does G depend only on the
        # sorted fiber sizes at the best split, or on vertex geometry?
        profile_groups = defaultdict(list)
        for V, g_val in G.items():
            if len(V) < 2:
                continue
            phi_V = phi(V, n, k, m)
            P_R = potential(len(V), m)
            best_profile = None
            for p_val in range(k):
                fibers = defaultdict(list)
                for v in V:
                    fibers[v[p_val]].append(v)
                if len(fibers) < 2:
                    continue
                profile = tuple(sorted(len(fv) for fv in fibers.values()))
                if best_profile is None or profile > best_profile:
                    best_profile = profile
            if best_profile is not None:
                profile_groups[best_profile].append((V, g_val))

        # Check consistency: same profile -> same G?
        inconsistent = 0
        total_multi = 0
        for profile, entries in profile_groups.items():
            if len(entries) < 2:
                continue
            total_multi += 1
            g_vals = [e[1] for e in entries]
            if max(g_vals) - min(g_vals) > 0.5:
                inconsistent += 1
                print(
                    f"  INCONSISTENT profile {profile}:"
                    f" G values = {sorted(set(round(g, 1) for g in g_vals))}"
                )

        if total_multi == 0:
            print("  Profile analysis: no multi-entry profiles")
        elif inconsistent == 0:
            print(
                f"  Profile analysis: G is profile-determined"
                f" ({total_multi} profiles, all consistent)"
            )
        else:
            print(
                f"  Profile analysis: G is GEOMETRY-DEPENDENT"
                f" ({inconsistent}/{total_multi} inconsistent)"
            )


if __name__ == "__main__":
    main()
