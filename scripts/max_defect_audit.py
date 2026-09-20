"""Enumerate maximum-defect sets (D=E(R)) by growth through
maximum-defect prefixes (valid if shellability holds), then compute
max collisions X* and compare with Hamming X_H = C(R)-E(R)."""

import itertools
import sys
import time

# pylint: disable=redefined-outer-name


def E(r):
    """Binary digit sum (popcount sum) for R."""
    return sum(bin(i).count("1") for i in range(r))


def C(r):
    """Hamming capacity for R."""
    return (r - 1) + sum(i.bit_length() for i in range(r)) - E(r)


RMAX = int(sys.argv[1])
K = int(sys.argv[2])


def D(s):
    """Defect of a set of K-tuples."""
    return len(s) * K - sum(
        len({w[:q] + w[q + 1:] for w in s}) for q in range(K)
    )


def X(s):
    """Collision count for a set of K-tuples."""
    syms = set(x for w in s for x in w)
    N = max(syms) + 2
    m = N - K
    ext = set()
    for u in s:
        for i in range(K):
            for y in range(N):
                if y not in u:
                    w = u[:i] + (y,) + u[i + 1:]
                    if w not in s:
                        ext.add(w)
    U = sum(
        len({w[:q] + w[q + 1:] for w in s})
        for q in range(K)
    )
    return U * (m + 1) - len(s) * K - len(ext)


def canon(s):
    """Canonical representative under coordinate permutation
    and symbol relabelling."""
    best = None
    for perm in itertools.permutations(range(K)):
        t = [tuple(w[p] for p in perm) for w in s]
        t.sort()
        mp = {}
        for w in t:
            for x in w:
                if x not in mp:
                    mp[x] = len(mp)
        t = tuple(
            sorted(tuple(mp[x] for x in w) for w in t)
        )
        if best is None or t < best:
            best = t
    return best


start = (tuple(range(K)),)
level = {canon(start)}
for t in range(1, RMAX):
    t0 = time.time()
    nxt = set()
    target = E(t + 1)
    for s in level:
        sset = set(s)
        syms = sorted(set(x for w in s for x in w))
        fresh = max(syms) + 1
        for u in s:
            for i in range(K):
                for x in syms + [fresh]:
                    if x in u:
                        continue
                    v = u[:i] + (x,) + u[i + 1:]
                    if v in sset:
                        continue
                    t_val = s + (v,)
                    if D(t_val) == target:
                        nxt.add(canon(t_val))
    level = nxt
    r = t + 1
    xs = [X(s) for s in level]
    print(
        f"R={r}: {len(level)} max-defect classes, "
        f"max X={max(xs)}, "
        f"Hamming X=C-E={C(r) - E(r)}, "
        f"time {time.time() - t0:.1f}s",
        flush=True,
    )
