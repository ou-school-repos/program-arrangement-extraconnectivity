#!/usr/bin/env python3
"""Profile-state DP and global validation for telescoping Phi <= P(R).

Phase 1 (Star closure):
  Compute G(V) = max_p [gap(V,p) + sum G(F_s)] bottom-up.
  State(V) = sorted tuple of (sorted fiber sizes at coord p) for each p.
  Check: same state -> same G?

Phase 2 (bounded global validation):
  Take the profile-state G from Phase 1 and test it on all subsets through
  a configured cardinality cap in small A(n,k).  For every V, require:
      G(state(V)) <= max_p [gap(V,p) + sum G(state(F_s))]
  If any V fails, the profile-state candidate is invalid.

Scope: finite certificates only.  Not a universal theorem.
"""

import itertools
from collections import defaultdict

# ---------------------------------------------------------------------------
# Arithmetic
# ---------------------------------------------------------------------------


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


# ---------------------------------------------------------------------------
# Coordinate-profile state
# ---------------------------------------------------------------------------


def compute_state(vertices, k):
    """State(V) = sorted tuple of per-coordinate sorted fiber profiles.

    For each coordinate p, compute sorted nonzero fiber sizes.
    Then sort the list of these per-coordinate profiles.
    """
    profiles = []
    for p in range(k):
        fibers = defaultdict(int)
        for v in vertices:
            fibers[v[p]] += 1
        profiles.append(tuple(sorted(fibers.values())))
    return tuple(sorted(profiles))


# ---------------------------------------------------------------------------
# Set generators
# ---------------------------------------------------------------------------


def star_graph(n, k, size):
    """Radius-one star centered at tuple(range(k))."""
    center = tuple(range(k))
    avail = sorted(neighbors(center, n))
    if size <= 0 or size > 1 + len(avail):
        return None
    return tuple(sorted([center] + avail[: size - 1]))


def all_subsets(n, k, max_r):
    """All k-permutations of {0..n-1} as subsets, up to max_r."""
    verts = list(itertools.permutations(range(n), k))
    result = []
    for r in range(1, min(max_r, len(verts)) + 1):
        for combo in itertools.combinations(verts, r):
            result.append(tuple(sorted(combo)))
    return result


# ---------------------------------------------------------------------------
# DP runner
# ---------------------------------------------------------------------------


def run_dp(n, k, pool):
    """Bottom-up DP on a pool of vertex sets.

    Returns (G_dict, state_info_list, found_negative).
    """
    m = n - k
    by_size = defaultdict(list)
    for V in pool:
        by_size[len(V)].append(V)

    G = {}
    state_info = []
    for V in by_size.get(1, []):
        G[V] = 0
        state_info.append(
            {
                "set": V,
                "state": compute_state(V, k),
                "G": 0,
                "best_p": None,
                "gap": 0,
                "children": None,
            }
        )

    max_R = max(by_size.keys()) if by_size else 0
    found_negative = False

    for R in range(2, max_R + 1):
        if R not in by_size:
            continue
        for V in by_size[R]:
            best_val = -float("inf")
            best_p = None
            best_gap = None
            best_children = None
            phi_V = phi(V, n, k, m)
            P_R = potential(R, m)

            for p_val in range(k):
                fibers = defaultdict(list)
                for v in V:
                    fibers[v[p_val]].append(v)
                if len(fibers) < 2:
                    continue
                children = tuple(tuple(sorted(fv)) for fv in fibers.values())
                sum_phi_F = sum(phi(fv, n, k, m) for fv in fibers.values())
                sum_P_c = sum(potential(len(fv), m) for fv in fibers.values())
                overhead = phi_V - sum_phi_F
                P_surplus = P_R - sum_P_c
                gap = P_surplus - overhead
                val = gap + sum(G[c] for c in children)
                if val > best_val:
                    best_val = val
                    best_p = p_val
                    best_gap = gap
                    best_children = children

            G[V] = best_val
            state = compute_state(V, k)
            state_info.append(
                {
                    "set": V,
                    "state": state,
                    "G": best_val,
                    "best_p": best_p,
                    "gap": best_gap,
                    "children": best_children,
                }
            )

            if best_val < 0 and not found_negative:
                found_negative = True

    return G, state_info, found_negative


def main():
    """Phase 1: Star closure DP.  Phase 2: Global validation."""
    # ================================================================
    # Phase 1: Star closure — DP certificate + state analysis
    # ================================================================
    print("=" * 60)
    print("PHASE 1: Star closure DP")
    print("=" * 60)

    star_state_info = {}
    for n, k in [(4, 2), (4, 3), (5, 2), (5, 3), (6, 3), (7, 4), (8, 4), (8, 5)]:
        m = n - k
        print(f"\n--- A({n},{k}) m={m} ---")

        seeds = []
        max_star = 16 if n <= 5 else 21
        for sz in range(2, max_star):
            sg = star_graph(n, k, sz)
            if sg is not None:
                seeds.append(sg)

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
                    for fv in fibers.values():
                        ft = tuple(sorted(fv))
                        if ft not in pool:
                            pool.add(ft)
                            queue.append(ft)

        G, info, neg = run_dp(n, k, pool)
        star_state_info[(n, k)] = info

        if neg:
            print("  [!] G(V) < 0 found -- exact bound impossible")
        else:
            print("  G(V) >= 0 for all sets in closure")

        if G:
            print(f"  G range: [{min(G.values()):.1f}," f" {max(G.values()):.1f}]")

        # State-consistency check
        state_groups = defaultdict(list)
        for entry in info:
            state_groups[entry["state"]].append(entry)

        inconsistent = 0
        total_multi = 0
        for _state, entries in state_groups.items():
            if len(entries) < 2:
                continue
            total_multi += 1
            g_vals = sorted(set(round(e["G"], 2) for e in entries))
            if g_vals[-1] - g_vals[0] > 0.5:
                inconsistent += 1
                print(
                    f"  INCONSISTENT state: G values = {g_vals}"
                    f" ({len(entries)} sets)"
                )

        if total_multi == 0:
            print("  State analysis: no multi-entry states")
        elif inconsistent == 0:
            print(
                f"  State analysis: G is state-determined"
                f" ({total_multi} states, all consistent)"
            )
        else:
            print(
                f"  State analysis: G is GEOMETRY-DEPENDENT"
                f" ({inconsistent}/{total_multi} inconsistent)"
            )

    # ================================================================
    # Phase 2a: Cross-test learned Star G on non-Star transitions
    # ================================================================
    print("\n" + "=" * 60)
    print("PHASE 2a: Cross-test learned G(state) on non-Star sets")
    print("=" * 60)

    for n, k in [(4, 2), (4, 3), (5, 2)]:
        m = n - k
        print(f"\n--- A({n},{k}) m={m} ---")

        # Build g_state from Phase 1 Star closure for this (n,k)
        g_state_groups = defaultdict(set)
        for entry in star_state_info.get((n, k), []):
            g_state_groups[entry["state"]].add(entry["G"])
        inconsistent_states = {
            state: values
            for state, values in g_state_groups.items()
            if len(values) != 1
        }
        if inconsistent_states:
            print(
                "  Star state table is inconsistent; profile cross-test "
                "is not defined for this configuration"
            )
            continue
        g_state = {
            state: next(iter(values)) for state, values in g_state_groups.items()
        }
        if not g_state:
            print("  No Star states for this config, skipping")
            continue

        print(f"  Learned {len(g_state)} states from Star closure")

        # Enumerate all subsets up to feasible max_r
        max_r = {4: 6, 5: 5}.get(n, 4)
        all_sets = all_subsets(n, k, max_r)
        pool = set(all_sets)

        known_parent = 0
        transition_checked = 0
        passed = 0
        failed = 0
        unknown = 0
        unknown_states = set()
        failed_examples = []

        for V in pool:
            if len(V) < 2:
                continue
            st = compute_state(V, k)
            if st not in g_state:
                unknown_states.add(st)
                continue
            known_parent += 1
            phi_V = phi(V, n, k, m)
            P_R = potential(len(V), m)
            found_known_transition = False
            transition_ok = False
            for p_val in range(k):
                fibers = defaultdict(list)
                for v in V:
                    fibers[v[p_val]].append(v)
                if len(fibers) < 2:
                    continue
                children = tuple(tuple(sorted(fv)) for fv in fibers.values())
                child_states = tuple(compute_state(c, k) for c in children)
                if any(cs not in g_state for cs in child_states):
                    continue
                found_known_transition = True
                sum_phi_F = sum(phi(fv, n, k, m) for fv in fibers.values())
                sum_P_c = sum(potential(len(fv), m) for fv in fibers.values())
                gap = (P_R - sum_P_c) - (phi_V - sum_phi_F)
                child_g = sum(g_state[cs] for cs in child_states)
                if g_state[st] <= gap + child_g:
                    transition_ok = True
                    break
            if transition_ok:
                passed += 1
            elif found_known_transition:
                failed += 1
                if len(failed_examples) < 5:
                    failed_examples.append((V, st, g_state[st]))
            else:
                unknown += 1

            if found_known_transition:
                transition_checked += 1

        print(f"  Known-parent sets: {known_parent}")
        print(
            f"  Passed: {passed}/{transition_checked}"
            " (inequality satisfied by learned G)"
        )
        print(
            f"  Failed: {failed}/{transition_checked}"
            f" (known transition exists but inequality fails)"
        )
        print(
            f"  Unknown-child transitions: {unknown}/{known_parent}"
            " (all child states unknown)"
        )
        print(f"  Unknown states: {len(unknown_states)}")
        if failed_examples:
            for V, st, gval in failed_examples[:3]:
                print(f"  FAIL: |V|={len(V)} state={st}" f" G_learned={gval}")

    # ================================================================
    # Phase 2b: Full-pool DP — state-consistency on expanded set
    # ================================================================
    print("\n" + "=" * 60)
    print("PHASE 2b: Full-pool DP (state consistency on expanded set)")
    print("=" * 60)

    for n, k in [(4, 2), (4, 3), (5, 2)]:
        m = n - k
        max_r = {4: 6, 5: 5}.get(n, 4)
        print(f"\n--- A({n},{k}) m={m} max_r={max_r} ---")

        all_sets = all_subsets(n, k, max_r)
        pool = set(all_sets)
        for V in list(pool):
            if len(V) >= 2:
                for p_val in range(k):
                    fibers = defaultdict(list)
                    for v in V:
                        fibers[v[p_val]].append(v)
                    if len(fibers) > 1:
                        for fv in fibers.values():
                            ft = tuple(sorted(fv))
                            pool.add(ft)

        G, info, neg = run_dp(n, k, pool)
        if neg:
            print("  [!] G(V) < 0 found")
            continue

        print(f"  G range: [{min(G.values()):.1f}," f" {max(G.values()):.1f}]")

        state_groups = defaultdict(list)
        for entry in info:
            state_groups[entry["state"]].append(entry)

        inconsistent = 0
        shown_inconsistencies = 0
        total_multi = 0
        for _state, entries in state_groups.items():
            if len(entries) < 2:
                continue
            total_multi += 1
            g_vals = sorted(set(round(e["G"], 2) for e in entries))
            if g_vals[-1] - g_vals[0] > 0.5:
                inconsistent += 1
                if shown_inconsistencies < 5:
                    print(f"  INCONSISTENT state: G values = {g_vals}")
                    shown_inconsistencies += 1

        if total_multi == 0:
            print("  State analysis: no multi-entry states")
        elif inconsistent == 0:
            print(
                f"  State analysis: G is state-determined"
                f" ({total_multi} states, all consistent)"
            )
        else:
            print(
                f"  State analysis: G is GEOMETRY-DEPENDENT"
                f" ({inconsistent}/{total_multi} inconsistent)"
            )


if __name__ == "__main__":
    main()
