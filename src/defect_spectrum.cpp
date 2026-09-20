// Exhaustive defect spectrum of connected origin-pinned R-subsets of A(n,k).
// Usage: defect_spectrum n k R
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>
int n, k, R;
std::vector<std::vector<int>> V, adj;
std::vector<std::vector<int>> rootid;
std::vector<std::vector<int>> cnt;
std::vector<int> ch;
int U = 0;
std::uint64_t leaves = 0;
bool work_limit_exceeded = false;
constexpr std::uint64_t max_search_leaves = 20'000'000;
std::set<int> seen;
std::map<int, std::vector<int>> wit;
bool connected() {
    std::vector<int> st{ch[0]};
    std::vector<char> in(V.size(), 0), got(V.size(), 0);
    for (int x : ch)
        in[x] = 1;
    got[ch[0]] = 1;
    int c = 1;
    while (!st.empty()) {
        int x = st.back();
        st.pop_back();
        for (int y : adj[x])
            if (in[y] && !got[y]) {
                got[y] = 1;
                ++c;
                st.push_back(y);
            }
    }
    return c == R;
}
void add(int v, int s) {
    for (int p = 0; p < k; ++p) {
        int &c = cnt[p][rootid[p][v]];
        if (s > 0) {
            if (c++ == 0)
                ++U;
        } else {
            if (--c == 0)
                --U;
        }
    }
}
void rec(int nx) {
    if (work_limit_exceeded)
        return;
    if ((int)ch.size() == R) {
        if (++leaves > max_search_leaves) {
            work_limit_exceeded = true;
            return;
        }
        int D = R * k - U;
        if (!seen.count(D) && connected()) {
            seen.insert(D);
            wit[D] = ch;
        }
        return;
    }
    for (int v = nx; v + (R - (int)ch.size()) <= (int)V.size(); ++v) {
        ch.push_back(v);
        add(v, 1);
        rec(v + 1);
        add(v, -1);
        ch.pop_back();
        if (work_limit_exceeded)
            return;
    }
}
int main(int argc, char **a) {
    if (argc != 4) {
        std::fprintf(stderr, "Usage: defect_spectrum n k R\n");
        return 2;
    }
    n = atoi(a[1]);
    k = atoi(a[2]);
    R = atoi(a[3]);
    if (n < 1 || k < 1 || k > n || R < 1 || R > 1024) {
        std::fprintf(stderr,
                     "Error: require n >= 1, 1 <= k <= n, 1 <= R <= 1024\n");
        return 2;
    }
    constexpr std::size_t max_vertices = 10'000;
    std::size_t vertex_count = 1;
    for (int i = 0; i < k; ++i) {
        const auto factor = static_cast<std::size_t>(n - i);
        if (vertex_count > max_vertices / factor) {
            std::fprintf(stderr, "Error: graph exceeds %zu vertices\n",
                         max_vertices);
            return 2;
        }
        vertex_count *= factor;
    }
    if (static_cast<std::size_t>(R) > vertex_count) {
        std::fprintf(stderr, "Error: R exceeds vertex count %zu\n",
                     vertex_count);
        return 2;
    }
    std::vector<int> p;
    std::vector<char> u(n, 0);
    auto gen = [&](auto &&self) -> void {
        if ((int)p.size() == k) {
            V.push_back(p);
            return;
        }
        for (int s = 0; s < n; ++s)
            if (!u[s]) {
                u[s] = 1;
                p.push_back(s);
                self(self);
                p.pop_back();
                u[s] = 0;
            }
    };
    gen(gen);
    constexpr std::size_t max_adjacency_entries = 10'000'000;
    const std::size_t degree = static_cast<std::size_t>(k) * (n - k);
    if (degree != 0 && V.size() > max_adjacency_entries / degree) {
        std::fprintf(stderr, "Error: adjacency exceeds resource limit\n");
        return 2;
    }
    adj.assign(V.size(), {});
    rootid.assign(k, std::vector<int>(V.size()));
    cnt.assign(k, {});
    for (int q = 0; q < k; ++q) {
        std::map<std::vector<int>, int> ids;
        for (size_t i = 0; i < V.size(); ++i) {
            auto r = V[i];
            r.erase(r.begin() + q);
            auto it = ids.emplace(r, ids.size()).first;
            rootid[q][i] = it->second;
        }
        cnt[q].assign(ids.size(), 0);
    }
    for (size_t x = 0; x < V.size(); ++x)
        for (size_t y = 0; y < V.size(); ++y) {
            int d = 0;
            for (int i = 0; i < k; ++i)
                d += V[x][i] != V[y][i];
            if (d == 1)
                adj[x].push_back(static_cast<int>(y));
        }
    ch.push_back(0);
    add(0, 1);
    rec(1);
    if (work_limit_exceeded) {
        std::fprintf(stderr, "Error: search exceeds %llu subsets\n",
                     static_cast<unsigned long long>(max_search_leaves));
        return 2;
    }
    std::printf("A(%d,%d) R=%d connected defects:", n, k, R);
    for (int d : seen)
        std::printf(" %d", d);
    std::printf("\n");
    for (auto &[d, w] : wit)
        if (d >= 10) {
            std::printf("  D=%d:", d);
            for (int x : w) {
                std::printf(" ");
                for (int i = 0; i < k; ++i)
                    std::printf("%d", V[x][i]);
            }
            std::printf("\n");
        }
}
