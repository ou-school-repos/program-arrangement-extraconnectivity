// witness_finder.cpp — search for R-sets in A(n,k) whose external boundary
// beats the embedded Hamming ball. Simulated annealing over arbitrary R-subsets
// (move: drop a member, add a vertex adjacent to the set), boundary counted
// directly (no incremental accumulator). Usage: ./witness_finder R k m seconds
// seed
#include <bits/stdc++.h>
using namespace std;
using Vtx = array<int8_t, 24>;
int R, K, M, N;
struct H {
    size_t operator()(const Vtx &v) const {
        size_t h = 0;
        for (int i = 0; i < K; i++)
            h = h * 31 + (uint8_t)v[i];
        return h;
    }
};
bool inj(const Vtx &v) {
    uint64_t s = 0;
    for (int i = 0; i < K; i++) {
        if (s >> v[i] & 1)
            return false;
        s |= 1ULL << v[i];
    }
    return true;
}
int boundary(const vector<Vtx> &S) {
    unordered_set<Vtx, H> in(S.begin(), S.end()), nb;
    for (auto &u : S) {
        uint64_t used = 0;
        for (int i = 0; i < K; i++)
            used |= 1ULL << u[i];
        for (int i = 0; i < K; i++)
            for (int y = 0; y < N; y++)
                if (!(used >> y & 1)) {
                    Vtx w = u;
                    w[i] = y;
                    if (!in.count(w))
                        nb.insert(w);
                }
    }
    return nb.size();
}
long E(int r) {
    long t = 0;
    for (int i = 0; i < r; i++)
        t += __builtin_popcount(i);
    return t;
}
long Cc(int r) {
    long s = 0;
    for (int i = 1; i < r; i++)
        s += 32 - __builtin_clz(i);
    return (r - 1) + s - E(r);
}
int main(int argc, char **argv) {
    R = atoi(argv[1]);
    K = atoi(argv[2]);
    M = atoi(argv[3]);
    double secs = atof(argv[4]);
    unsigned seed = atoi(argv[5]);
    N = K + M;
    if (K > 24 || N > 64) {
        puts("limits");
        return 1;
    }
    mt19937 rng(seed);
    Vtx c{};
    for (int i = 0; i < K; i++)
        c[i] = i;
    long Hval = (long)(R * K - E(R)) * M - Cc(R);
    int d = 32 - __builtin_clz(max(1, R - 1));
    if (R == 1)
        d = 0;
    // starting sets: Hamming ball (binary order on d coords), balanced
    // rook-star, random connected
    vector<vector<Vtx>> starts;
    {
        vector<Vtx> S;
        for (int x = 0; x < R; x++) {
            Vtx v = c;
            for (int j = 0; j < d; j++)
                if (x >> j & 1)
                    v[j] = K + j;
            S.push_back(v);
        }
        starts.push_back(S);
    }
    {
        vector<Vtx> S{c};
        for (int a = 0; a < R - 1; a++) {
            Vtx v = c;
            v[a % K] = K + (a % M);
            if (find(S.begin(), S.end(), v) == S.end())
                S.push_back(v);
        }
        while ((int)S.size() < R) {
            Vtx v = S[rng() % S.size()];
            v[rng() % K] = K + rng() % M;
            if (inj(v) && find(S.begin(), S.end(), v) == S.end())
                S.push_back(v);
        }
        starts.push_back(S);
    }
    int best = INT_MAX;
    vector<Vtx> bestS;
    auto t0 = chrono::steady_clock::now();
    long iters = 0;
    for (int run = 0;; run++) {
        vector<Vtx> S = starts[run % starts.size()];
        bool noseed = argc > 6;
        if (noseed && run < 2)
            run = 2;
        if (run >= 2) { // random connected start
            S = {c};
            while ((int)S.size() < R) {
                Vtx v = S[rng() % S.size()];
                v[rng() % K] = rng() % N;
                if (inj(v) && find(S.begin(), S.end(), v) == S.end())
                    S.push_back(v);
            }
        }
        int cur = boundary(S);
        double T = 3.0;
        if (cur < best) {
            best = cur;
            bestS = S;
        }
        for (int step = 0; step < 4000; step++, iters++) {
            int ri = rng() % R;
            Vtx v = S[rng() % R];
            v[rng() % K] = rng() % N;
            if (!inj(v) || find(S.begin(), S.end(), v) != S.end())
                continue;
            Vtx old = S[ri];
            S[ri] = v;
            int b = boundary(S);
            if (b <= cur ||
                uniform_real_distribution<>(0, 1)(rng) < exp((cur - b) / T)) {
                cur = b;
                if (b < best) {
                    best = b;
                    bestS = S;
                }
            } else
                S[ri] = old;
            T = max(0.05, T * 0.999);
        }
        if (chrono::duration<double>(chrono::steady_clock::now() - t0).count() >
            secs)
            break;
    }
    printf("R=%d A(%d,%d) m=%d d=%d gate=%s  Hamming=%ld  best found=%d  %s  "
           "iters=%ld\n",
           R, N, K, M, d, (d <= K && d <= M) ? "open" : "closed", Hval, best,
           best < Hval ? "<-- BEATS HAMMING" : "", iters);
    if (best < Hval) {
        for (auto &v : bestS) {
            printf(" (");
            for (int i = 0; i < K; i++)
                printf("%d%s", v[i], i + 1 < K ? "," : "");
            printf(")");
        }
        puts("");
    }
}
