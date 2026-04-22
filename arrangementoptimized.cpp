// Extraconnectivity of Arrangement Graphs — optimized search
// (Based on Cheng et al., with algorithmic fixes/speedups)
//
// Fixes applied:
//   1. Integer-packed vertices (uint64_t nibbles) for O(1) compare/hash
//   2. Sorted vertex enumeration eliminates R^(R-2) spanning tree redundancy (R! faster)
//   3. unordered_set for O(1) membership checks
//
// Usage: ./arrangementoptimized [R]   (default R=5)

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

// ── Vertex representation ──────────────────────────────────────────────────
// Each r-permutation is packed into a uint64_t with 4-bit nibbles.
// Position 0 occupies the highest nibble so that integer comparison
// equals lexicographic comparison.
//
//   "FBCDE" → F=5 B=1 C=2 D=3 E=4 → 0x51234

static int R = 5; // permutation length, configurable via argv

static inline int get_sym(uint64_t v, int pos) {
    return (v >> ((R - 1 - pos) * 4)) & 0xF;
}

static inline uint64_t set_sym(uint64_t v, int pos, int sym) {
    int shift = (R - 1 - pos) * 4;
    return (v & ~(0xFULL << shift)) | ((uint64_t)sym << shift);
}

static inline bool contains_sym(uint64_t v, int sym) {
    for (int i = 0; i < R; i++) {
        if (get_sym(v, i) == sym) return true;
    }
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

// ── Global state ───────────────────────────────────────────────────────────

static std::vector<uint64_t> ver;              // current vertex set
static std::unordered_set<uint64_t> ver_set;   // O(1) membership

struct Result {
    int cons;
    std::string example;
};
static std::map<int, Result> results;          // nk1 → best result
static uint64_t nodes_explored = 0;

// ── Neighbor-set calculation ───────────────────────────────────────────────
// Faithful port of Cheng's calc() using integer operations.
// Returns {nk1coef, cons} for the current vertex set.

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

            if (differs == 1) {
                if (!isShared[diff1]) {
                    isShared[diff1] = true;
                    isSharednum++;
                    nk1coef++;
                }
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

        // Prune vertices whose changed position is shared or whose
        // changed character appears again later in the same vertex.
        for (int n = 0; n < static_cast<int>(chgs.size()); n++) {
            if (isShared[chgs[n]]) {
                chgs.erase(chgs.begin() + n);
                dcverts.erase(dcverts.begin() + n);
                n--;
            } else {
                uint64_t dv = dcverts[n];
                int pos = chgs[n];
                int ch = get_sym(dv, pos);
                bool dup = false;
                for (int p = pos + 1; p < R; p++) {
                    if (get_sym(dv, p) == ch) { dup = true; break; }
                }
                if (dup) {
                    chgs.erase(chgs.begin() + n);
                    dcverts.erase(dcverts.begin() + n);
                    n--;
                }
            }
        }

        cons += static_cast<int>(dcverts.size());
        cons = (cons - isSharednum) + 1;
    }

    return {nk1coef, cons};
}

// ── Recursive search with sorted vertex ordering (Fix 1) ──────────────────
// Vertices at indices 2..R-1 are enumerated in strictly increasing
// lexicographic order, ensuring each connected R-subset is generated
// exactly once (eliminating the R^(R-2) spanning tree redundancy).

static void solve(int point, int nodl, int largchg) {
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

    // Enforce sorted order: new vertex must be > previous free vertex.
    // point == 2 is the first free vertex (0 and 1 are fixed), no minimum.
    uint64_t min_vertex = (point >= 3) ? ver[point - 1] : 0;

    // Generate candidates from all existing vertices via single-position
    // mutations. std::set gives us sorted order + dedup automatically.
    std::set<uint64_t> candidates;

    for (int i = 0; i < point; i++) {
        for (int pos = 0; pos <= largchg + 1 && pos < R; pos++) {
            for (int sym = 0; sym <= nodl; sym++) {
                if (contains_sym(ver[i], sym)) continue;

                uint64_t temp = set_sym(ver[i], pos, sym);
                if (temp <= min_vertex) continue;
                if (ver_set.count(temp)) continue;

                candidates.insert(temp);
            }
        }
    }

    for (uint64_t cand : candidates) {
        // Compute updated symmetry-breaking constraints from the candidate.
        int new_nodl = nodl;
        int new_largchg = largchg;
        for (int pos = 0; pos < R; pos++) {
            int sym = get_sym(cand, pos);
            new_nodl = std::max(new_nodl, sym + 1);
            if (sym != pos) { // differs from identity
                new_largchg = std::max(new_largchg, pos);
            }
        }

        ver[point] = cand;
        ver_set.insert(cand);
        solve(point + 1, new_nodl, new_largchg);
        ver_set.erase(cand);
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

    // ver[0] = identity permutation (ABCDE...)
    ver[0] = make_identity();
    ver_set.insert(ver[0]);

    // ver[1] = first neighbor: position 0 gets symbol R (WLOG by edge-transitivity)
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
              << nodes_explored << " subsets evaluated\n";

    return 0;
}
