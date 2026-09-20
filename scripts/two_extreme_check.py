import sys


def load(fn):
    return [tuple(map(int, line.split()[:4])) for line in open(fn) if line[0].isdigit()]


def test(R, fn, K, M):
    sig = [(D, X, p, s - p) for D, X, p, s in load(fn)]
    bad = []
    cells = 0
    for k in range(1, K + 1):
        for m in range(1, M + 1):
            F = [(D, X) for D, X, p, e in sig if p <= k and e <= m]
            if not F:
                continue
            cells += 1

            def cost(t):
                return (R * k - t[0]) * m - t[0] - t[1]

            best = min(map(cost, F))
            B = max(F, key=lambda t: (t[1], t[0]))
            Dm = max(t[0] for t in F)
            A = max([t for t in F if t[0] == Dm], key=lambda t: t[1])
            win = sorted({t for t in F if cost(t) == best})
            if min(cost(A), cost(B)) > best:
                bad.append((k, m, best, win, A, B))
            print(f"  R={R} k={k} m={m}: Phi_conn={best} winners={win} A*={A} B*={B}")
    print(f"R={R} {fn}: {cells} cells, violations {len(bad)}", bad)


R = int(sys.argv[1])
for fn, K, M in [
    (a, int(b), int(c))
    for a, b, c in zip(sys.argv[2::3], sys.argv[3::3], sys.argv[4::3])
]:
    test(R, fn, K, M)
