// Exhaustive oracle: max Q_m(S) = m*k*|S| - |ext(S)| over origin-pinned
// R-subsets of A(n,k).  Vertex-transitivity makes the pin exact.
// Usage: max_q_oracle n k R    (prints one line: n k m R maxQ G slack witness)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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
std::vector<int> mark, chosen, best;
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
    ++stamp;
    int inside = stamp;
    for (int v : chosen)
        mark[v] = inside;
    ++stamp;
    int bnd = 0;
    for (int v : chosen)
        for (int w : adj[v])
            if (mark[w] != inside && mark[w] != stamp) {
                mark[w] = stamp;
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
    if ((std::uint64_t)verts.size() * verts.size() > 500'000'000'000ULL) {
        std::fprintf(stderr, "adjacency too large: |A(n,k)|=%zu\n",
                     verts.size());
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
