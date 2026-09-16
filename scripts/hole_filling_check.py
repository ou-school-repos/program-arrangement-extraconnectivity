#!/usr/bin/env python3
"""Hole-filling induction check (Bridge 1, reversed polarity).

Tests the marginal step proposed for the "fill the hole" induction:
starting from any S with L_c > 0, pick a swallowed slot (i, x) (x in
P_i \\ Q_i, x swallowed) and add v = (i,) + x to get S' = S u {v}.

Goal of the induction: prove Slack(S) >= 0 for arbitrary S by walking
S = S_0 -> S_1 -> ... -> S_final (saturated, L_c = 0, Slack >= 0 known)
one hole-fill at a time. This only proves what we want if slack is
*non-increasing* along the walk:

    Delta Slack = Slack(S') - Slack(S) <= 0   at every step

because then Slack(S) = Slack(S_0) >= Slack(S_1) >= ... >= Slack(S_final) >= 0.
(The other polarity, Delta Slack >= 0, would only give an upper bound on
Slack(S) and is useless here.) So "safe" = Delta Slack <= 0, matching
best_delta <= 0 below.

In terms of the marginal quantities:

    alpha = Lc(S) - Lc(S')  (collision mass destroyed; can be negative!)
    beta  = (dC(S') + m*dE(S')) - (dC(S) + m*dE(S))  (arithmetic refund)
    Delta Slack = beta - (m+1) + alpha   (since conc always +1 on a fill)

"safe" requires beta + alpha <= m + 1.

Run:  python3 scripts/hole_filling_check.py
"""

from collections import Counter

from coverage_avoidance import analyze, corpus
from lib import swallowed_boundary


def slack(r):
    """Return the weighted slack recorded in an analysis result."""
    return r["dC"] + r["m"] * r["dE"] - (r["m"] + 1) * r["conc"] - r["Lc"]


def swallowed_slots(A, S, c=0):
    """Recompute the swallowed (i, x) pairs directly (mirrors analyze())."""
    n, k = A.n, A.k
    rest = [p for p in range(k) if p != c]
    pi = {tuple(v[p] for p in rest) for v in S}
    Q = {i: set() for i in range(n)}
    for v in S:
        Q[v[c]].add(tuple(v[p] for p in rest))

    P = {i: {x for x in pi if i not in set(x)} for i in range(n)}
    blo = swallowed_boundary(Q, n)
    out = []
    for i in range(n):
        for x in P[i] - Q[i]:
            if x in blo[i]:
                out.append((i,) + x)
    return out


def all_steps(
    total_steps,
    delta_hist,
    alpha_hist,
    beta_ge_0,
    violations_pi_unchanged,
    min_delta,
    min_ctx,
    tag,
    A,
    S,
    r0,
):
    """Evaluate every hole-fill step for one set; return per-set best (min) delta."""
    pi0 = r0["pi"]
    holes = swallowed_slots(A, S)
    best_delta = None
    for v in holes:
        S1 = S | {v}
        r1, _ = analyze(A, S1)
        alpha = r0["Lc"] - r1["Lc"]
        beta = (r1["dC"] + r0["m"] * r1["dE"]) - (r0["dC"] + r0["m"] * r0["dE"])
        delta = slack(r1) - slack(r0)
        total_steps += 1
        delta_hist[delta >= 0] += 1
        alpha_hist[alpha] += 1
        if beta >= 0:
            beta_ge_0 += 1
        if r1["pi"] != pi0:
            violations_pi_unchanged += 1
        if min_delta is None or delta < min_delta:
            min_delta = delta
            min_ctx = (tag, v, alpha, beta, r0, r1)
        if best_delta is None or delta < best_delta:
            best_delta = delta
    return (
        total_steps,
        delta_hist,
        alpha_hist,
        beta_ge_0,
        violations_pi_unchanged,
        min_delta,
        min_ctx,
        best_delta,
        len(holes),
    )


def main():
    """Run the exhaustive hole-filling marginal check."""
    total_steps = 0
    min_delta = None
    min_ctx = None
    delta_hist = Counter()
    beta_ge_0 = 0
    alpha_hist = Counter()
    violations_pi_unchanged = 0

    # existence (greedy) check: does every Lc>0 set have >=1 safe hole
    # (delta_slack <= 0, i.e. slack does not increase going S -> S')?
    sets_with_holes = 0
    sets_with_safe_hole = 0
    exist_fail = []

    for tag, A, S in corpus():
        r0, _ = analyze(A, S)
        if r0["Lc"] <= 0:
            continue
        S = set(S)
        holes = swallowed_slots(A, S)
        if not holes:
            continue
        (
            total_steps,
            delta_hist,
            alpha_hist,
            beta_ge_0,
            violations_pi_unchanged,
            min_delta,
            min_ctx,
            best_delta,
            _nh,
        ) = all_steps(
            total_steps,
            delta_hist,
            alpha_hist,
            beta_ge_0,
            violations_pi_unchanged,
            min_delta,
            min_ctx,
            tag,
            A,
            S,
            r0,
        )
        sets_with_holes += 1
        if best_delta <= 0:
            sets_with_safe_hole += 1
        else:
            exist_fail.append((tag, best_delta, r0))

    print(f"hole-fill steps tested: {total_steps}")
    print(f"delta_slack >= 0: {delta_hist[True]}  delta_slack < 0: {delta_hist[False]}")
    print(f"alpha (Lc drop) histogram: {dict(sorted(alpha_hist.items()))}")
    print(f"beta >= 0 in {beta_ge_0}/{total_steps} steps")
    print(
        f"|pi| changed on hole-fill (predicted impossible): {violations_pi_unchanged}"
    )
    if min_ctx:
        tag, v, alpha, beta, r0, r1 = min_ctx
        print(f"\nworst single step: tag={tag} v={v} alpha={alpha} beta={beta}")
        print(f"  before: R={r0['R']} pi={r0['pi']} Lc={r0['Lc']} slack={slack(r0)}")
        print(f"  after:  R={r1['R']} pi={r1['pi']} Lc={r1['Lc']} slack={slack(r1)}")
        print(f"  delta_slack = {slack(r1) - slack(r0)}")

    print("\n--- existence (greedy) check ---")
    print(f"sets with Lc>0 and >=1 hole: {sets_with_holes}")
    print(f"sets with >=1 safe hole (best delta_slack <= 0): {sets_with_safe_hole}")
    print(
        f"sets with NO safe hole (every hole-fill increases slack): {len(exist_fail)}"
    )
    for tag, best_delta, r0 in exist_fail[:15]:
        print(
            f"  EXIST-FAIL {tag}: R={r0['R']} pi={r0['pi']} Lc={r0['Lc']} "
            f"slack={slack(r0)} best_achievable_delta={best_delta}"
        )


if __name__ == "__main__":
    main()
