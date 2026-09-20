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
        size_t h = accumulate(v.begin(), v.begin() + K, size_t{0},
                              [](size_t acc, int8_t vi) {
                                  return acc * 31 + static_cast<uint8_t>(vi);
                              });
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
    for (const auto &u : S) {
        uint64_t used = accumulate(
            u.begin(), u.begin() + K, uint64_t{0},
            [](uint64_t acc, int8_t ui) { return acc | (1ULL << ui); });
        for (int i = 0; i < K; i++)
            for (int y = 0; y < N; y++)
                if (!(used >> y & 1)) {
                    Vtx w = u;
                    w[i] = static_cast<int8_t>(y);
                    if (!in.count(w))
                        nb.insert(w);
                }
    }
    return static_cast<int>(nb.size());
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
    if (argc == 2 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
        cout
            << "Usage: " << argv[0] << " R k m seconds seed [noseed]\n"
            << "Search for an R-set in A(k+m,k) with boundary smaller than the "
               "embedded Hamming ball. Requires the Hamming embedding gate to "
               "be open.\n";
        return 0;
    }
    if (argc != 6 && argc != 7) {
        cerr << "Usage: " << argv[0] << " R k m seconds seed [noseed]\n"
             << "Run with --help for details.\n";
        return 2;
    }
    R = atoi(argv[1]);
    K = atoi(argv[2]);
    M = atoi(argv[3]);
    double secs = atof(argv[4]);
    unsigned seed = atoi(argv[5]);
    N = K + M;
    if (R < 1 || K < 1 || M < 1 || secs <= 0 || K > 24 || N > 64) {
        cerr << "Error: require R >= 1, k >= 1, m >= 1, seconds > 0, "
                "k <= 24, and k+m <= 64.\n";
        return 2;
    }
    const int d = 32 - __builtin_clz(static_cast<unsigned>(max(1, R - 1)));
    if (d > K || d > M) {
        cerr << "Error: embedded Hamming ball is infeasible (gate closed): "
             << "bit_length(R-1)=" << d << ", k=" << K << ", m=" << M << ".\n";
        return 2;
    }
    mt19937 rng(seed);
    Vtx c{};
    for (int i = 0; i < K; i++)
        c[i] = static_cast<int8_t>(i);
    long Hval = (long)(R * K - E(R)) * M - Cc(R);
    // starting sets: Hamming ball (binary order on d coords), balanced
    // rook-star, random connected
    vector<vector<Vtx>> starts;
    {
        vector<Vtx> S;
        for (int x = 0; x < R; x++) {
            Vtx v = c;
            for (int j = 0; j < d; j++)
                if (x >> j & 1)
                    v[j] = static_cast<int8_t>(K + j);
            S.push_back(v);
        }
        starts.push_back(S);
    }
    {
        vector<Vtx> S{c};
        for (int a = 0; a < R - 1; a++) {
            Vtx v = c;
            v[a % K] = static_cast<int8_t>(K + (a % M));
            if (find(S.begin(), S.end(), v) == S.end())
                S.push_back(v);
        }
        while ((int)S.size() < R) {
            Vtx v = S[rng() % S.size()];
            v[rng() % K] = static_cast<int8_t>(K + rng() % M);
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
                v[rng() % K] = static_cast<int8_t>(rng() % N);
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
            int ri = static_cast<int>(rng() % R);
            Vtx v = S[rng() % R];
            v[rng() % K] = static_cast<int8_t>(rng() % N);
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
        for (const auto &v : bestS) {
            printf(" (");
            for (int i = 0; i < K; i++)
                printf("%d%s", v[i], i + 1 < K ? "," : "");
            printf(")");
        }
        puts("");
    }
}
