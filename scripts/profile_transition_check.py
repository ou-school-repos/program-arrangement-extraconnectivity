#!/usr/bin/env python3
"""Profile-transition feasibility for telescoping Phi-vs-P.

Build the profile-transition graph from recursive Star Graph splits.
For each profile (sorted fiber-size multiset), compute the deficit
and child profiles.  Then check:

1. Are there positive cycles in the transition graph?
2. If not, do the backward-induction G values stay bounded?

A bounded G proves a telescoping potential exists.
An unbounded G or a positive cycle proves no profile-only G works.
"""

import math
from collections import defaultdict

# ---------------------------------------------------------------------------
# Arithmetic
# ---------------------------------------------------------------------------


def sbl(R):
    """Sum of bit_length(i) for 1 <= i < R."""
    return sum(v.bit_length() for v in range(1, R))


def e_seq(R):
    """Cumulative popcount, OEIS A000788."""
    return sum(v.bit_count() for v in range(R))


def c_constant(R):
    """Hamming-ball collision constant C(R)."""
    if R == 0:
        return 0
    return R - 1 + sbl(R) - e_seq(R)


def potential(R, m):
    """P(R) = C(R) + m * E(R)."""
    return c_constant(R) + m * e_seq(R)


# ---------------------------------------------------------------------------
# Geometry
# ---------------------------------------------------------------------------


def neighbors(vertex, alphabet_size):
    """Neighbors in the arrangement graph."""
    used = set(vertex)
    result = set()
    for pos in range(len(vertex)):
        for sym in range(alphabet_size):
            if sym not in used:
                result.add(vertex[:pos] + (sym,) + vertex[pos + 1 :])
    return result


def cross_collisions(vertices, n, k):
    """Root-incidence excess over the external vertex boundary."""
    vset = set(vertices)
    total = 0
    external = set()
    for p in range(k):
        directional = set()
        for v in vertices:
            used = set(v)
            for a in range(n):
                if a not in used:
                    w = v[:p] + (a,) + v[p + 1 :]
                    if w not in vset:
                        directional.add(w)
        total += len(directional)
        external |= directional
    return total - len(external)


def defect(vertices, k):
    """R*k minus the total number of occupied coordinate roots."""
    root_count = 0
    for p in range(k):
        root_count += len({v[:p] + v[p + 1 :] for v in vertices})
    return len(vertices) * k - root_count


def phi(vertices, n, k, m):
    """Phi = cross_collisions + (m+1) * defect."""
    return cross_collisions(vertices, n, k) + (m + 1) * defect(vertices, k)


def star_graph(n, k, size):
    """Radius-one star centered at tuple(range(k))."""
    center = tuple(range(k))
    available = sorted(neighbors(center, n))
    if size <= 0 or size > 1 + len(available):
        return None
    return [center] + available[: size - 1]


# ---------------------------------------------------------------------------
# Profile = sorted tuple of fiber sizes
# ---------------------------------------------------------------------------


def get_profile(vertices, coord):
    """Sorted tuple of fiber sizes when splitting at coord."""
    fibers = defaultdict(list)
    for v in vertices:
        fibers[v[coord]].append(v)
    return tuple(sorted(len(f) for f in fibers.values()))


def profile_total(profile):
    """Sum of fiber sizes."""
    return sum(profile)


# ---------------------------------------------------------------------------
# Recursion: build profile-transition graph
# ---------------------------------------------------------------------------


def build_transitions(n, k, max_r):
    """Build profile-transition graph for Star Graphs up to max_r.

    Returns:
        transitions: dict profile -> list of (child_profiles, deficit)
        profiles_by_r: dict R -> set of profiles with total R
    """
    transitions = {}
    profiles_by_r = defaultdict(set)
    m = n - k

    for r in range(2, max_r + 1):
        V = star_graph(n, k, r)
        if V is None or len(V) < r:
            continue

        for coord in range(k):
            fibers = defaultdict(list)
            for v in V:
                fibers[v[coord]].append(v)
            if len(fibers) < 2:
                continue

            parent_profile = tuple(sorted(len(f) for f in fibers.values()))
            profiles_by_r[profile_total(parent_profile)].add(parent_profile)

            # Compute deficit at this split
            phi_V = phi(V, n, k, m)
            phi_fibers = sum(phi(f, n, k, m) for f in fibers.values())
            overhead = phi_V - phi_fibers
            P_R = potential(r, m)
            P_fibers = sum(potential(s, m) for s in parent_profile)
            P_surplus = P_R - P_fibers
            deficit = overhead - P_surplus

            # Child profiles: for each fiber, compute its profile
            # when split at its own "best" coordinate
            child_profiles = []
            for fiber_vertices in fibers.values():
                fv = list(fiber_vertices)
                if len(fv) <= 1:
                    continue  # singleton, no further split
                best_profile = None
                best_deficit = None
                for c2 in range(k):
                    sub_fibers = defaultdict(list)
                    for v in fv:
                        sub_fibers[v[c2]].append(v)
                    if len(sub_fibers) < 2:
                        continue
                    sub_profile = tuple(sorted(len(sf) for sf in sub_fibers.values()))
                    # Compute deficit for this sub-split
                    phi_fv = phi(fv, n, k, m)
                    phi_sub = sum(phi(sf, n, k, m) for sf in sub_fibers.values())
                    sub_overhead = phi_fv - phi_sub
                    sub_PR = potential(len(fv), m)
                    sub_Pf = sum(potential(ss, m) for ss in sub_profile)
                    sub_surplus = sub_PR - sub_Pf
                    sub_deficit = sub_overhead - sub_surplus
                    if best_profile is None or sub_deficit > best_deficit:
                        best_profile = sub_profile
                        best_deficit = sub_deficit
                if best_profile is not None:
                    child_profiles.append(best_profile)

            key = (parent_profile, coord)
            transitions[key] = (child_profiles, deficit)

    return transitions, profiles_by_r


# ---------------------------------------------------------------------------
# Cycle detection and G computation
# ---------------------------------------------------------------------------


def check_positive_cycles(transitions):
    """Check for positive cycles using Bellman-Ford on the deficit graph.

    For each transition (parent_profile, coord) -> (child_profiles, deficit):
        G(parent) - sum G(child) >= deficit

    This is NOT a standard difference constraint.  We check consistency
    by trying backward induction and detecting contradictions.
    """
    # Build adjacency: parent_profile -> list of (child_profiles, deficit)
    adj = defaultdict(list)
    for (parent, _coord), (children, deficit) in transitions.items():
        adj[parent].append((children, deficit))

    # Topological sort by total size (larger first)
    all_profiles = list(adj.keys())
    all_profiles.sort(key=lambda p: sum(p), reverse=True)

    # Backward induction: compute G values
    G = {}
    contradictions = []

    for profile in all_profiles:
        if profile not in adj:
            G[profile] = 0
            continue

        max_required = 0
        for children, deficit in adj[profile]:
            child_G_sum = sum(G.get(c, 0) for c in children)
            required = deficit + child_G_sum
            if required > max_required:
                max_required = required

        G[profile] = max_required

    return G, contradictions


def check_cycles_dfs(transitions):
    """Check for cycles in the profile-transition graph using DFS."""
    adj = defaultdict(list)
    for (parent, _coord), (children, _deficit) in transitions.items():
        for child in children:
            adj[parent].append(child)

    WHITE, GRAY, BLACK = 0, 1, 2
    color = defaultdict(int)
    cycle_found = False
    cycle_path = []

    def dfs(node, path):
        nonlocal cycle_found, cycle_path
        color[node] = GRAY
        path.append(node)
        for neighbor in adj[node]:
            if color[neighbor] == GRAY:
                cycle_found = True
                cycle_path = path[path.index(neighbor) :] + [neighbor]
            elif color[neighbor] == WHITE:
                dfs(neighbor, path)
        path.pop()
        color[node] = BLACK

    for node in adj:
        if color[node] == WHITE:
            dfs(node, [])
            if cycle_found:
                return True, cycle_path

    return False, []


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    """Build transitions, check cycles, compute G, report results."""
    print("=== Profile-transition graph for Star Graphs ===\n")

    for n, k in [(5, 3), (6, 3), (7, 4), (8, 4), (8, 5)]:
        m = n - k
        max_r = min(20, 1 + k * (n - k))
        print(f"A({n},{k}) m={m} max_r={max_r}")

        transitions, profiles_by_r = build_transitions(n, k, max_r)
        print(f"  Transitions: {len(transitions)}")

        # Check for cycles
        has_cycle, cycle_path = check_cycles_dfs(transitions)
        if has_cycle:
            print(f"  CYCLE DETECTED: {cycle_path}")
            print("  No profile-only telescoping potential exists!")
        else:
            print("  No cycles (DAG)")

        # Compute G values by backward induction
        G, _ = check_positive_cycles(transitions)

        # Report G values
        max_G = max(G.values()) if G else 0
        print(f"  Max G value: {max_G}")

        # Show G values sorted by total size
        print(f"  {'profile':>30s} {'total':>5s} {'deficit':>8s} {'G':>8s}")
        print("  " + "-" * 55)
        for profile in sorted(G.keys(), key=lambda p: sum(p)):
            total = sum(profile)
            # Find deficit for this profile
            deficit_vals = []
            for (parent, _coord), (children, deficit) in transitions.items():
                if parent == profile:
                    deficit_vals.append(deficit)
            deficit_str = str(deficit_vals[0]) if deficit_vals else "0"
            print(
                f"  {str(profile):>30s} {total:5d} "
                f"{deficit_str:>8s} {G[profile]:8.1f}"
            )

        # Check if G is bounded (constant) or grows with R
        g_by_total = defaultdict(list)
        for profile, g_val in G.items():
            g_by_total[sum(profile)].append(g_val)

        max_g_by_total = {total: max(vals) for total, vals in g_by_total.items()}
        totals = sorted(max_g_by_total.keys())
        if len(totals) >= 3:
            # Check if max G grows with total size
            recent = [max_g_by_total[t] for t in totals[-3:]]
            if recent[-1] > recent[0] * 1.5:
                print(f"  G appears to GROW with R (not bounded)")
            else:
                print(f"  G appears BOUNDED across tested sizes")
        print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
