// Extraconnectivity of Arrangement Graphs — optimized search
// (Based on Cheng et al., with algorithmic fixes/speedups)
//
// Fixes applied:
//   1. Canonical set dedup eliminates R^(R-2) spanning tree redundancy
//   2. Integer-packed vertices (uint64_t nibbles) for O(1) compare/hash
//   3. unordered_set for O(1) membership checks
//   4. nauty canonical graph labeling for isomorphism rejection (~R! reduction)
//
// Usage: ./arrangementoptimized [R]   (default R=5)
//
// Compile: g++ -O2 -std=c++17 -I/usr/include/nauty -lnauty ...

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
#include <nauty/nauty.h>
}

// ── Vertex representation ──────────────────────────────────────────────────
// Each r-permutation packed into uint64_t with 4-bit nibbles.
// Position 0 in the highest nibble → integer comparison = lex comparison.

static int R = 5;

static inline int get_sym(uint64_t v, int pos) {
    return (v >> ((R - 1 - pos) * 4)) & 0xF;
}

static inline uint64_t set_sym(uint64_t v, int pos, int sym) {
    int shift = (R - 1 - pos) * 4;
    return (v & ~(0xFULL << shift)) | ((uint64_t)sym << shift);
}

static inline bool contains_sym(uint64_t v, int sym) {
    for (int i = 0; i < R; i++)
        if (get_sym(v, i) == sym) return true;
    return false;
}

static inline uint64_t make_identity() {
    uint64_t v = 0;
    for (int i = 0; i < R; i++) v = set_sym(v, i, i);
    return v;
}

static std::string vertex_to_string(uint64_t v) {
    std::string s(R, ' ');
    for (int i = 0; i < R; i++) s[i] = 'A' + get_sym(v, i);
    return s;
}

// ── Canonical graph dedup via nauty ────────────────────────────────────────
// Build the induced subgraph of the partial vertex set on the arrangement
// graph (edges = Hamming distance 1), compute its canonical form via nauty,
// and use that as the dedup key. This collapses both:
//   - spanning tree redundancy (same set, different addition order)
//   - isomorphic copies (different vertices, same structure)

static std::vector<uint64_t> ver;
static std::unordered_set<uint64_t> ver_set;

struct CanonicalGraphHash {
    size_t operator()(const std::vector<graph>& v) const {
        size_t h = v.size();
        for (auto x : v) {
            h ^= std::hash<graph>{}(x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return h;
    }
};

// One dedup set per recursion depth.
static std::vector<std::unordered_set<std::vector<graph>, CanonicalGraphHash>> seen;

// Compute canonical graph for the first 'count' vertices in ver[].
// Returns the canonical adjacency matrix as a vector<graph>.
static std::vector<graph> canonical_graph(int count) {
    int n = count;
    int m = SETWORDSNEEDED(n);

    std::vector<graph> g(n * m);
    std::vector<graph> cg(n * m);
    std::vector<int> lab(n), ptn(n), orbits(n);

    EMPTYGRAPH(g.data(), m, n);

    // Build edges: vertices i,j are adjacent if they differ in exactly 1 position.
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            int diffs = 0;
            for (int k = 0; k < R; k++) {
                if (get_sym(ver[i], k) != get_sym(ver[j], k)) {
                    diffs++;
                    if (diffs > 1) break;
                }
            }
            if (diffs == 1) ADDONEEDGE(g.data(), i, j, m);
        }
    }

    DEFAULTOPTIONS_GRAPH(options);
    options.getcanon = TRUE;
    statsblk stats;

    densenauty(g.data(), lab.data(), ptn.data(), orbits.data(),
               &options, &stats, m, n, cg.data());

    return cg;
}



struct Result { int cons; std::string example; };
static std::map<int, Result> results;
static uint64_t nodes_explored = 0;
static uint64_t nodes_pruned = 0;

// ── Neighbor-set calculation ───────────────────────────────────────────────
// Faithful port of Cheng's calc() using integer operations.

static std::pair<int, int> calc() {
    int nk1coef = 0;
    int cons = 0;

    for (int i = 1; i < R; i++) {
        uint64_t cur = ver[i];
        std::vector<uint64_t> dcverts;
        std::vector<int> chgs;
        bool isShared[16] = {};
        int isSharednum = 0;

        for (int j = 0; j < i; j++) {
            uint64_t cur2 = ver[j];
            int differs = 0, diff1 = 0, diff2 = 0;

            for (int k = 0; k < R; k++) {
                if (get_sym(cur, k) != get_sym(cur2, k)) {
                    if (differs == 0) { diff1 = k; differs++; }
                    else if (differs == 1) { diff2 = k; differs++; }
                    else { differs++; break; }
                }
            }

            if (differs == 1 && !isShared[diff1]) {
                isShared[diff1] = true;
                isSharednum++;
                nk1coef++;
            }

            if (differs == 2) {
                if (diff1 > diff2) std::swap(diff1, diff2);
                if (get_sym(cur, diff1) != get_sym(cur2, diff2)) {
                    uint64_t v = set_sym(cur, diff1, get_sym(cur2, diff1));
                    if (std::find(dcverts.begin(), dcverts.end(), v) == dcverts.end()) {
                        dcverts.push_back(v);
                        chgs.push_back(diff1);
                    }
                }
                if (get_sym(cur, diff2) != get_sym(cur2, diff1)) {
                    uint64_t v = set_sym(cur, diff2, get_sym(cur2, diff2));
                    if (std::find(dcverts.begin(), dcverts.end(), v) == dcverts.end()) {
                        dcverts.push_back(v);
                        chgs.push_back(diff2);
                    }
                }
            }
        }

        for (int n = 0; n < static_cast<int>(chgs.size()); n++) {
            bool prune = false;
            if (isShared[chgs[n]]) {
                prune = true;
            } else {
                int pos = chgs[n];
                int ch = get_sym(dcverts[n], pos);
                for (int p = pos + 1; p < R; p++)
                    if (get_sym(dcverts[n], p) == ch) { prune = true; break; }
            }
            if (prune) {
                chgs.erase(chgs.begin() + n);
                dcverts.erase(dcverts.begin() + n);
                n--;
            }
        }

        cons += static_cast<int>(dcverts.size());
        cons = (cons - isSharednum) + 1;
    }
    return {nk1coef, cons};
}

// ── Recursive search ───────────────────────────────────────────────────────
// Original generation with nodl/largchg symmetry breaking preserved.
// nauty canonical graph dedup at each level eliminates both spanning tree
// redundancy AND isomorphic copies.

static void solve(int point, int nodl, int largchg) {
    // Compute canonical graph and check for duplicates.
    auto cg = canonical_graph(point);
    if (!seen[point].insert(std::move(cg)).second) {
        nodes_pruned++;
        return;
    }

    if (point == R) {
        nodes_explored++;
        auto [nk1, cons] = calc();
        auto it = results.find(nk1);
        if (it == results.end() || it->second.cons < cons) {
            std::string exa;
            for (int i = 0; i < R; i++) exa += vertex_to_string(ver[i]) + " ";
            results[nk1] = {cons, exa};
        }
        return;
    }

    // Generate candidates: same logic as original Cheng code.
    for (int i = 0; i < point; i++) {
        for (int j = 0; j <= nodl; j++) {
            if (contains_sym(ver[i], j)) continue;
            for (int k = 0; k <= largchg + 1 && k < R; k++) {
                uint64_t temp = set_sym(ver[i], k, j);
                if (ver_set.count(temp)) continue;

                ver[point] = temp;
                ver_set.insert(temp);
                solve(point + 1, std::max(nodl, j + 1), std::max(largchg, k));
                ver_set.erase(temp);
            }
        }
    }
}

// ── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        R = std::atoi(argv[1]);
        if (R < 2 || R > 16) {
            std::cerr << "R must be between 2 and 16\n";
            return 1;
        }
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    ver.resize(R);
    seen.resize(R + 1);

    ver[0] = make_identity();
    ver_set.insert(ver[0]);
    ver[1] = set_sym(ver[0], 0, R);
    ver_set.insert(ver[1]);

    std::cerr << "Searching R=" << R
              << "  ver[0]=" << vertex_to_string(ver[0])
              << "  ver[1]=" << vertex_to_string(ver[1]) << "\n";

    solve(2, R + 1, 0);

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    for (auto& [nk1, res] : results) {
        std::cout << "(" << R << "nk-" << nk1
                  << ") (n-k)-" << (nk1 + res.cons)
                  << ", EX: " << res.example << "\n";
    }

    std::cerr << "Done: " << elapsed << "s, "
              << nodes_explored << " evaluated, "
              << nodes_pruned << " pruned\n";

    return 0;
}
