// Finite regression for the arithmetic lemmas used by the fracture writeup.
// This is a diagnostic, not a proof. Usage: fracture_arith_check [DMAX<=12]
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

using i64 = std::int64_t;
constexpr i64 NEG = INT64_MIN / 4;

int main(int argc, char **argv) {
    const int dmax = argc > 1 ? std::atoi(argv[1]) : 10;
    if (dmax < 2 || dmax > 12) {
        std::fprintf(stderr, "DMAX in [2,12]\n");
        return 2;
    }
    const int N = 1 << dmax;
    std::vector<i64> E(N + 1, 0);
    for (int i = 0; i < N; ++i)
        E[i + 1] = E[i] + __builtin_popcount(static_cast<unsigned>(i));
    int failures = 0;
    for (int d = 1; d <= dmax; ++d)
        for (int a = 1; a < (1 << (d - 1)); ++a)
            if (a * (d - 1) - 2 * E[a] < d - 1 ||
                E[1 << d] - E[a] - E[(1 << d) - a] - a !=
                    a * (d - 1) - 2 * E[a]) {
                std::printf("A fails d=%d a=%d\n", d, a);
                ++failures;
            }
    for (int d = 2; d <= dmax; ++d) {
        const int R = 1 << d;
        i64 worst = NEG;
        for (int M = 1; M < R; ++M) {
            const int rest = R - M;
            std::vector<i64> best(rest + 1, NEG);
            best[0] = 0;
            const int cap = M == R / 2 ? rest - 1 : M;
            for (int part = 1; part <= cap; ++part)
                for (int s = part; s <= rest; ++s)
                    if (best[s - part] > NEG)
                        best[s] = std::max(best[s], best[s - part] + E[part]);
            if (best[rest] > NEG)
                worst = std::max(worst, E[M] + best[rest] + R - M);
        }
        if (worst > E[R] - (d - 1)) {
            std::printf("P fails d=%d worst=%lld\n", d,
                        static_cast<long long>(worst));
            ++failures;
        }
    }
    for (int d = 3; d <= dmax; ++d) {
        const int h = 1 << (d - 1);
        for (int t = 1; t <= h - 2; ++t)
            if (!(E[h - 1 - t] + E[t] < E[h - 1] - (d - 3))) {
                std::printf("S fails d=%d t=%d\n", d, t);
                ++failures;
            }
    }
    std::printf("fracture arithmetic regressions through d=%d: %s\n", dmax,
                failures ? "FAIL" : "OK");
    return failures ? 1 : 0;
}
