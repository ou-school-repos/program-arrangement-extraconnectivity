// exact_profile_d2.cpp — exact unrestricted boundary profile Phi(R) of A(n,k)
// for |V| = nPk <= 512.
//
// Method: ESU enumeration of all sets that are connected in the distance-<=2
// graph and contain vertex 0 (complete up to vertex-transitivity). Sets that
// split into parts at distance >= 3 have additive boundaries (splitting lemma),
// so Phi(R) = min(this program's output, min_{a+b=R} Phi(a)+Phi(b)); take that
// minimum over the per-R outputs.
//
// Prints the minimum boundary and (D, X, s) of one minimizer.
// Build:  g++ -O2 -o exact_profile_d2 exact_profile_d2.cpp
// Usage:  ./exact_profile_d2 n k R
// Exact unrestricted Phi(R) on A(n,k) (n^k small, |V|<=512) via ESU over
// distance-<=2-connected sets plus additive splits for parts at distance >=3.
// Also records (D,X,s) of minimizers.
#include <bits/stdc++.h>
using namespace std;
using B = bitset<512>;
int n, k, R, NV;
vector<vector<int>> W;
vector<B> A1, A2;
long long cnt = 0;
int best = 1e9;
vector<int> bestSet;
static inline int pc(const B &x) { return static_cast<int>(x.count()); }
static inline int first_set_bit(const B &x) {
    for (int i = 0; i < 512; ++i)
        if (x.test(static_cast<size_t>(i)))
            return i;
    return -1;
}
int S[512];
void rec(int sz, B Sset, B ext, const B &NS2, B N1) {
    if (sz == R) {
        cnt++;
        int b = pc(N1 & ~Sset);
        if (b < best) {
            best = b;
            bestSet.assign(S, S + R);
        }
        return;
    }
    while (ext.any()) {
        const int w = first_set_bit(ext);
        ext.reset(static_cast<size_t>(w));
        B nb = A2[w] & ~Sset & ~NS2;
        nb.reset(0); // exclusive, >root(0)
        S[sz] = w;
        Sset.set(static_cast<size_t>(w));
        rec(sz + 1, Sset, ext | nb, NS2 | A2[w], N1 | A1[w]);
        Sset.reset(static_cast<size_t>(w));
    }
}
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: exact_profile_d2 n k R\n");
        return 1;
    }
    n = atoi(argv[1]);
    k = atoi(argv[2]);
    R = atoi(argv[3]);
    if (n < 2 || k < 1 || k >= n || R < 1) {
        fprintf(stderr, "Require n > k >= 1 and R >= 1\n");
        return 1;
    }
    // all injective k-words
    function<void(vector<int> &)> gen = [&](vector<int> &c) {
        if ((int)c.size() == k) {
            W.push_back(c);
            return;
        }
        for (int x = 0; x < n; x++)
            if (find(c.begin(), c.end(), x) == c.end()) {
                c.push_back(x);
                gen(c);
                c.pop_back();
            }
    };
    vector<int> c;
    gen(c);
    if (W.size() > 512) {
        fprintf(stderr, "too big: graph has %zu vertices (limit 512)\n",
                W.size());
        return 1;
    }
    if (static_cast<size_t>(R) > W.size()) {
        fprintf(stderr, "R exceeds the graph's vertex count\n");
        return 1;
    }
    NV = static_cast<int>(W.size());
    map<vector<int>, int> id;
    for (int i = 0; i < NV; i++)
        id[W[i]] = i;
    A1.assign(NV, 0);
    A2.assign(NV, 0);
    for (int i = 0; i < NV; i++)
        for (int q = 0; q < k; q++)
            for (int x = 0; x < n; x++) {
                auto v = W[i];
                if (find(v.begin(), v.end(), x) != v.end())
                    continue;
                v[q] = x;
                A1[i].set(static_cast<size_t>(id.at(v)));
            }
    for (int i = 0; i < NV; i++) {
        B t = A1[i];
        for (int j = 0; j < NV; j++)
            if (A1[i].test(static_cast<size_t>(j)))
                t |= A1[j];
        t.reset(static_cast<size_t>(i));
        A2[i] = t;
    }
    S[0] = 0;
    B initial_members;
    initial_members.set(0);
    B initial_ext = A2[0];
    initial_ext.reset(0);
    B initial_distance2 = A2[0];
    initial_distance2.set(0);
    rec(1, initial_members, initial_ext, initial_distance2, A1[0]);
    // D,X,s of minimizer
    int m = n - k, U = 0;
    for (int q = 0; q < k; q++) {
        set<vector<int>> r;
        for (int t = 0; t < R; t++) {
            auto v = W[bestSet[t]];
            v.erase(v.begin() + q);
            r.insert(v);
        }
        U += static_cast<int>(r.size());
    }
    set<int> sym;
    for (int t = 0; t < R; t++)
        for (int x : W[bestSet[t]])
            sym.insert(x);
    printf("A(%d,%d) R=%d: d2-connected sets=%lld  min boundary=%d  minimizer "
           "D=%d X=%d s=%d :",
           n, k, R, cnt, best, R * k - U, U * (m + 1) - R * k - best,
           (int)sym.size());
    for (int t = 0; t < R; t++) {
        printf(" (");
        for (int q = 0; q < k; q++)
            printf("%d%s", W[bestSet[t]][q], q + 1 < k ? "," : "");
        printf(")");
    }
    puts("");
}
