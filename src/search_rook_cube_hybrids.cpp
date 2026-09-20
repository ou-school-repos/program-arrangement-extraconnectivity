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
int main(int argc, char **argv) {
    if (argc == 2 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
        cout << "Usage: " << argv[0] << " R [k m]\n"
             << "With only R, searches the tightest gate-open tall-slice cell "
                "k=R-1, m=bit_length(R-1).\n";
        return 0;
    }
    if (argc != 2 && argc != 4) {
        cerr << "Usage: " << argv[0] << " R [k m]\n"
             << "Run with --help for details.\n";
        return 2;
    }
    R = atoi(argv[1]);
    const bool inferred_parameters = argc == 2;
    if (argc == 2) {
        if (R < 2) {
            cerr << "Error: inferred mode requires R >= 2.\n";
            return 2;
        }
        K = R - 1;
        M = 0;
        for (unsigned value = static_cast<unsigned>(R - 1); value != 0;
             value >>= 1U)
            ++M;
    } else {
        K = atoi(argv[2]);
        M = atoi(argv[3]);
    }
    N = K + M;
    if (R < 1 || K < 1 || M < 1 || K > 24 || N > 64) {
        cerr << "Error: require R >= 1, k >= 1, m >= 1, k <= 24, and "
                "k+m <= 64.\n";
        return 2;
    }
    long Hval = (long)(R * K - E(R)) * M - Cc(R);
    int best = INT_MAX;
    string bestdesc;
    vector<Vtx> best_set;
    Vtx c{};
    for (int i = 0; i < K; i++)
        c[i] = static_cast<int8_t>(i);
    for (int cd = 0; cd <= min({4, K, M}); cd++)
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
                best_set = S;
                bestdesc = "cube dim " + to_string(cd) + " + " + to_string(a) +
                           " arms, greedy fill";
            }
        }
    if (inferred_parameters)
        printf("Inferred starting cell: k=%d, m=%d (tightest gate-open tall "
               "slice; "
               "not an optimality claim)\n",
               K, M);
    printf("R=%d A(%d,%d) m=%d: Hamming=%ld best structured=%d (%s) %s\n", R, N,
           K, M, Hval, best, bestdesc.c_str(),
           best < Hval ? "<-- BEATS HAMMING" : "");
    if (!best_set.empty()) {
        puts("best set:");
        for (const auto &vertex : best_set) {
            printf("(");
            for (int coordinate = 0; coordinate < K; ++coordinate)
                printf("%d%s", static_cast<int>(vertex[coordinate]),
                       coordinate + 1 < K ? "," : "");
            puts(")");
        }
    }
}
