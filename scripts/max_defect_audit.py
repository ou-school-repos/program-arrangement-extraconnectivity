# Enumerate maximum-defect sets (D=E(R)) by growth through maximum-defect prefixes (valid if shellability holds),
# then compute max collisions X* and compare with Hamming X_H = C(R)-E(R).
import itertools
import sys
import time


def E(R):
    return sum(bin(i).count("1") for i in range(R))


def C(R):
    return (R - 1) + sum(i.bit_length() for i in range(R)) - E(R)


RMAX = int(sys.argv[1])
K = int(sys.argv[2])


def D(S):
    return len(S) * K - sum(len({w[:q] + w[q + 1 :] for w in S}) for q in range(K))


def X(S):
    syms = set(x for w in S for x in w)
    N = max(syms) + 2
    m = N - K
    ext = set()
    for u in S:
        for i in range(K):
            for y in range(N):
                if y not in u:
                    w = u[:i] + (y,) + u[i + 1 :]
                    if w not in S:
                        ext.add(w)
    U = sum(len({w[:q] + w[q + 1 :] for w in S}) for q in range(K))
    return U * (m + 1) - len(S) * K - len(ext)


def canon(S):
    best = None
    for perm in itertools.permutations(range(K)):
        T = [tuple(w[p] for p in perm) for w in S]
        # relabel symbols by first appearance in lexicographically sorted order, iterate to fixpoint-ish
        T.sort()
        mp = {}
        for w in T:
            for x in w:
                if x not in mp:
                    mp[x] = len(mp)
        T = tuple(sorted(tuple(mp[x] for x in w) for w in T))
        if best is None or T < best:
            best = T
    return best


start = (tuple(range(K)),)
level = {canon(start)}
for t in range(1, RMAX):
    t0 = time.time()
    nxt = set()
    target = E(t + 1)
    for S in level:
        Sset = set(S)
        syms = sorted(set(x for w in S for x in w))
        fresh = max(syms) + 1
        for u in S:
            for i in range(K):
                for x in syms + [fresh]:
                    if x in u:
                        continue
                    v = u[:i] + (x,) + u[i + 1 :]
                    if v in Sset:
                        continue
                    T = S + (v,)
                    if D(T) == target:
                        nxt.add(canon(T))
    level = nxt
    R = t + 1
    xs = [X(S) for S in level]
    print(
        f"R={R}: {len(level)} max-defect classes (canonical-ish), max X={max(xs)}, Hamming X=C-E={C(R) - E(R)}, time {time.time() - t0:.1f}s",
        flush=True,
    )
