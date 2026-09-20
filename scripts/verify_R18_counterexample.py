"""Verify that the balanced rook-star in A(22,17) has
a larger direct boundary than the embedded Hamming ball."""

n, k, m = 22, 17, 5
c = tuple(range(k))
free = list(range(k, n))
S: set[tuple[int, ...]] = {c} | {
    c[:i] + (free[i % m],) + c[i + 1 :] for i in range(k)
}  # 17 arms, distinct coordinates,
# symbols round-robin over 5 free symbols
N = {
    u[:i] + (y,) + u[i + 1 :]
    for u in S
    for i in range(k)
    for y in range(n)
    if y not in u
} - S
print(
    "R=",
    len(S),
    " direct boundary of balanced rook-star in A(22,17):",
    len(N),
)

# Hamming ball of size 18: binary-order initial segment
# on d=5 coordinates, bit j flips coordinate j to fresh
# symbol free[j]
H = set()
for x in range(18):
    v = list(c)
    for j in range(5):
        if x >> j & 1:
            v[j] = free[j]
    H.add(tuple(v))
NH = {
    u[:i] + (y,) + u[i + 1 :]
    for u in H
    for i in range(k)
    for y in range(n)
    if y not in u
} - H
print(
    "direct boundary of embedded Hamming ball:",
    len(NH),
)
