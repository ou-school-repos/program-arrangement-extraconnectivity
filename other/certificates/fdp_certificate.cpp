// Sound upper bound F_{m,k}(R) >= max{ Q_m(S) : S subset of A(m+k,k), |S| = R
// }, by induction on k with m fixed.  See the header comment block in the
// accompanying note for the derivation of each constraint.
//
//   Q(S) = (m+1) d_p + sum_c [ Q'(S_c) + L_c ]   for EVERY coordinate p (exact)
//   d_p = R - U_p,   Q'(S_c) <= F_{m,k-1}(a_c) (induction) L_c <= U_p - a_c
//   (pool envelope, relaxed) Q'(S_c) + L_c <= m(k-1) a_c (local boundary cap)
//   sum_c L_c <= U_p * min(m, s-1) - d_p,  s = #nonempty slices  (line-slot
//   capacity) sum_p d_p = D(S) <= E(R)                           (Lean:
//   sum_unique_roots_lower_bound)
//
// Usage: fdp_certificate m max_k max_R [--detail]
// Output: for each k, the R values with F <= G (each such R is certified
// independently: Q <= G for every |S| = R in A(m+k,k)), and the uncertified R
// with their excess F - G.  --detail prints every F.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <numeric>
#include <vector>

using i64 = std::int64_t;
constexpr i64 NEG = std::numeric_limits<i64>::min() / 4;

i64 E(int r) {
    i64 s = 0;
    for (int i = 0; i < r; ++i)
        s += __builtin_popcount(unsigned(i));
    return s;
}
i64 C(int r) {
    if (r == 0)
        return 0;
    i64 s = r - 1 - E(r);
    for (int i = 1; i < r; ++i)
        s += 32 - __builtin_clz(unsigned(i));
    return s;
}
i64 G(int m, int r) { return C(r) + m * E(r); }
i64 floor_div(i64 a, i64 b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

// max over x in [0, X] of the upper concave envelope of (d, phi[d]), X =
// num/den.
i64 hull_max_upto(const std::vector<i64> &phi, i64 num, i64 den) {
    std::vector<int> pts;
    for (int d = 0; d < (int)phi.size(); ++d) {
        if (phi[d] <= NEG)
            continue;
        while (pts.size() >= 2) {
            int a = pts[pts.size() - 2], b = pts.back();
            // remove b if it lies on or below segment a-d
            if ((phi[b] - phi[a]) * (d - a) <= (phi[d] - phi[a]) * (b - a))
                pts.pop_back();
            else
                break;
        }
        pts.push_back(d);
    }
    if (pts.empty() || pts.front() * den > num)
        return NEG; // no feasible defect <= X
    i64 best = NEG;
    for (size_t i = 0; i < pts.size(); ++i) {
        int a = pts[i];
        if (a * den <= num)
            best = std::max(best, phi[a]);
        if (i + 1 < pts.size()) {
            int b = pts[i + 1];
            if (a * den < num &&
                num < b * den) { // X strictly inside segment a-b
                i64 v = floor_div(phi[a] * (b * den - num) +
                                      phi[b] * (num - a * den),
                                  (i64)(b - a) * den);
                best = std::max(best, v);
            }
        }
    }
    return best;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s m max_k max_R [--detail]\n", argv[0]);
        return 2;
    }
    const int m = std::atoi(argv[1]), K = std::atoi(argv[2]),
              RM = std::atoi(argv[3]);
    const bool detail = argc > 4 && std::strcmp(argv[4], "--detail") == 0;
    if (m < 1 || K < 1 || RM < 1 || RM > 4000) {
        std::fprintf(stderr, "bad arguments\n");
        return 2;
    }

    std::vector<i64> prev(RM + 1, NEG); // k = 0: A(m,0) is a single vertex
    prev[0] = 0;
    prev[1] = 0;
    for (int k = 1; k <= K; ++k) {
        const int n = m + k;
        const int P = std::min(n, RM);
        // phi[R][d]: best RHS given coordinate defect d (U = R - d)
        std::vector<std::vector<i64>> phi(RM + 1);
        for (int R = 0; R <= RM; ++R)
            phi[R].assign(std::max(R, 1), NEG);
        std::vector<i64> f(RM + 1), h(RM + 1);
        std::vector<std::vector<i64>> dpH(P + 1, std::vector<i64>(RM + 1)),
            dpF = dpH;
        for (int U = 1; U <= RM; ++U) {
            for (int a = 1; a <= U; ++a) {
                if (prev[a] <= NEG) {
                    f[a] = h[a] = NEG;
                    continue;
                }
                const i64 M = (i64)m * (k - 1) * a;
                f[a] = std::min(prev[a], M);
                const i64 l = std::max<i64>(0, std::min<i64>(U - a, M - f[a]));
                h[a] = f[a] + l;
            }
            const int top = std::min(RM, (m + 1) * U);
            for (auto &row : dpH)
                std::fill(row.begin(), row.end(), NEG);
            for (auto &row : dpF)
                std::fill(row.begin(), row.end(), NEG);
            dpH[0][0] = dpF[0][0] = 0;
            for (int p = 1; p <= P; ++p)
                for (int s = p; s <= top; ++s) {
                    i64 bh = NEG, bf = NEG;
                    for (int a = 1; a <= std::min(U, s); ++a) {
                        if (f[a] <= NEG)
                            continue;
                        if (dpH[p - 1][s - a] > NEG)
                            bh = std::max(bh, dpH[p - 1][s - a] + h[a]);
                        if (dpF[p - 1][s - a] > NEG)
                            bf = std::max(bf, dpF[p - 1][s - a] + f[a]);
                    }
                    dpH[p][s] = bh;
                    dpF[p][s] = bf;
                }
            for (int R = U; R <= top; ++R) {
                const int d = R - U;
                i64 best = NEG;
                for (int p = 1; p <= P; ++p) {
                    if (dpH[p][R] <= NEG)
                        continue;
                    const i64 budget = (i64)U * std::min(m, p - 1) - d;
                    if (budget < 0)
                        continue; // sum of L_c >= 0 cannot fit: no such
                                  // configuration
                    best =
                        std::max(best, std::min(dpH[p][R], dpF[p][R] + budget));
                }
                if (best > NEG)
                    phi[R][d] = std::max(phi[R][d], (i64)(m + 1) * d + best);
            }
        }
        std::vector<i64> cur(RM + 1, NEG);
        cur[0] = 0;
        for (int R = 1; R <= RM; ++R) {
            const i64 single =
                std::accumulate(phi[R].begin(), phi[R].end(), NEG,
                                [](const i64 current, const i64 value) {
                                    return std::max(current, value);
                                });
            if (single <= NEG)
                continue;
            const i64 averaged =
                hull_max_upto(phi[R], E(R), k); // mean defect <= E(R)/k
            cur[R] = std::min(single, averaged);
        }
        int certified = 0, last_feasible = 0;
        std::vector<int> open;
        for (int R = 1; R <= RM; ++R) {
            if (cur[R] <= NEG)
                continue;
            last_feasible = R;
            if (cur[R] <= G(m, R))
                ++certified;
            else
                open.push_back(R);
        }
        std::printf(
            "m=%d k=%d: certified %d of %d DP-feasible R <= %d; uncertified R:",
            m, k, certified, certified + (int)open.size(), last_feasible);
        for (int R : open)
            std::printf(" %d(+%lld)", R, (long long)(cur[R] - G(m, R)));
        std::printf("\n");
        if (detail)
            for (int R = 1; R <= RM; ++R)
                if (cur[R] > NEG)
                    std::printf("  k=%d R=%d F=%lld G=%lld\n", k, R,
                                (long long)cur[R], (long long)G(m, R));
        prev.swap(cur);
    }
}
