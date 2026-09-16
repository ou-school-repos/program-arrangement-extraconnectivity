#!/usr/bin/env python3
"""Decompose the arithmetic gap dC+m*dE against Lc, unfilled slots, and
explicit binary-carry mass of sum(R_i)=R, focused on the tightest-slack
corpus sets, to test whether "unfilled slot OR carry" is a real
exhaustive pigeonhole or just a name for the already-known aggregate gap.

carry_mass(R_1,...,R_n): ripple-carry accumulate R_1+R_2+...+R_n one
slice at a time; each ripple-carry addition of (running_sum, R_i)
contributes count of bit positions where a carry-out occurs (unweighted,
since the earlier weighted-by-2^j conjecture was refuted). Total carry
mass = sum over all n-1 accumulation steps.

Run:  python3 scripts/carry_decomposition_check.py
"""

from coverage_avoidance import analyze, corpus


def slack(r):
    """Compute global slack: dC + m*dE - (m+1)*conc - Lc."""
    return r["dC"] + r["m"] * r["dE"] - (r["m"] + 1) * r["conc"] - r["Lc"]


def carry_count_single(x, y):
    """Count ripple-carry bit positions in x + y."""
    carry = 0
    cnt = 0
    while x or y or carry:
        s = (x & 1) + (y & 1) + carry
        newcarry = 1 if s >= 2 else 0
        cnt += newcarry
        carry = newcarry
        x >>= 1
        y >>= 1
    return cnt


def carry_mass(Ri_list):
    """Total carry mass from ripple-carry accumulating sum(Ri)."""
    nonzero = [r for r in Ri_list if r > 0]
    if len(nonzero) <= 1:
        return 0
    running = nonzero[0]
    total = 0
    for r in nonzero[1:]:
        total += carry_count_single(running, r)
        running += r
    return total


def main():
    """Run carry decomposition on the coverage_avoidance corpus."""
    results = []
    for tag, A, S in corpus():
        r, rows = analyze(A, S)
        if r["Lc"] <= 0:
            continue
        s = slack(r)
        Ri = [row["Ri"] for row in rows]
        cm = carry_mass(Ri)
        unfilled = (
            r["pi"] - r["R"]
        )  # note: pi >= R always false in general; this is <=0 typically
        results.append((s, tag, r, Ri, cm, unfilled))

    results.sort(key=lambda t: t[0])
    print(
        f"{'slack':>6} {'tag':<16} {'Lc':>4} {'pi-R':>6} {'carry_mass':>10} "
        f"{'dC+mdE':>8} {'(m+1)conc':>10} {'unexplained':>12}"
    )
    for s, tag, r, Ri, cm, unfilled in results[:30]:
        gap = r["dC"] + r["m"] * r["dE"]
        conc_term = (r["m"] + 1) * r["conc"]
        # candidate decomposition: Lc <= (m+1)*(pi-R) + gap is the known-true
        # inequality. (pi-R) is <=0 typically (conc = R-pi >=0), so "unfilled
        # slots" as literally pi-R is not the right sign; use conc's
        # complement or report raw values.
        unexplained = r["Lc"] - cm  # how much of Lc carry mass fails to cover
        print(
            f"{s:6d} {tag:<16} {r['Lc']:4d} {unfilled:6d} {cm:10d} "
            f"{gap:8d} {conc_term:10d} {unexplained:12d}"
        )

    print("\n--- tightest 10 sets: full breakdown ---")
    for s, tag, r, Ri, cm, unfilled in results[:10]:
        print(f"\n{tag}: slack={s}")
        print(f"  R={r['R']} pi={r['pi']} conc={r['conc']} Lc={r['Lc']}")
        print(f"  Ri={Ri}")
        print(
            f"  dC={r['dC']} dE={r['dE']} m={r['m']} "
            f"dC+m*dE={r['dC'] + r['m'] * r['dE']}"
        )
        print(f"  carry_mass(Ri)={cm}")
        gap_cm = r["Lc"] - cm
        print(f"  Lc - carry_mass = {gap_cm}  (>0 means carries don't cover Lc)")
        print(
            f"  m*conc = {r['m'] * r['conc']}  "
            f"Lc - carry_mass - conc = {r['Lc'] - cm - r['conc']}"
        )


if __name__ == "__main__":
    main()
