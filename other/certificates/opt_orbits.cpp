// Count orbits of R-subsets attaining the target formula G (Q == G), using the
// patched canonicalizer.  Usage: opt_orbits n k R
#include "arrangement_core.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <set>
using namespace arrangement;

bool stabilizer_exceeds(int n, int k, std::uint64_t limit) {
    std::uint64_t size = 1;
    for (const int degree : {k, n - k})
        for (int factor = 2; factor <= degree; ++factor) {
            const auto value = static_cast<std::uint64_t>(factor);
            if (size > limit / value)
                return true;
            size *= value;
        }
    return size > limit;
}

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
int main(int argc, char **v) {
    if (argc != 4) {
        std::fprintf(stderr, "usage: %s n k R\n", v[0]);
        return 2;
    }
    int n = atoi(v[1]), k = atoi(v[2]), R = atoi(v[3]), m = n - k;
    if (n < 1 || k < 1 || k > n || R < 1 || R > 4096) {
        std::fprintf(stderr,
                     "require n >= 1, 1 <= k <= n, and 1 <= R <= 4096\n");
        return 2;
    }
    constexpr std::uint64_t max_stabilizer_size = 100'000;
    if (stabilizer_exceeds(n, k, max_stabilizer_size)) {
        std::fprintf(stderr, "origin stabilizer exceeds %llu automorphisms\n",
                     static_cast<unsigned long long>(max_stabilizer_size));
        return 2;
    }
    constexpr std::uint64_t max_vertices = 500'000;
    std::uint64_t vertex_count = 1;
    for (int i = 0; i < k; ++i) {
        const auto factor = static_cast<std::uint64_t>(n - i);
        if (vertex_count > max_vertices / factor) {
            std::fprintf(stderr, "graph exceeds %llu vertices\n",
                         static_cast<unsigned long long>(max_vertices));
            return 2;
        }
        vertex_count *= factor;
    }
    if (static_cast<std::uint64_t>(R) > vertex_count) {
        std::fprintf(stderr, "R exceeds |A(n,k)|\n");
        return 2;
    }
    constexpr std::uint64_t max_search_leaves = 50'000'000;
    const std::uint64_t available = vertex_count - 1;
    std::uint64_t choose = static_cast<std::uint64_t>(R - 1);
    choose = std::min(choose, available - choose);
    unsigned __int128 search_leaves = 1;
    for (std::uint64_t i = 1; i <= choose; ++i) {
        search_leaves = search_leaves * (available - choose + i) / i;
        if (search_leaves > max_search_leaves) {
            std::fprintf(stderr, "search exceeds %llu origin-pinned subsets\n",
                         static_cast<unsigned long long>(max_search_leaves));
            return 2;
        }
    }
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
