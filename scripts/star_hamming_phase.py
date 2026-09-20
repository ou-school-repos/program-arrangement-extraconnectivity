"""Verify star-Hamming phase transitions and rook-star
dominance over the Hamming ball across parameter regimes."""

import math
from math import comb

# pylint: disable=redefined-outer-name


def E(r):
    """Binary digit sum (popcount sum) for R."""
    return sum(bin(i).count("1") for i in range(r))


def C(r):
    """Hamming capacity for R."""
    return (r - 1) + sum(
        i.bit_length() for i in range(r)
    ) - E(r)


def bal(n, parts):
    """Balanced allocation of n items into parts bins."""
    q, rem = divmod(n, parts)
    return [q + 1] * rem + [q] * (parts - rem)


def rookX(r, k, m):
    """Rook-star collision count; None if infeasible."""
    n = r - 1
    if n > k * m:
        return None
    return (
        comb(n, 2)
        - sum(comb(x, 2) for x in bal(n, min(k, n)))
        - sum(comb(x, 2) for x in bal(n, m))
    )


def load(fn):
    """Load signature catalogue from file."""
    return [
        (D, X, p, s - p)
        for D, X, p, s in (
            tuple(map(int, line.split()[:4]))
            for line in open(fn, encoding="utf-8")
            if line[:1].isdigit()
        )
    ]


# 1) does B* from the catalogues equal the rook-star formula
#    whenever a star fits?
tot = ok = 0
for R, fn, K, M in [
    (5, "r5.txt", 8, 9),
    (6, "r6.txt", 8, 9),
    (7, "r7_8_3.txt", 3, 5),
    (7, "r7_7_4.txt", 4, 3),
    (8, "r8_6_3.txt", 3, 3),
]:
    sig = load(fn)
    for k in range(1, K + 1):
        for m in range(1, M + 1):
            F = [
                (D, X)
                for D, X, p, e in sig
                if p <= k and e <= m
            ]
            rx = rookX(R, k, m)
            if not F or rx is None:
                continue
            B = max(F, key=lambda t: (t[1], t[0]))
            tot += 1
            ok += B == (R - 1, rx)
            if B != (R - 1, rx):
                print("mismatch", R, k, m, B, rx)
print(
    f"B* = balanced rook-star (D=R-1, X=rook formula)"
    f" in {ok}/{tot} star-feasible cells"
)

# 2) smallest R where the rook-star strictly beats the
#    Hamming ball with the gate open (k,m <= 60)
hits = []
for R in range(2, 300):
    d = (R - 1).bit_length()
    XH = C(R) - E(R)
    for k in range(d, 61):
        for m in range(d, 61):
            rx = rookX(R, k, m)
            if rx is None:
                continue
            gate = (E(R) - (R - 1)) * (m + 1)
            if gate < rx - XH:
                hits.append(
                    (
                        R,
                        k,
                        m,
                        (R * k - E(R)) * m - C(R),
                        (R * k - R + 1) * m
                        - (R - 1)
                        - rx,
                    )
                )
    if hits:
        break
print(
    "first gate-open R where a star beats Hamming:",
    hits[:6],
)
print(
    "\nTall regime (k >= R-1): m-values"
    " (gate open, m>=d) where the balanced star"
    " beats the Hamming ball;"
    " t(N,m)=Turan number"
)

for R in [18, 20, 24, 32, 48, 64, 100, 128, 256]:
    d = (R - 1).bit_length()
    k = R - 1
    w = [
        m
        for m in range(d, 2000)
        if (E(R) - (R - 1)) * (m + 1)
        < rookX(R, k, m) - (C(R) - E(R))
    ]
    approx = R / (math.log2(R) - 2)
    mw = f"[{min(w)},{max(w)}]" if w else "none"
    hstart = max(w) + 1 if w else d
    print(
        f"R={R:4d} d={d}: star wins for m in {mw}"
        f"  (Hamming from m={hstart};"
        f" R/(log2 R - 2)={approx:.1f})"
    )
