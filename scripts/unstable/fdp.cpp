// Exploratory fiber-recursion upper bound.
//
// This is deliberately under scripts/unstable: the relaxation forgets symbol
// capacity and therefore is not a proof of the target boundary inequality.
// Usage: fdp_unstable m k max_R [detail]

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>

using ll = long long;
constexpr ll NEG = LLONG_MIN / 4;

ll E(int r) {
    ll result = 0;
    for (int i = 0; i < r; ++i)
        result += __builtin_popcount(static_cast<unsigned>(i));
    return result;
}

ll C(int r) {
    if (r == 0)
        return 0;
    ll result = r - 1 - E(r);
    for (int i = 1; i < r; ++i)
        result += 32 - __builtin_clz(static_cast<unsigned>(i));
    return result;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s m k max_R [detail]\n", argv[0]);
        return 1;
    }
    const int m = std::atoi(argv[1]);
    const int maximum_k = std::atoi(argv[2]);
    const int maximum_r = std::atoi(argv[3]);
    const bool detail = argc > 4;
    if (m < 1 || maximum_k < 0 || maximum_r < 0)
        return 1;

    std::vector<ll> previous(maximum_r + 1, NEG);
    previous[0] = 0;
    if (maximum_r >= 1)
        previous[1] = 0;

    for (int k = 1; k <= maximum_k; ++k) {
        const int n = m + k;
        std::vector<ll> current(maximum_r + 1, NEG);
        current[0] = 0;
        for (int used_roots = 1; used_roots <= maximum_r; ++used_roots) {
            std::vector<ll> child(used_roots + 1, NEG);
            for (int size = 1; size <= used_roots; ++size)
                if (previous[size] > NEG)
                    child[size] = std::min(previous[size] + used_roots - size,
                                           static_cast<ll>(m) * (k - 1) * size);

            const int pools = std::min(n, maximum_r);
            std::vector<std::vector<ll>> dp(
                pools + 1, std::vector<ll>(maximum_r + 1, NEG));
            dp[0][0] = 0;
            for (int pool = 1; pool <= pools; ++pool) {
                for (int total = 1; total <= maximum_r; ++total) {
                    ll best = NEG;
                    for (int size = 1; size <= std::min(used_roots, total);
                         ++size)
                        if (child[size] > NEG &&
                            dp[pool - 1][total - size] > NEG)
                            best = std::max(best, dp[pool - 1][total - size] +
                                                      child[size]);
                    dp[pool][total] = best;
                }
            }
            for (int r = used_roots;
                 r <= std::min(maximum_r, (m + 1) * used_roots); ++r) {
                const ll best = std::accumulate(
                    dp.begin() + 1, dp.end(), NEG,
                    [r](ll current, const std::vector<ll> &row) {
                        return std::max(current, row[r]);
                    });
                if (best > NEG)
                    current[r] = std::max(
                        current[r],
                        static_cast<ll>(m + 1) * (r - used_roots) + best);
            }
        }

        int first_bad = -1;
        int last_feasible = 0;
        for (int r = 1; r <= maximum_r; ++r) {
            if (current[r] > NEG)
                last_feasible = r;
            if (first_bad < 0 && current[r] > NEG &&
                current[r] > C(r) + static_cast<ll>(m) * E(r))
                first_bad = r;
        }
        std::printf(
            "m=%d k=%d: relaxed F<=G through R<%d (feasible through R=%d)\n", m,
            k, first_bad < 0 ? maximum_r + 1 : first_bad, last_feasible);
        if (detail) {
            for (int r = 1; r <= maximum_r; ++r)
                if (current[r] > NEG)
                    std::printf("  R=%d F=%lld G=%lld\n", r, current[r],
                                C(r) + static_cast<ll>(m) * E(r));
        }
        previous = std::move(current);
    }
}
