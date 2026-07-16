"""Exhaustive verification of every lemma in the CrossTop decomposition,
against a faithful model of the repo's definitions (validated by reproducing
HBCrossCollisions exactly on A(7,3), A(8,4), A(9,4) for all feasible R).

Definitions modeled:
  popcount / bit_length (Nat.size) / E_seq / sum_bit_length / C_constant
  embed_vertex(i)_p = k+p if bit p of i set (p<d) else p ;  p for p >= d
  coord_boundary V p = { w outside V : drop_pos w p = drop_pos v p, some v in V }
  total_coord_edges  = sum over p of |coord_boundary V p|
  external_neighbors = |union over p of coord_boundary V p|
  cross_collisions   = total_coord_edges - external_neighbors
"""


def popcount(x):
    return bin(x).count("1")


def bit_length(x):
    return x.bit_length()


def E_seq(m):
    return sum(popcount(i) for i in range(m))


def sum_bit_length(m):
    return sum(bit_length(i) for i in range(m))


def C_constant(R):
    return 0 if R == 0 else (R - 1) + sum_bit_length(R) - E_seq(R)


def embed_vertex(n, k, d, i):
    return tuple((k + p if (i >> p) & 1 else p) if p < d else p for p in range(k))


def hamming_ball(R, n, k, d):
    return set(embed_vertex(n, k, d, i) for i in range(R))


def coord_boundary(V, n, k, p):
    out = set()
    for v in V:
        used = set(v) - {v[p]}
        for s in range(n):
            if s in used:
                continue
            w = v[:p] + (s,) + v[p + 1 :]
            if w not in V:
                out.add(w)
    return out


def cross_collisions(V, n, k):
    Bs = [coord_boundary(V, n, k, p) for p in range(k)]
    total = sum(len(B) for B in Bs)
    U = set().union(*Bs)
    return total - len(U)


def deg(m, D, j):
    return sum(1 for p in range(D) if (j ^ (1 << p)) < m)


def main():
    # 0. Model faithfulness: reproduce HBCrossCollisions exactly.
    for n, k in [(7, 3), (8, 4), (9, 4)]:
        for R in range(1, 2 ** min(4, k) + 1):
            d = bit_length(R - 1)
            if d > k or k + d > n:
                continue
            assert cross_collisions(hamming_ball(R, n, k, d), n, k) + E_seq(
                R
            ) == C_constant(R), (n, k, R)
    print("0. model reproduces HBCrossCollisions: OK")

    # CrossRecurrence as stated in the repo
    # (true, but see analysis: circular interface).
    def ext_cube(d, m):
        return max(max(m * (d - 1) - E_seq(m), 0) - C_constant(m), 0)

    for n, k in [(8, 4), (9, 4), (11, 5)]:
        for R in range(2, 2 ** min(4, k) + 1):
            d = bit_length(R - 1)
            P = 2 ** (d - 1)
            m = R - P
            if d > k or k + d > n or not (0 < m <= P):
                continue
            cR = cross_collisions(hamming_ball(R, n, k, d), n, k)
            cP = cross_collisions(hamming_ball(P, n, k, d), n, k)
            cm = cross_collisions(hamming_ball(m, n, k, d), n, k)
            assert cR == cP + cm + ext_cube(d, m), (n, k, R)
            assert cP == 0  # the P-term is always identically zero
    print("1. CrossRecurrence (as stated) + cP==0: OK")

    # CrossTop closed form: cross(HB R,d) + 2*E_seq(m) == m*(d-1).
    for n, k in [(8, 4), (9, 4), (11, 5), (12, 5)]:
        for R in range(2, 2 ** min(4, k) + 1):
            d = bit_length(R - 1)
            P = 2 ** (d - 1)
            m = R - P
            if d > k or k + d > n or not (0 < m <= P):
                continue
            assert cross_collisions(hamming_ball(R, n, k, d), n, k) + 2 * E_seq(
                m
            ) == m * (d - 1), (n, k, R)
    print("2. CrossTop closed form: OK")

    # Layer A: sum_{j in [m,2^D)} deg(m,D,j) + 2*E_seq(m) == m*D, all m<=2^D.
    for D in range(0, 8):
        for m in range(0, 2**D + 1):
            assert (
                sum(deg(m, D, j) for j in range(m, 2**D)) + 2 * E_seq(m) == m * D
            ), (D, m)
    print("3. Layer A (sum_ball_deg): OK")

    # 3b. Layer A ingredients.
    for D in range(0, 8):
        for m in range(0, 2**D):
            assert deg(m, D, m) == popcount(m)  # ball_deg_self
            assert (
                sum(1 for p in range(D) if not (m >> p) & 1) + popcount(m) == D
            )  # card_up_neighbors
    print("3b. ball_deg_self / card_up_neighbors: OK")

    # Layer B bridge: cross == sum over j in [t,2^d) with deg>=1 of (deg-1), all t.
    n, k = 9, 4
    for d in range(0, 5):
        if d > k or k + d > n:
            continue
        for t in range(1, 2**d + 1):
            c = cross_collisions(hamming_ball(t, n, k, d), n, k)
            s = sum(deg(t, d, j) - 1 for j in range(t, 2**d) if deg(t, d, j) >= 1)
            assert c == s, (d, t, c, s)
    print("4. Layer B (cross_collisions_eq_cube_sum): OK")

    # Layer C: reindex + full-strip facts.
    for d in range(1, 7):
        P = 2 ** (d - 1)
        for m in range(1, P + 1):
            R = P + m
            for jp in range(m, P):
                assert deg(R, d, P + jp) == 1 + deg(m, d - 1, jp), (
                    d,
                    m,
                    jp,
                )  # ball_deg_top
            assert all(deg(R, d, j) >= 1 for j in range(R, 2**d))  # strip is fully hit
    print("5. Layer C (ball_deg_top, full strip): OK")

    # Arithmetic used by the new driver.
    for j in range(12):
        assert 2 * E_seq(2**j) == j * 2**j  # E_seq_pow
    for R in range(0, 200):
        assert C_constant(R) + E_seq(R) == (max(R - 1, 0)) + sum_bit_length(R)
    print("6. E_seq_pow / C_constant_add_E_seq: OK")

    # The circularity fact: ext_cube(d,m) equals HB(m)'s vertex boundary in Q_(d-1)
    #    exactly because HBCrossCollisions(m) holds — i.e. proving CrossRecurrence
    #    directly re-derives the target theorem at m.
    for d in range(1, 7):
        for m in range(1, 2 ** (d - 1) + 1):
            edgeb = m * (d - 1) - 2 * E_seq(m)
            crossm = edgeb - (
                len(
                    {
                        j ^ (1 << p)
                        for j in range(m)
                        for p in range(d - 1)
                        if (j ^ (1 << p)) >= m
                    }
                )
            )
            assert crossm == C_constant(m) - E_seq(m), (d, m)  # == HBCrossCollisions(m)
            assert ext_cube(d, m) == edgeb - crossm
    print("7. ext_cube(d,m) == vertex boundary  ⟺  HBCrossCollisions(m): OK")
    print("\nAll checks passed.")


if __name__ == "__main__":
    main()
