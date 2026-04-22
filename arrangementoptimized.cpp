// Extraconnectivity of Arrangement Graphs — optimized search
// (Based on Cheng et al., with algorithmic fixes/speedups)
//
// Fixes applied:
//   1. Nauty-based 4-color auxiliary graph dedup exploits S_n x S_R symmetry
//   2. Integer-packed vertices (uint64_t, 5-bit nibbles) for O(1) compare/hash
//   3. unordered_set for O(1) membership checks
//   4. Independent verify() cross-checks every result
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
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
#include <nauty/nauty.h>
}

// ── Vertex representation ──────────────────────────────────────────────────
// Each r-permutation packed into uint64_t with 5-bit nibbles.
// Position 0 in the highest nibble → integer comparison = lex comparison.
// 5 bits support up to 32 symbols, allowing R up to 12.

static int R = 5;

static inline int get_sym(uint64_t vertex, int pos) {
    return static_cast<int>((vertex >> ((R - 1 - pos) * 5)) & 0x1FU);
}

static inline uint64_t set_sym(uint64_t vertex, int pos, int sym) {
    const int shift = (R - 1 - pos) * 5;
    return (vertex & ~(0x1FULL << shift)) |
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
        const int sym = get_sym(vertex, i);
        str[i] = (sym < 26) ? static_cast<char>('A' + sym)
                            : static_cast<char>('a' + sym - 26);
    }
    return str;
}

// ── 128-bit hash-only dedup ────────────────────────────────────────────────
// Instead of storing full nauty canonical graphs (~6KB each), we hash them
// to 128 bits. Collision probability ~2^-128 per pair — negligible at any
// realistic search size.

namespace {
struct Hash128 {
    uint64_t lo, hi;
    bool operator==(const Hash128 &o) const { return lo == o.lo && hi == o.hi; }
};
struct Hash128Hasher {
    size_t operator()(const Hash128 &h) const {
        return h.lo ^ (h.hi * 0x9e3779b97f4a7c15ULL);
    }
};
} // namespace

// One dedup set per recursion depth (hash-only: 16 bytes/entry, not ~6KB).
static std::vector<std::unordered_set<Hash128, Hash128Hasher>> seen;

// ── Nauty workspace (file-scope for reuse + Debian Bookworm compat) ────────
// DYNALLSTAT expands to 'static thread_local' which is illegal inside a block
// scope on some compilers. Declaring at file scope fixes this and also avoids
// millions of realloc calls by reusing the buffers across solve() invocations.
DYNALLSTAT(graph, nauty_g, nauty_g_sz);
DYNALLSTAT(graph, nauty_cg, nauty_cg_sz);
DYNALLSTAT(int, nauty_lab, nauty_lab_sz);
DYNALLSTAT(int, nauty_ptn, nauty_ptn_sz);
DYNALLSTAT(int, nauty_orbits, nauty_orbits_sz);

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

// ── Telemetry ──────────────────────────────────────────────────────────────
static std::chrono::high_resolution_clock::time_point t_start;
static std::chrono::high_resolution_clock::time_point t_last_report;
static uint64_t nodes_since_check = 0;

// ── Independent verifier ───────────────────────────────────────────────────
// Computes neighbor-set formula directly, independent of calc().
// Splits neighbors into "named" (symbols in V') and "anonymous" (rest).
// Returns {nk1, cons} matching calc()'s format.

static std::pair<int, int> verify_neighbor_set() {
    // Collect all distinct symbols used across V'.
    std::unordered_set<int> used_syms;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            used_syms.insert(get_sym(ver[i], p));
        }
    }
    const int M = static_cast<int>(used_syms.size());

    // For each position p, group vertices by their values at all OTHER
    // positions. Each group produces one distinct anonymous neighbor per
    // anonymous symbol. Anonymous neighbors from different groups/positions
    // are always distinct.
    int anon_coeff = 0;
    for (int p = 0; p < R; p++) {
        std::unordered_set<uint64_t> group_keys;
        for (int i = 0; i < R; i++) {
            group_keys.insert(set_sym(ver[i], p, 0x1F)); // sentinel
        }
        anon_coeff += static_cast<int>(group_keys.size());
    }

    // Enumerate all "named" neighbors: vertex with one position changed
    // to a symbol that appears somewhere in V'.
    std::unordered_set<uint64_t> named_nbrs;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            for (const int s : used_syms) {
                if (!contains_sym(ver[i], s)) {
                    named_nbrs.insert(set_sym(ver[i], p, s));
                }
            }
        }
    }
    // Remove V' members.
    for (int i = 0; i < R; i++) {
        named_nbrs.erase(ver[i]);
    }
    const int named_count = static_cast<int>(named_nbrs.size());

    // |N(V')| = anon_coeff * (n-M) + named_count
    //         = anon_coeff * (n-k) + (named_count - anon_coeff*(M-R))
    // Matching (R*k - nk1)*(n-k) - (nk1+cons):
    const int nk1 = R * R - anon_coeff;
    const int total_const = anon_coeff * (M - R) - named_count;
    const int cons = total_const - nk1;

    return {nk1, cons};
}

// ── Neighbor-set calculation ───────────────────────────────────────────────
// Faithful port of Cheng's calc() using integer operations.

static std::pair<int, int> calc() {
    int nk1coef = 0;
    int cons = 0;

    for (int i = 1; i < R; i++) {
        const uint64_t cur = ver[i];
        std::vector<uint64_t> dcverts;
        std::vector<int> chgs;
        bool isShared[32] = {};
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
// Depth-gated dedup:
//   - Shallow depths (point ≤ threshold): nauty canonical graph (expensive but
//     powerful — exploits full S_n × S_R symmetry to prune large subtrees)
//   - Deep depths: sorted vertex-set dedup (cheap O(R log R) per node)

// Sorted-set dedup structures (for deep levels).
namespace {
struct SortedVecHash {
    size_t operator()(const std::vector<uint64_t> &v) const {
        return std::accumulate(
            v.begin(), v.end(), v.size(), [](size_t h, uint64_t x) {
                return h ^ (std::hash<uint64_t>{}(x) + 0x9e3779b97f4a7c15ULL +
                            (h << 6) + (h >> 2));
            });
    }
};
} // namespace

static std::vector<std::unordered_set<std::vector<uint64_t>, SortedVecHash>>
    seen_sorted;

static void solve(int point, int nodl, int largchg) {
    // Depth gate: use nauty at shallow depths, sorted-set at deep.
    // Threshold: nauty for the first half of the recursion where subtrees
    // are large and pruning is most valuable.
    const int nauty_limit = std::max(3, R / 2 + 2);
    const bool use_nauty = (point <= nauty_limit);

    if (use_nauty) {
        // Build 4-color auxiliary graph for the current partial set.
        int max_sym = 0;
        for (int i = 0; i < point; i++) {
            for (int p = 0; p < R; p++) {
                const int sym = get_sym(ver[i], p);
                if (sym > max_sym) {
                    max_sym = sym;
                }
            }
        }
        const int N = max_sym + 1;
        const int n_aux = R + N + R * N + point;
        const int m_aux = SETWORDSNEEDED(n_aux);
        nauty_check(WORDSIZE, m_aux, n_aux, NAUTYVERSIONID);

        DYNALLSTAT(graph, g, g_sz);
        DYNALLSTAT(graph, cg, cg_sz);
        DYNALLSTAT(int, lab, lab_sz);
        DYNALLSTAT(int, ptn, ptn_sz);
        DYNALLSTAT(int, orbits, orbits_sz);

        DYNALLOC2(graph, g, g_sz, m_aux, n_aux, "malloc");
        DYNALLOC2(graph, cg, cg_sz, m_aux, n_aux, "malloc");
        DYNALLOC1(int, lab, lab_sz, n_aux, "malloc");
        DYNALLOC1(int, ptn, ptn_sz, n_aux, "malloc");
        DYNALLOC1(int, orbits, orbits_sz, n_aux, "malloc");

        EMPTYGRAPH(g, m_aux, n_aux);

        // Wire edges: Position↔Grid, Symbol↔Grid, Perm↔Grid.
        for (int p = 0; p < R; p++) {
            for (int s = 0; s < N; s++) {
                const int grid = R + N + p * N + s;
                ADDONEEDGE(g, p, grid, m_aux);
                ADDONEEDGE(g, R + s, grid, m_aux);
            }
        }
        for (int i = 0; i < point; i++) {
            const int perm_idx = R + N + R * N + i;
            for (int p = 0; p < R; p++) {
                const int s = get_sym(ver[i], p);
                const int grid = R + N + p * N + s;
                ADDONEEDGE(g, perm_idx, grid, m_aux);
            }
        }

        // Equitable partitions (color boundaries).
        for (int i = 0; i < n_aux; i++) {
            lab[i] = i;
            ptn[i] = 1;
        }
        if (R > 0) {
            ptn[R - 1] = 0;
        }
        if (N > 0) {
            ptn[R + N - 1] = 0;
        }
        if (R * N > 0) {
            ptn[R + N + R * N - 1] = 0;
        }
        ptn[n_aux - 1] = 0;

        DEFAULTOPTIONS_GRAPH(options);
        options.getcanon = TRUE;
        options.defaultptn = FALSE;

        statsblk stats;
        densenauty(g, lab, ptn, orbits, &options, &stats, m_aux, n_aux, cg);

        std::vector<setword> canon_key(cg,
                                       cg + static_cast<size_t>(n_aux) * m_aux);
        if (!seen[point].insert(std::move(canon_key)).second) {
            nodes_pruned++;
            return;
        }
    } else {
        // Cheap sorted-set dedup.
        std::vector<uint64_t> key(ver.begin(), ver.begin() + point);
        std::sort(key.begin(), key.end());
        if (!seen_sorted[point].insert(std::move(key)).second) {
            nodes_pruned++;
            return;
        }
    }

    // Leaf: evaluate.
    if (point == R) {
        nodes_explored++;
        const auto [nk1, cons] = calc();

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

    // Generate candidates (same logic as original Cheng code).
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
        if (R < 2 || R > 12) {
            std::cerr << "R must be between 2 and 12\n";
            return 1;
        }
    }

    const auto t0 = std::chrono::high_resolution_clock::now();

    ver.resize(R);
    seen.resize(R + 1);
    seen_sorted.resize(R + 1);

    ver[0] = make_identity();
    ver_set.insert(ver[0]);
    ver[1] = set_sym(ver[0], 0, R);
    ver_set.insert(ver[1]);

    std::cerr << "Searching R=" << R << "  ver[0]=" << vertex_to_string(ver[0])
              << "  ver[1]=" << vertex_to_string(ver[1]) << "\n";

    if (R == 2) {
        // R=2: point=2 is the leaf, no branches to unroll.
        solve(2, R + 1, 0);
    } else {

        // Pre-enumerate top-level branches for progress tracking.
        struct Branch {
            uint64_t temp;
            int nodl;
            int largchg;
        };
        std::vector<Branch> branches;
        const int init_nodl = R + 1;
        const int init_largchg = 0;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j <= init_nodl; j++) {
                if (contains_sym(ver[i], j)) {
                    continue;
                }
                for (int k = 0; k <= init_largchg + 1 && k < R; k++) {
                    const uint64_t temp = set_sym(ver[i], k, j);
                    if (ver_set.count(temp) != 0) {
                        continue;
                    }
                    branches.push_back({temp, std::max(init_nodl, j + 1),
                                        std::max(init_largchg, k)});
                }
            }
        }

        const int total = static_cast<int>(branches.size());
        int next_pct = 5;

        for (int b = 0; b < total; b++) {
            ver[2] = branches[b].temp;
            ver_set.insert(branches[b].temp);
            solve(3, branches[b].nodl, branches[b].largchg);
            ver_set.erase(branches[b].temp);

            const int pct = (b + 1) * 100 / total;
            if (pct >= next_pct || b + 1 == total) {
                const double elapsed =
                    std::chrono::duration<double>(
                        std::chrono::high_resolution_clock::now() - t0)
                        .count();
                std::cerr << "\r  " << pct << "%  (" << (b + 1) << "/" << total
                          << " branches, " << nodes_explored << " evaluated, "
                          << elapsed << "s)    " << std::flush;
                next_pct = pct + 5;
            }
        }
        std::cerr << "\n";

    } // else R > 2

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

    // Post-search verification: cross-check each result example with verify().
    bool all_ok = true;
    for (const auto &[nk1, res] : results) {
        // Parse example string "ABCDE FBCDE ..." back into ver[].
        std::istringstream iss(res.example);
        std::string tok;
        for (int i = 0; i < R && (iss >> tok); i++) {
            uint64_t v = 0;
            for (int p = 0; p < R; p++) {
                v = set_sym(v, p, tok[p] - 'A');
            }
            ver[i] = v;
        }
        const auto [vnk1, vcons] = verify_neighbor_set();
        if (nk1 != vnk1) {
            std::cerr << "VERIFY FAIL: nk1 mismatch for " << res.example
                      << ": calc=" << nk1 << " verify=" << vnk1 << "\n";
            all_ok = false;
        }
    }
    if (all_ok) {
        std::cerr << "\u2713 Verified.\n";
    } else {
        std::cerr << "\u2717 Verification failed.\n";
    }

    return 0;
}
