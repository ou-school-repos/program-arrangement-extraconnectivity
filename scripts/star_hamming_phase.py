import math
from math import comb


def E(R):
    return sum(bin(i).count("1") for i in range(R))


def C(R):
    return (R - 1) + sum(i.bit_length() for i in range(R)) - E(R)


def bal(N, parts):
    q, r = divmod(N, parts)
    return [q + 1] * r + [q] * (parts - r)


def rookX(R, k, m):
    N = R - 1
    if N > k * m:
        return None
    return (
        comb(N, 2)
        - sum(comb(x, 2) for x in bal(N, min(k, N)))
        - sum(comb(x, 2) for x in bal(N, m))
    )


# 1) does B* from the catalogues equal the rook-star formula whenever a star fits?
def load(fn):
    return [
        (D, X, p, s - p)
        for D, X, p, s in (
            tuple(map(int, line.split()[:4])) for line in open(fn) if line[:1].isdigit()
        )
    ]


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
            F = [(D, X) for D, X, p, e in sig if p <= k and e <= m]
            rx = rookX(R, k, m)
            if not F or rx is None:
                continue
            B = max(F, key=lambda t: (t[1], t[0]))
            tot += 1
            ok += B == (R - 1, rx)
            if B != (R - 1, rx):
                print("mismatch", R, k, m, B, rx)
print(
    f"B* = balanced rook-star (D=R-1, X=rook formula) in {ok}/{tot} star-feasible cells"
)
# 2) smallest R where the rook-star strictly beats the Hamming ball with the gate open (k,m <= 60)
hits = []
for R in range(2, 300):
    d = (R - 1).bit_length()
    XH = C(R) - E(R)
    for k in range(d, 61):
        for m in range(d, 61):
            rx = rookX(R, k, m)
            if rx is None:
                continue
            if (E(R) - (R - 1)) * (m + 1) < rx - XH:
                hits.append(
                    (
                        R,
                        k,
                        m,
                        (R * k - E(R)) * m - C(R),
                        (R * k - R + 1) * m - (R - 1) - rx,
                    )
                )
    if hits:
        break
print("first gate-open R where a star beats Hamming:", hits[:6])
print(
    "\nTall regime (k >= R-1): m-values (gate open, m>=d) where the balanced star beats the Hamming ball; t(N,m)=Turan number"
)
for R in [18, 20, 24, 32, 48, 64, 100, 128, 256]:
    d = (R - 1).bit_length()
    k = R - 1
    w = [
        m
        for m in range(d, 2000)
        if (E(R) - (R - 1)) * (m + 1) < rookX(R, k, m) - (C(R) - E(R))
    ]
    approx = R / (math.log2(R) - 2)
    print(
        f"R={R:4d} d={d}: star wins for m in {'[' + str(min(w)) + ',' + str(max(w)) + ']' if w else 'none'}  (Hamming from m={max(w) + 1 if w else d}; R/(log2 R - 2)={approx:.1f})"
    )
