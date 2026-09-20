# Test "shellability": does every maximum-defect connected R-set have an ordering whose every prefix is maximum-defect?
import itertools
import sys
from functools import lru_cache


def E(R):
    return sum(bin(i).count("1") for i in range(R))


R = int(sys.argv[1])
N = int(sys.argv[2])
K = int(sys.argv[3])
V = list(itertools.permutations(range(N), K))
idx = {v: i for i, v in enumerate(V)}
adj = [
    set(
        idx[u[:i] + (x,) + u[i + 1 :]] for i in range(K) for x in range(N) if x not in u
    )
    for u in V
]


def D(S):
    W = [V[i] for i in S]
    return len(W) * K - sum(len({w[:q] + w[q + 1 :] for w in W}) for q in range(K))


Et = [E(t) for t in range(R + 1)]
maxd = 0
bad = 0
seen = set()


@lru_cache(None)
def shellable(S):
    if len(S) <= 1:
        return True
    return any(D(S - {v}) == Et[len(S) - 1] and shellable(S - {v}) for v in S)


def rec(S, ext, Sset, NS):
    global maxd, bad
    if len(S) == R:
        fs = frozenset(S)
        if D(fs) == Et[R]:
            maxd += 1
            if not shellable(fs):
                bad += 1
                print("NON-SHELLABLE", [V[i] for i in S])
        return
    ext = list(ext)
    while ext:
        w = ext.pop()
        nb = {x for x in adj[w] if x > 0 and x not in Sset and x not in NS}
        Sset.add(w)
        rec(S + [w], set(ext) | nb, Sset, NS | adj[w])
        Sset.discard(w)


rec([0], set(adj[0]), {0}, set(adj[0]))
print(
    f"R={R} host A({N},{K}): max-defect connected sets through root={maxd}, non-shellable={bad}"
)
