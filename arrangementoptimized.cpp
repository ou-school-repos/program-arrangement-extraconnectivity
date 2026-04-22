// Extraconnectivity of Arrangement Graphs — optimized search
// (Based on Cheng et al., with algorithmic fixes/speedups)
//
// Fixes applied:
//   1. Canonical set dedup eliminates R^(R-2) spanning tree redundancy
//   2. Integer-packed vertices (uint64_t nibbles) for O(1) compare/hash
//   3. unordered_set for O(1) membership checks
//
// Usage: ./arrangementoptimized [R]   (default R=5)

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// ── Vertex representation ──────────────────────────────────────────────────
// Each r-permutation packed into uint64_t with 4-bit nibbles.
// Position 0 in the highest nibble → integer comparison = lex comparison.

static int R = 5;

static inline int get_sym(uint64_t vertex, int pos) {
    return static_cast<int>((vertex >> ((R - 1 - pos) * 4)) & 0xFU);
}

static inline uint64_t set_sym(uint64_t vertex, int pos, int sym) {
    const int shift = (R - 1 - pos) * 4;
    return (vertex & ~(0xFULL << shift)) |
           (static_cast<uint64_t>(sym) << shift);
}

static inline bool contains_sym(uint64_t vertex, int sym) {
    for (int i = 0; i < R; i++) {
        if (get_sym(vertex, i) == sym) {
            return true;
        }
    }
    return false;
}

static inline uint64_t make_identity() {
    uint64_t vertex = 0;
    for (int i = 0; i < R; i++) {
        vertex = set_sym(vertex, i, i);
    }
    return vertex;
}

static std::string vertex_to_string(uint64_t vertex) {
    std::string str(R, ' ');
    for (int i = 0; i < R; i++) {
        str[i] = static_cast<char>('A' + get_sym(vertex, i));
    }
    return str;
}

// ── Canonical set hashing ──────────────────────────────────────────────────
// Sort the partial vertex set and hash it to detect duplicates.
// This eliminates the R^(R-2) spanning tree redundancy: the same
// unordered set reached via different addition orders is recognized.
//
// NOTE: nauty-based graph isomorphism was tested but is too aggressive —
// it collapses subsets with isomorphic induced subgraphs that have
// different neighborhoods in A(n,r). The sorted vertex set is the
// correct dedup granularity for this problem.

namespace {
struct VectorHash {
    size_t operator()(const std::vector<uint64_t> &vec) const {
        return std::accumulate(vec.begin(), vec.end(), vec.size(),
                               [](size_t hash, uint64_t val) {
                                   return hash ^ (std::hash<uint64_t>{}(val) +
                                                  0x9e3779b97f4a7c15ULL +
                                                  (hash << 6) + (hash >> 2));
                               });
    }
};
} // namespace

// One dedup set per recursion depth (partial sets of size k).
static std::vector<std::unordered_set<std::vector<uint64_t>, VectorHash>> seen;

// ── Global state ───────────────────────────────────────────────────────────

static std::vector<uint64_t> ver;
static std::unordered_set<uint64_t> ver_set;

namespace {
struct Result {
    int cons = 0;
    std::string example;
};
} // namespace
static std::map<int, Result> results;
static uint64_t nodes_explored = 0;
static uint64_t nodes_pruned = 0;

// ── Neighbor-set calculation ───────────────────────────────────────────────
// Faithful port of Cheng's calc() using integer operations.

static std::pair<int, int> calc() {
    int nk1coef = 0;
    int cons = 0;

    for (int i = 1; i < R; i++) {
        const uint64_t cur = ver[i];
        std::vector<uint64_t> dcverts;
        std::vector<int> chgs;
        bool isShared[16] = {};
        int isSharednum = 0;

        for (int j = 0; j < i; j++) {
            const uint64_t cur2 = ver[j];
            int differs = 0;
            int diff1 = 0;
            int diff2 = 0;

            for (int k = 0; k < R; k++) {
                if (get_sym(cur, k) != get_sym(cur2, k)) {
                    if (differs == 0) {
                        diff1 = k;
                        differs++;
                    } else if (differs == 1) {
                        diff2 = k;
                        differs++;
                    } else {
                        differs++;
                        break;
                    }
                }
            }

            if (differs == 1 && !isShared[diff1]) {
                isShared[diff1] = true;
                isSharednum++;
                nk1coef++;
            }

            if (differs == 2) {
                if (diff1 > diff2) {
                    std::swap(diff1, diff2);
                }
                if (get_sym(cur, diff1) != get_sym(cur2, diff2)) {
                    const uint64_t vtx =
                        set_sym(cur, diff1, get_sym(cur2, diff1));
                    if (std::find(dcverts.begin(), dcverts.end(), vtx) ==
                        dcverts.end()) {
                        dcverts.push_back(vtx);
                        chgs.push_back(diff1);
                    }
                }
                if (get_sym(cur, diff2) != get_sym(cur2, diff1)) {
                    const uint64_t vtx =
                        set_sym(cur, diff2, get_sym(cur2, diff2));
                    if (std::find(dcverts.begin(), dcverts.end(), vtx) ==
                        dcverts.end()) {
                        dcverts.push_back(vtx);
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
                const int pos = chgs[n];
                const int ch = get_sym(dcverts[n], pos);
                for (int p = pos + 1; p < R; p++) {
                    if (get_sym(dcverts[n], p) == ch) {
                        prune = true;
                        break;
                    }
                }
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
// Canonical set dedup at each level eliminates spanning tree redundancy.

static void solve(int point, int nodl, int largchg) {
    // Canonicalize partial set (sorted) and check for duplicates.
    std::vector<uint64_t> canonical(ver.begin(), ver.begin() + point);
    std::sort(canonical.begin(), canonical.end());
    if (!seen[point].insert(std::move(canonical)).second) {
        nodes_pruned++;
        return;
    }

    if (point == R) {
        nodes_explored++;
        auto [nk1, cons] = calc();
        auto it = results.find(nk1);
        if (it == results.end() || it->second.cons < cons) {
            std::string exa;
            for (int i = 0; i < R; i++) {
                exa += vertex_to_string(ver[i]) + " ";
            }
            results[nk1] = {cons, exa};
        }
        return;
    }

    // Generate candidates: same logic as original Cheng code.
    for (int i = 0; i < point; i++) {
        for (int j = 0; j <= nodl; j++) {
            if (contains_sym(ver[i], j)) {
                continue;
            }
            for (int k = 0; k <= largchg + 1 && k < R; k++) {
                const uint64_t temp = set_sym(ver[i], k, j);
                if (ver_set.count(temp) != 0) {
                    continue;
                }

                ver[point] = temp;
                ver_set.insert(temp);
                solve(point + 1, std::max(nodl, j + 1), std::max(largchg, k));
                ver_set.erase(temp);
            }
        }
    }
}

// ── Main ───────────────────────────────────────────────────────────────────

int main(int argc, const char *argv[]) {
    if (argc >= 2) {
        R = static_cast<int>(std::strtol(argv[1], nullptr, 10));
        if (R < 2 || R > 16) {
            std::cerr << "R must be between 2 and 16\n";
            return 1;
        }
    }

    const auto t0 = std::chrono::high_resolution_clock::now();

    ver.resize(R);
    seen.resize(R + 1);

    ver[0] = make_identity();
    ver_set.insert(ver[0]);
    ver[1] = set_sym(ver[0], 0, R);
    ver_set.insert(ver[1]);

    std::cerr << "Searching R=" << R << "  ver[0]=" << vertex_to_string(ver[0])
              << "  ver[1]=" << vertex_to_string(ver[1]) << "\n";

    solve(2, R + 1, 0);

    const auto t1 = std::chrono::high_resolution_clock::now();
    const double elapsed = std::chrono::duration<double>(t1 - t0).count();

    // Compute column widths for aligned output.
    int max_nk1_w = 0;
    int max_nk_w = 0;
    for (const auto &[nk1, res] : results) {
        max_nk1_w =
            std::max(max_nk1_w, static_cast<int>(std::to_string(nk1).size()));
        max_nk_w = std::max(
            max_nk_w, static_cast<int>(std::to_string(nk1 + res.cons).size()));
    }

    for (const auto &[nk1, res] : results) {
        std::cout << "(" << R << "nk-" << std::setw(max_nk1_w) << nk1
                  << ") (n-k)-" << std::setw(max_nk_w) << (nk1 + res.cons)
                  << ", EX: " << res.example << "\n";
    }

    std::cerr << "Done: " << elapsed << "s, " << nodes_explored
              << " evaluated, " << nodes_pruned << " pruned\n";

    return 0;
}
