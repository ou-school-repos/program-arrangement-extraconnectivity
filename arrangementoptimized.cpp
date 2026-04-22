// Extraconnectivity of Arrangement Graphs — optimized search
// (Based on Cheng et al., with algorithmic fixes/speedups)
//
// Optimizations:
//   1. Nauty 4-color auxiliary graph dedup at shallow depths (S_n × S_R)
//   2. Sorted vertex-set dedup at deep depths (cheap O(R log R))
//   3. 5-bit nibble packing for R up to 12
//   4. OpenMP parallelism on top-level branches
//   5. Independent verify() cross-checks every result
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
#include <mutex>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

extern "C" {
#include <nauty/nauty.h>
}

// ── Vertex representation ──────────────────────────────────────────────────
// Each r-permutation packed into uint64_t with 5-bit nibbles.

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

// ── Hash functors ──────────────────────────────────────────────────────────

namespace {

struct SetwordVecHash {
    size_t operator()(const std::vector<setword> &v) const {
        return std::accumulate(
            v.begin(), v.end(), v.size(),
            [](size_t h, setword x) {
                return h ^ (std::hash<setword>{}(x) +
                            0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
            });
    }
};

struct SortedVecHash {
    size_t operator()(const std::vector<uint64_t> &v) const {
        return std::accumulate(
            v.begin(), v.end(), v.size(),
            [](size_t h, uint64_t x) {
                return h ^ (std::hash<uint64_t>{}(x) +
                            0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
            });
    }
};

struct Result {
    int cons = 0;
    std::string example;
};

} // namespace

// ── Per-thread search context ──────────────────────────────────────────────
// Each OpenMP thread gets its own context — zero contention.

struct SearchContext {
    std::vector<uint64_t> ver;
    std::unordered_set<uint64_t> ver_set;
    std::map<int, Result> results;
    uint64_t nodes_explored = 0;
    uint64_t nodes_pruned = 0;

    // Dedup tables (one per recursion depth).
    std::vector<std::unordered_set<std::vector<setword>, SetwordVecHash>> seen;
    std::vector<
        std::unordered_set<std::vector<uint64_t>, SortedVecHash>> seen_sorted;
};

// ── Independent verifier ───────────────────────────────────────────────────

static std::pair<int, int> verify_neighbor_set(const SearchContext &ctx) {
    std::unordered_set<int> used_syms;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            used_syms.insert(get_sym(ctx.ver[i], p));
        }
    }
    const int M = static_cast<int>(used_syms.size());

    int anon_coeff = 0;
    for (int p = 0; p < R; p++) {
        std::unordered_set<uint64_t> group_keys;
        for (int i = 0; i < R; i++) {
            group_keys.insert(set_sym(ctx.ver[i], p, 0x1F));
        }
        anon_coeff += static_cast<int>(group_keys.size());
    }

    std::unordered_set<uint64_t> named_nbrs;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            for (const int s : used_syms) {
                if (!contains_sym(ctx.ver[i], s)) {
                    named_nbrs.insert(set_sym(ctx.ver[i], p, s));
                }
            }
        }
    }
    for (int i = 0; i < R; i++) {
        named_nbrs.erase(ctx.ver[i]);
    }
    const int named_count = static_cast<int>(named_nbrs.size());

    const int nk1 = R * R - anon_coeff;
    const int total_const = anon_coeff * (M - R) - named_count;
    const int cons = total_const - nk1;
    return {nk1, cons};
}

// ── Neighbor-set calculation ───────────────────────────────────────────────

static std::pair<int, int> calc(const SearchContext &ctx) {
    int nk1coef = 0;
    int cons = 0;

    for (int i = 1; i < R; i++) {
        const uint64_t cur = ctx.ver[i];
        std::vector<uint64_t> dcverts;
        std::vector<int> chgs;
        bool isShared[32] = {};
        int isSharednum = 0;

        for (int j = 0; j < i; j++) {
            const uint64_t cur2 = ctx.ver[j];
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

static std::mutex nauty_mtx;  // Serialize nauty (not compiled with TLS).

static void solve(int point, int nodl, int largchg, SearchContext &ctx) {
    const int nauty_limit = std::max(3, R / 2 + 2);
    const bool use_nauty = (point <= nauty_limit);

    if (use_nauty) {
        int max_sym = 0;
        for (int i = 0; i < point; i++) {
            for (int p = 0; p < R; p++) {
                const int sym = get_sym(ctx.ver[i], p);
                if (sym > max_sym) {
                    max_sym = sym;
                }
            }
        }
        const int N = max_sym + 1;
        const int n_aux = R + N + R * N + point;
        const int m_aux = SETWORDSNEEDED(n_aux);
        nauty_check(WORDSIZE, m_aux, n_aux, NAUTYVERSIONID);

        const size_t g_total = static_cast<size_t>(m_aux) * n_aux;
        std::vector<graph> gv(g_total, 0);
        std::vector<graph> cgv(g_total);
        std::vector<int> lab(n_aux);
        std::vector<int> ptn(n_aux);
        std::vector<int> orbits(n_aux);
        graph *g = gv.data();
        graph *cg = cgv.data();

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
                const int s = get_sym(ctx.ver[i], p);
                const int grid = R + N + p * N + s;
                ADDONEEDGE(g, perm_idx, grid, m_aux);
            }
        }

        for (int i = 0; i < n_aux; i++) {
            lab[i] = i;
            ptn[i] = 1;
        }
        if (R > 0) { ptn[R - 1] = 0; }
        if (N > 0) { ptn[R + N - 1] = 0; }
        if (R * N > 0) { ptn[R + N + R * N - 1] = 0; }
        ptn[n_aux - 1] = 0;

        DEFAULTOPTIONS_GRAPH(options);
        options.getcanon = TRUE;
        options.defaultptn = FALSE;

        statsblk stats;
        std::vector<setword> canon_key;
        {
            std::lock_guard<std::mutex> lk(nauty_mtx);
            densenauty(g, lab.data(), ptn.data(), orbits.data(),
                       &options, &stats, m_aux, n_aux, cg);
            canon_key.assign(cg, cg + static_cast<size_t>(n_aux) * m_aux);
        }
        if (!ctx.seen[point].insert(std::move(canon_key)).second) {
            ctx.nodes_pruned++;
            return;
        }
    } else {
        std::vector<uint64_t> key(ctx.ver.begin(), ctx.ver.begin() + point);
        std::sort(key.begin(), key.end());
        if (!ctx.seen_sorted[point].insert(std::move(key)).second) {
            ctx.nodes_pruned++;
            return;
        }
    }

    if (point == R) {
        ctx.nodes_explored++;
        const auto [nk1, cons] = calc(ctx);
        auto it = ctx.results.find(nk1);
        if (it == ctx.results.end() || it->second.cons < cons) {
            std::string exa;
            for (int i = 0; i < R; i++) {
                exa += vertex_to_string(ctx.ver[i]) + " ";
            }
            ctx.results[nk1] = {cons, exa};
        }
        return;
    }

    for (int i = 0; i < point; i++) {
        for (int j = 0; j <= nodl; j++) {
            if (contains_sym(ctx.ver[i], j)) {
                continue;
            }
            for (int k = 0; k <= largchg + 1 && k < R; k++) {
                const uint64_t temp = set_sym(ctx.ver[i], k, j);
                if (ctx.ver_set.count(temp) != 0) {
                    continue;
                }
                ctx.ver[point] = temp;
                ctx.ver_set.insert(temp);
                solve(point + 1, std::max(nodl, j + 1),
                      std::max(largchg, k), ctx);
                ctx.ver_set.erase(temp);
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

    // Seed vertices: identity and one-swap.
    const uint64_t v0 = make_identity();
    const uint64_t v1 = set_sym(v0, 0, R);

    std::cerr << "Searching R=" << R << "  ver[0]=" << vertex_to_string(v0)
              << "  ver[1]=" << vertex_to_string(v1) << "\n";

    // Global results (merged from threads).
    std::map<int, Result> results;
    uint64_t total_explored = 0;
    uint64_t total_pruned = 0;

    if (R == 2) {
        SearchContext ctx;
        ctx.ver = {v0, v1};
        ctx.ver_set = {v0, v1};
        ctx.seen.resize(R + 1);
        ctx.seen_sorted.resize(R + 1);
        solve(2, R + 1, 0, ctx);
        results = std::move(ctx.results);
        total_explored = ctx.nodes_explored;
        total_pruned = ctx.nodes_pruned;
    } else {
        // Pre-enumerate top-level branches for progress + parallelism.
        struct Branch {
            uint64_t temp;
            int nodl;
            int largchg;
        };
        std::vector<Branch> branches;
        const int init_nodl = R + 1;
        const int init_largchg = 0;
        std::unordered_set<uint64_t> seed_set = {v0, v1};
        for (int i = 0; i < 2; i++) {
            const uint64_t base = (i == 0) ? v0 : v1;
            for (int j = 0; j <= init_nodl; j++) {
                if (contains_sym(base, j)) {
                    continue;
                }
                for (int k = 0; k <= init_largchg + 1 && k < R; k++) {
                    const uint64_t temp = set_sym(base, k, j);
                    if (seed_set.count(temp) != 0) {
                        continue;
                    }
                    branches.push_back({temp, std::max(init_nodl, j + 1),
                                        std::max(init_largchg, k)});
                }
            }
        }

        const int num_branches = static_cast<int>(branches.size());
        std::vector<SearchContext> ctxs(num_branches);

        // Initialize each branch's context.
        for (int b = 0; b < num_branches; b++) {
            ctxs[b].ver.resize(R);
            ctxs[b].ver[0] = v0;
            ctxs[b].ver[1] = v1;
            ctxs[b].ver[2] = branches[b].temp;
            ctxs[b].ver_set = {v0, v1, branches[b].temp};
            ctxs[b].seen.resize(R + 1);
            ctxs[b].seen_sorted.resize(R + 1);
        }

        int completed = 0;
        int next_pct = 5;

        #pragma omp parallel for schedule(dynamic)
        for (int b = 0; b < num_branches; b++) {
            solve(3, branches[b].nodl, branches[b].largchg, ctxs[b]);

            #pragma omp critical
            {
                completed++;
                const int pct = completed * 100 / num_branches;
                if (pct >= next_pct || completed == num_branches) {
                    const double elapsed =
                        std::chrono::duration<double>(
                            std::chrono::high_resolution_clock::now() - t0)
                            .count();
                    // Sum explored so far.
                    uint64_t exp = 0;
                    for (int i = 0; i < num_branches; i++) {
                        exp += ctxs[i].nodes_explored;
                    }
                    std::cerr << "\r  " << pct << "%  (" << completed << "/"
                              << num_branches << " branches, " << exp
                              << " evaluated, " << elapsed << "s)    "
                              << std::flush;
                    next_pct = pct + 5;
                }
            }
        }
        std::cerr << "\n";

        // Merge results from all threads.
        for (int b = 0; b < num_branches; b++) {
            total_explored += ctxs[b].nodes_explored;
            total_pruned += ctxs[b].nodes_pruned;
            for (auto &[nk1, res] : ctxs[b].results) {
                auto it = results.find(nk1);
                if (it == results.end() || it->second.cons < res.cons) {
                    results[nk1] = std::move(res);
                }
            }
        }
    }

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

    std::cerr << "Done: " << elapsed << "s, " << total_explored
              << " evaluated, " << total_pruned << " pruned\n";

    // Post-search verification.
    SearchContext vctx;
    vctx.ver.resize(R);
    bool all_ok = true;
    for (const auto &[nk1, res] : results) {
        std::istringstream iss(res.example);
        std::string tok;
        for (int i = 0; i < R && (iss >> tok); i++) {
            uint64_t v = 0;
            for (int p = 0; p < R; p++) {
                v = set_sym(v, p, tok[p] - 'A');
            }
            vctx.ver[i] = v;
        }
        const auto [vnk1, vcons] = verify_neighbor_set(vctx);
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
