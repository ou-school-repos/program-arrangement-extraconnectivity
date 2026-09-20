// pattern_catalogue.cpp — catalogue of connected R-vertex patterns in
// arrangement graphs.
//
// Host graph A(N,K) with K = R-1, N = 2R-2 contains every connected R-vertex
// pattern: a spanning tree changes at most R-1 coordinates (p <= K), and each
// added vertex adds at most one symbol, so active + constant symbols <= K + R -
// 1 = N. ESU enumerates every connected R-set through vertex 0 (complete by
// vertex-transitivity). For each set it records the signature (D, X, p, s_a):
//   U = sum_q #distinct q-roots, D = RK - U, X = U(N-K+1) - RK - |N(S)|
//   (boundary identity), p = #coordinates that vary, s_a = #symbols occurring
//   in varying coordinates.
// Prediction for any A(n,k):  Phi_conn(R) = min over signatures with p<=k,
// s_a-p<=n-k of
//   (Rk - D)(n-k) - D - X.
// Usage: ./pattern_catalogue R [n k]...   (prints signatures, then predictions)
#include <bits/stdc++.h>
using namespace std;
int R, K, N, NV;
vector<array<int8_t, 8>> W;
vector<vector<int>> adj;
vector<char> inS;
vector<int> nsCnt, stamp;
int stampId = 0;
int S[16];
set<array<int, 4>> sigs;
map<array<int, 4>, vector<int>> ex;
long long leaves = 0;
void leaf() {
    leaves++;
    ++stampId;
    int ext = 0;
    for (int t = 0; t < R; t++)
        for (int w : adj[S[t]])
            if (!inS[w] && stamp[w] != stampId) {
                stamp[w] = stampId;
                ext++;
            }
    int U = 0, p = 0;
    set<int> sym;
    for (int q = 0; q < K; q++) {
        set<vector<int>> roots;
        set<int> col;
        for (int t = 0; t < R; t++) {
            vector<int> r;
            for (int j = 0; j < K; j++)
                if (j != q)
                    r.push_back(W[S[t]][j]);
            roots.insert(r);
            col.insert(W[S[t]][q]);
        }
        U += static_cast<int>(roots.size());
        if (col.size() > 1) {
            p++;
            for (int c : col)
                sym.insert(c);
        }
    }
    array<int, 4> sg = {R * K - U, U * (N - K + 1) - R * K - ext, p,
                        (int)sym.size()};
    if (sigs.insert(sg).second)
        ex[sg] = vector<int>(S, S + R);
}
void rec(int sz, vector<int> ext) {
    if (sz == R) {
        leaf();
        return;
    }
    while (!ext.empty()) {
        int w = ext.back();
        ext.pop_back();
        vector<int> nb;
        copy_if(adj[w].begin(), adj[w].end(), back_inserter(nb),
                [&](int x) { return x > 0 && !inS[x] && nsCnt[x] == 0; });
        S[sz] = w;
        inS[w] = 1;
        nsCnt[w]++;
        for (int x : adj[w])
            nsCnt[x]++;
        vector<int> e2 = ext;
        for (int x : nb)
            if (find(e2.begin(), e2.end(), x) == e2.end())
                e2.push_back(x);
        rec(sz + 1, e2);
        inS[w] = 0;
        nsCnt[w]--;
        for (int x : adj[w])
            nsCnt[x]--;
    }
}
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s R [n k]...\n", argv[0]);
        return 1;
    }
    R = atoi(argv[1]);
    // K=R-1 coordinates must fit array<int8_t,8>, S[] holds R entries, symbols
    // fit int8_t.
    if (R < 1 || R > 9) {
        fprintf(
            stderr,
            "R must be in 1..9 (array<int8_t,8> holds K=R-1<=8 coordinates)\n");
        return 1;
    }
    K = max(1, R - 1);
    N = K + R - 1;
    int argi = 2;
    if (argc >= 5 && string(argv[2]) == "--host") {
        N = atoi(argv[3]);
        K = atoi(argv[4]);
        argi = 5;
        if (K < 1 || K > 8 || N <= K) {
            fprintf(stderr, "bad host\n");
            return 1;
        }
    }
    vector<int> c;
    function<void()> gen = [&]() {
        if ((int)c.size() == K) {
            array<int8_t, 8> a{};
            for (int i = 0; i < K; i++)
                a[i] = static_cast<int8_t>(c[i]);
            W.push_back(a);
            return;
        }
        for (int x = 0; x < N; x++)
            if (find(c.begin(), c.end(), x) == c.end()) {
                c.push_back(x);
                gen();
                c.pop_back();
            }
    };
    gen();
    NV = static_cast<int>(W.size());
    map<array<int8_t, 8>, int> id;
    for (int i = 0; i < NV; i++)
        id[W[i]] = i;
    adj.assign(NV, {});
    for (int i = 0; i < NV; i++)
        for (int q = 0; q < K; q++)
            for (int x = 0; x < N; x++) {
                bool used = false;
                for (int j = 0; j < K; j++)
                    if (W[i][j] == x)
                        used = true;
                if (used)
                    continue;
                auto v = W[i];
                v[q] = static_cast<int8_t>(x);
                adj[i].push_back(id[v]);
            }
    inS.assign(NV, 0);
    nsCnt.assign(NV, 0);
    stamp.assign(NV, 0);
    S[0] = 0;
    inS[0] = 1;
    nsCnt[0]++;
    for (int x : adj[0])
        nsCnt[x]++;
    vector<int> e0;
    copy(adj[0].begin(), adj[0].end(), back_inserter(e0));
    if (R == 1)
        leaf();
    else
        rec(1, e0);
    printf("R=%d host=A(%d,%d) [complete for cells k<=K, n-k<=N-K] connected "
           "sets through root=%lld signatures=%zu\n",
           R, N, K, leaves, sigs.size());
    printf("D X p s_a  example\n");
    for (const auto &s : sigs) {
        printf("%d %d %d %d  ", s[0], s[1], s[2], s[3]);
        for (int v : ex[s]) {
            printf("(");
            for (int j = 0; j < K; j++)
                printf("%d%s", W[v][j], j + 1 < K ? "," : "");
            printf(")");
        }
        puts("");
    }
    for (int a = argi; a + 1 < argc; a += 2) {
        int n = atoi(argv[a]), k = atoi(argv[a + 1]);
        if (k < 1 || n <= k) {
            fprintf(stderr, "skip A(%d,%d): need n>k>=1\n", n, k);
            continue;
        }
        long best = LONG_MAX;
        array<int, 4> arg{};
        for (const auto &s : sigs)
            if (s[2] <= k && s[3] - s[2] <= n - k) {
                long v = (long)(R * k - s[0]) * (n - k) - s[0] - s[1];
                if (v < best) {
                    best = v;
                    arg = s;
                }
            }
        printf("predict A(%d,%d): Phi_conn(%d)=%ld via "
               "(D,X,p,s_a)=(%d,%d,%d,%d)\n",
               n, k, R, best, arg[0], arg[1], arg[2], arg[3]);
    }
}
