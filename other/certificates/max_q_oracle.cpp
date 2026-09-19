// Exhaustive oracle: max Q_m(S) = m*k*|S| - |ext(S)| over origin-pinned
// R-subsets of A(n,k).  Vertex-transitivity makes the pin exact.
// Usage: max_q_oracle n k R    (prints one line: n k m R maxQ G slack witness)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using i64 = std::int64_t;
static i64 E(int r) {
    i64 s = 0;
    for (int i = 0; i < r; ++i)
        s += __builtin_popcount(unsigned(i));
    return s;
}
static i64 C(int r) {
    if (!r)
        return 0;
    i64 s = r - 1 - E(r);
    for (int i = 1; i < r; ++i)
        s += 32 - __builtin_clz(unsigned(i));
    return s;
}
int n, k, R;
std::vector<std::vector<int>> verts, adj;
std::vector<std::uint64_t> mark;
std::vector<int> chosen, best;
std::uint64_t stamp = 0;
i64 bestQ = -1;
std::uint64_t leaves = 0;
static void gen(std::vector<int> &p, std::vector<char> &used) {
    if ((int)p.size() == k) {
        verts.push_back(p);
        return;
    }
    for (int s = 0; s < n; ++s)
        if (!used[s]) {
            used[s] = 1;
            p.push_back(s);
            gen(p, used);
            p.pop_back();
            used[s] = 0;
        }
}
static void evaluate() {
    ++leaves;
    if (stamp > std::numeric_limits<std::uint64_t>::max() - 2) {
        std::fill(mark.begin(), mark.end(), 0);
        stamp = 0;
    }
    const std::uint64_t inside = ++stamp;
    for (int v : chosen)
        mark[v] = inside;
    const std::uint64_t border = ++stamp;
    int bnd = 0;
    for (int v : chosen)
        for (int w : adj[v])
            if (mark[w] != inside && mark[w] != border) {
                mark[w] = border;
                ++bnd;
            }
    i64 q = i64(n - k) * k * R - bnd;
    if (q > bestQ) {
        bestQ = q;
        best = chosen;
    }
}
static void rec(int next) {
    if ((int)chosen.size() == R) {
        evaluate();
        return;
    }
    int need = R - (int)chosen.size();
    for (int v = next; v + need <= (int)verts.size(); ++v) {
        chosen.push_back(v);
        rec(v + 1);
        chosen.pop_back();
    }
}
int main(int argc, char **argv) {
    if (argc != 4) {
        std::fprintf(stderr, "usage: %s n k R\n", argv[0]);
        return 2;
    }
    n = std::atoi(argv[1]);
    k = std::atoi(argv[2]);
    R = std::atoi(argv[3]);
    if (k < 1 || k >= n || R < 1 || n > 8) {
        std::fprintf(stderr, "require 1<=k<n<=8, R>=1\n");
        return 2;
    }
    std::vector<int> p;
    std::vector<char> used(n, 0);
    gen(p, used);
    if (R > (int)verts.size()) {
        std::fprintf(stderr, "R exceeds |A(n,k)|\n");
        return 2;
    }
    constexpr std::uint64_t max_search_leaves = 50'000'000;
    const std::uint64_t available =
        static_cast<std::uint64_t>(verts.size() - 1);
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
    const std::uint64_t vertex_count = static_cast<std::uint64_t>(verts.size());
    const std::uint64_t degree = static_cast<std::uint64_t>(k) * (n - k);
    constexpr std::uint64_t max_adjacency_entries = 100'000'000;
    if (degree != 0 && vertex_count > max_adjacency_entries / degree) {
        std::fprintf(stderr, "adjacency too large: |A(n,k)|=%zu, degree=%llu\n",
                     verts.size(), static_cast<unsigned long long>(degree));
        return 2;
    }
    // adjacency: differ in exactly one coordinate
    adj.assign(verts.size(), {});
    for (size_t a = 0; a < verts.size(); ++a)
        for (size_t b = 0; b < verts.size(); ++b) {
            int diff = 0;
            for (int i = 0; i < k; ++i)
                diff += verts[a][i] != verts[b][i];
            if (diff == 1)
                adj[a].push_back((int)b);
        }
    if (!std::all_of(adj.begin(), adj.end(), [=](const auto &l) {
            return static_cast<int>(l.size()) == k * (n - k);
        })) {
        std::fprintf(stderr, "degree check failed\n");
        return 3;
    }
    mark.assign(verts.size(), 0);
    chosen.push_back(0);
    rec(1);
    const int m = n - k;
    const i64 G = C(R) + m * E(R);
    std::printf(
        "n=%d k=%d m=%d R=%d leaves=%llu maxQ=%lld G=%lld slack=%lld witness=",
        n, k, m, R, (unsigned long long)leaves, (long long)bestQ, (long long)G,
        (long long)(G - bestQ));
    for (int v : best) {
        std::printf("(");
        for (int i = 0; i < k; ++i)
            std::printf("%d", verts[v][i]);
        std::printf(")");
    }
    std::printf("\n");
}
