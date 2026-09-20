"""Test shellability: does every maximum-defect connected R-set
have an ordering whose every prefix is maximum-defect?"""

import itertools
import sys
from functools import lru_cache


def E(r):
    """Binary digit sum (popcount sum) for R."""
    return sum(bin(i).count("1") for i in range(r))


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


def D(s):
    """Defect of a set of vertex indices."""
    w = [V[i] for i in s]
    return len(w) * K - sum(len({w2[:q] + w2[q + 1 :] for w2 in w}) for q in range(K))


Et = [E(t) for t in range(R + 1)]
stats = [0, 0]  # [maxd, bad]
seen: set[int] = set()


@lru_cache(None)
def shellable(s):
    """Return True if the vertex-set s is shellable."""
    if len(s) <= 1:
        return True
    return any(D(s - {v}) == Et[len(s) - 1] and shellable(s - {v}) for v in s)


def rec(s, ext, sset, ns):
    """Depth-first growth of connected R-sets through root 0."""
    if len(s) == R:
        fs = frozenset(s)
        if D(fs) == Et[R]:
            stats[0] += 1
            if not shellable(fs):
                stats[1] += 1
                print(
                    "NON-SHELLABLE",
                    [V[i] for i in s],
                )
        return
    ext = list(ext)
    while ext:
        w = ext.pop()
        nb = {x for x in adj[w] if x > 0 and x not in sset and x not in ns}
        sset.add(w)
        rec(
            s + [w],
            set(ext) | nb,
            sset,
            ns | adj[w],
        )
        sset.discard(w)


rec([0], set(adj[0]), {0}, set(adj[0]))
print(
    f"R={R} host A({N},{K}): "
    f"max-defect connected sets through root={stats[0]}, "
    f"non-shellable={stats[1]}"
)
