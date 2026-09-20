// greedy.cpp — structured witness search: seed with a partial rook-star (a
// arms) and/or a binary cube core (dimension c), then grow greedily to R
// vertices, each step adding the set-neighbour that minimises the directly
// counted external boundary. Reports any set beating the embedded Hamming ball.
// Usage: ./greedy R k m
#include <bits/stdc++.h>
using namespace std;
using Vtx = array<int8_t, 24>;
int R, K, M, N;
struct Hh {
    size_t operator()(const Vtx &v) const {
        size_t h = accumulate(v.begin(), v.begin() + K, size_t{0},
                              [](size_t acc, int8_t vi) {
                                  return acc * 31 + static_cast<uint8_t>(vi);
                              });
        return h;
    }
};
int boundary(const vector<Vtx> &S) {
    unordered_set<Vtx, Hh> in(S.begin(), S.end()), nb;
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
int main(int /*argc*/, char **argv) {
    R = atoi(argv[1]);
    K = atoi(argv[2]);
    M = atoi(argv[3]);
    N = K + M;
    long Hval = (long)(R * K - E(R)) * M - Cc(R);
    int best = INT_MAX;
    string bestdesc;
    Vtx c{};
    for (int i = 0; i < K; i++)
        c[i] = static_cast<int8_t>(i);
    for (int cd = 0; cd <= 4; cd++)
        for (int a = 0; a <= R - 1; a++) {
            vector<Vtx> S;
            set<Vtx> in;
            for (int x = 0; x < (1 << cd) && (int)S.size() < R; x++) {
                Vtx v = c;
                for (int j = 0; j < cd; j++)
                    if (x >> j & 1)
                        v[j] = static_cast<int8_t>(K + j);
                if (in.insert(v).second)
                    S.push_back(v);
            } // cube core on coords 0..cd-1, fresh symbols K..K+cd-1
            for (int t = 0; t < a && (int)S.size() < R; t++) {
                Vtx v = c;
                int coord = (cd + t) % K;
                v[coord] = static_cast<int8_t>(K + ((cd + t) % M));
                bool ok = true;
                for (int i = 0; i < K; i++)
                    for (int j = i + 1; j < K; j++)
                        if (v[i] == v[j])
                            ok = false;
                if (ok && in.insert(v).second)
                    S.push_back(v);
            } // star arms in fresh coords, round-robin symbols
            if (S.empty()) {
                S.push_back(c);
                in.insert(c);
            }
            while ((int)S.size() < R) {
                int bb = INT_MAX;
                Vtx bv{};
                set<Vtx> cand;
                for (const auto &u : S) {
                    uint64_t used =
                        accumulate(u.begin(), u.begin() + K, uint64_t{0},
                                   [](uint64_t acc, int8_t ui) {
                                       return acc | (1ULL << ui);
                                   });
                    for (int i = 0; i < K; i++)
                        for (int y = 0; y < N; y++)
                            if (!(used >> y & 1)) {
                                Vtx w = u;
                                w[i] = static_cast<int8_t>(y);
                                if (!in.count(w))
                                    cand.insert(w);
                            }
                }
                for (const auto &w : cand) {
                    S.push_back(w);
                    int b = boundary(S);
                    S.pop_back();
                    if (b < bb) {
                        bb = b;
                        bv = w;
                    }
                }
                S.push_back(bv);
                in.insert(bv);
            }
            int b = boundary(S);
            if (b < best) {
                best = b;
                bestdesc = "cube dim " + to_string(cd) + " + " + to_string(a) +
                           " arms, greedy fill";
            }
        }
    printf("R=%d A(%d,%d) m=%d: Hamming=%ld best structured=%d (%s) %s\n", R, N,
           K, M, Hval, best, bestdesc.c_str(),
           best < Hval ? "<-- BEATS HAMMING" : "");
}
