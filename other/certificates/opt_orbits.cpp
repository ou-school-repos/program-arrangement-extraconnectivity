// Count orbits of R-subsets attaining the target formula G (Q == G), using the
// patched canonicalizer.  Usage: opt_orbits n k R
#include "../include/arrangement_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <set>
using namespace arrangement;
long long E(int r) {
    long long s = 0;
    for (int i = 0; i < r; i++)
        s += __builtin_popcount(i);
    return s;
}
long long C(int r) {
    if (!r)
        return 0;
    long long s = r - 1 - E(r);
    for (int i = 1; i < r; i++)
        s += 32 - __builtin_clz(i);
    return s;
}
int main(int, char **v) {
    int n = atoi(v[1]), k = atoi(v[2]), R = atoi(v[3]), m = n - k;
    Instance I(n, k);
    auto st = origin_stabilizer(I);
    long long G = C(R) + m * E(R);
    std::set<std::vector<int>> opt;
    std::vector<int> ch{0};
    std::vector<int> mark(I.vertices.size(), 0);
    int stamp = 0;
    long long best = -1;
    std::vector<std::vector<int>> adj(I.vertices.size());
    for (int p = 0; p < k; p++)
        for (const auto &l : I.lines[p])
            for (int a : l)
                for (int b : l)
                    if (a != b)
                        adj[a].push_back(b);
    auto eval = [&]() {
        int in = ++stamp;
        for (int x : ch)
            mark[x] = in;
        int t = ++stamp, b = 0;
        for (int x : ch)
            for (int w : adj[x])
                if (mark[w] != in && mark[w] != t) {
                    mark[w] = t;
                    b++;
                }
        long long q = (long long)m * k * R - b;
        best = std::max(best, q);
        if (q == G) {
            auto s = ch;
            std::sort(s.begin(), s.end());
            opt.insert(canonical_key(s, I, st));
        }
    };
    std::function<void(int)> rec = [&](int nx) {
        if ((int)ch.size() == R) {
            eval();
            return;
        }
        int need = R - ch.size();
        for (int x = nx; x + need <= (int)I.vertices.size(); x++) {
            ch.push_back(x);
            rec(x + 1);
            ch.pop_back();
        }
    };
    rec(1);
    int bl = 0;
    while ((1 << bl) < R)
        bl++;
    printf("A(%d,%d) R=%d maxQ=%lld G=%lld embeddable=%s optimal-orbits=%zu\n",
           n, k, R, best, G, (bl <= m && bl <= k) ? "yes" : "no", opt.size());
}
