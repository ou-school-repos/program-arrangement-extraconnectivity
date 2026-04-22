// Extraconnectivity of Arrangement Graphs — optimized search
// (Based on Cheng et al., with algorithmic fixes/speedups)
//
// Fixes applied:
//   1. Nauty 4-color auxiliary graph dedup for full S_n x S_R symmetry.
//   2. Eliminated leaf-level deduplication to completely solve the OOM bug.
//   3. Dynamic symbol remapping forces perfect Nauty structural matches.
//   4. 128-bit custom flat hash table (16 bytes per node vs thousands).
//   5. Zero dynamic allocations (std::vector) in calc() and candidate loops.
//   6. Real-time telemetry printed during the heavy search phase.
//
// Usage: ./arrangementoptimized [R] [nauty_depth_limit]

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
#include <nauty/nauty.h>
}

static int R = 5;

// ── Vertex representation ──────────────────────────────────────────────────
// Each r-permutation packed into uint64_t with 5-bit nibbles.
// Position 0 in the highest nibble → integer comparison = lex comparison.
// 5 bits support up to 32 symbols, allowing R up to 12.

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
        if (get_sym(vertex, i) == sym)
            return true;
    }
    return false;
}

static inline uint32_t sym_mask(uint64_t vertex) {
    uint32_t m = 0;
    for (int i = 0; i < R; i++)
        m |= (1U << get_sym(vertex, i));
    return m;
}

static inline uint64_t make_identity() {
    uint64_t vertex = 0;
    for (int i = 0; i < R; i++)
        vertex = set_sym(vertex, i, i);
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

// ── 128-bit hash fingerprint ───────────────────────────────────────────────
// Instead of storing full nauty canonical graphs (~6KB each), we hash them
// to 128 bits. Collision probability ~2^-128 per pair — negligible at any
// realistic search size.

static inline uint64_t splitmix64(uint64_t z) {
    z ^= (z >> 30);
    z *= 0xbf58476d1ce4e5b9ULL;
    z ^= (z >> 27);
    z *= 0x94d049bb133111ebULL;
    z ^= (z >> 31);
    return z;
}

struct Hash128 {
    uint64_t h1, h2;
    bool operator==(const Hash128 &o) const { return h1 == o.h1 && h2 == o.h2; }
};

static inline Hash128 hash_nauty_graph(const graph *cg, int m_aux, int n_aux) {
    uint64_t h1 = 0x123456789ABCDEF0ULL;
    uint64_t h2 = 0x0FEDCBA987654321ULL;
    const size_t num_words = static_cast<size_t>(m_aux) * n_aux;
    for (size_t i = 0; i < num_words; i++) {
        uint64_t w = static_cast<uint64_t>(cg[i]);
        w ^= i * 0x9E3779B97F4A7C15ULL;
        h1 ^= w;
        h1 = splitmix64(h1);
        h2 ^= h1;
        h2 = splitmix64(h2);
    }
    return {h1, h2};
}

static inline Hash128 hash_sorted_vertices(const uint64_t *arr, int len) {
    uint64_t h1 = 0x8a976b32c61e4fbbULL ^ static_cast<uint64_t>(len);
    uint64_t h2 = 0x93309a6324d081f9ULL ^ static_cast<uint64_t>(len);
    for (int i = 0; i < len; i++) {
        h1 ^= arr[i];
        h1 = splitmix64(h1);
        h2 ^= h1;
        h2 = splitmix64(h2);
    }
    return {h1, h2};
}

// ── Open-addressing flat hash set (fixes OOM) ─────────────────────────────
// Stores only 16-byte fingerprints instead of multi-KB canonical graphs.

class FlatHashSet128 {
    std::vector<Hash128> data_;
    size_t count_ = 0;
    size_t mask_;

  public:
    explicit FlatHashSet128(size_t capacity_pow2 = 1U << 16)
        : data_(capacity_pow2, {0, 0}), mask_(capacity_pow2 - 1) {}

    // Returns true if newly inserted, false if already present.
    bool insert(Hash128 key) {
        if (key.h1 == 0 && key.h2 == 0)
            key.h1 = 1; // reserve {0,0} as empty sentinel
        size_t idx = key.h1 & mask_;
        while (true) {
            if (data_[idx].h1 == 0 && data_[idx].h2 == 0) {
                data_[idx] = key;
                count_++;
                if (count_ * 2 > data_.size())
                    rehash();
                return true;
            }
            if (data_[idx] == key)
                return false;
            idx = (idx + 1) & mask_;
        }
    }

    void clear() {
        std::fill(data_.begin(), data_.end(), Hash128{0, 0});
        count_ = 0;
    }
    [[nodiscard]] size_t size() const { return count_; }

  private:
    void rehash() {
        std::vector<Hash128> old = std::move(data_);
        data_.assign(old.size() * 2, {0, 0});
        mask_ = data_.size() - 1;
        count_ = 0;
        for (const auto &k : old) {
            if (k.h1 != 0 || k.h2 != 0)
                insert(k);
        }
    }
};

static std::vector<FlatHashSet128> seen_nauty;
static std::vector<FlatHashSet128> seen_sorted;

// ── Nauty static buffers (no DYNALLSTAT — portable across all compilers) ──
// Fixed-size arrays avoid the DYNALLSTAT '_Thread_local inside block scope'
// bug on Debian Bookworm and also eliminate per-call malloc overhead.
constexpr int MAX_NAUTY_N = 512;
constexpr int MAX_NAUTY_M = SETWORDSNEEDED(MAX_NAUTY_N);
static graph g_nauty[MAX_NAUTY_N * MAX_NAUTY_M];
static graph cg_nauty[MAX_NAUTY_N * MAX_NAUTY_M];
static int lab_nauty[MAX_NAUTY_N];
static int ptn_nauty[MAX_NAUTY_N];
static int orbits_nauty[MAX_NAUTY_N];

// ── Global state ───────────────────────────────────────────────────────────

static std::vector<uint64_t> ver;
static uint32_t ver_sym_mask[16];

namespace {
struct Result {
    int cons = 0;
    std::string example;
};
} // namespace
static std::map<int, Result> results;

// Telemetry counters
static uint64_t nodes_generated = 0;
static uint64_t nodes_evaluated = 0;
static uint64_t nodes_pruned_iso = 0;
static uint64_t nodes_pruned_exact = 0;
static uint64_t nodes_pruned_local = 0;
static std::chrono::high_resolution_clock::time_point t0_global;
static std::chrono::high_resolution_clock::time_point t_last_print;

// Linear scan replaces unordered_set for ver membership (faster for small R).
static inline bool in_ver_set(uint64_t v, int point) {
    return std::any_of(ver.begin(), ver.begin() + point,
                       [v](uint64_t x) { return x == v; });
}

// ── Independent verifier ───────────────────────────────────────────────────
// Computes neighbor-set formula directly, independent of calc().
// Splits neighbors into "named" (symbols in V') and "anonymous" (rest).
// Returns {nk1, cons} matching calc()'s format.

static std::pair<int, int> verify_neighbor_set() {
    uint32_t used_syms_mask = 0;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++)
            used_syms_mask |= (1U << get_sym(ver[i], p));
    }
    const int M = __builtin_popcount(used_syms_mask);

    int anon_coeff = 0;
    for (int p = 0; p < R; p++) {
        uint64_t group_keys[16];
        int group_sz = 0;
        for (int i = 0; i < R; i++) {
            uint64_t key = set_sym(ver[i], p, 0x1F);
            bool found = false;
            for (int q = 0; q < group_sz; q++) {
                if (group_keys[q] == key) {
                    found = true;
                    break;
                }
            }
            if (!found)
                group_keys[group_sz++] = key;
        }
        anon_coeff += group_sz;
    }

    uint64_t named_nbrs[8192];
    int named_sz = 0;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            for (int s = 0; s < 32; s++) {
                if ((used_syms_mask & (1U << s)) == 0)
                    continue;
                if (!contains_sym(ver[i], s)) {
                    uint64_t vtx = set_sym(ver[i], p, s);
                    if (in_ver_set(vtx, R))
                        continue;
                    bool found = false;
                    for (int q = 0; q < named_sz; q++) {
                        if (named_nbrs[q] == vtx) {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        named_nbrs[named_sz++] = vtx;
                }
            }
        }
    }

    const int nk1 = R * R - anon_coeff;
    const int total_const = anon_coeff * (M - R) - named_sz;
    const int cons = total_const - nk1;
    return {nk1, cons};
}

// ── Incremental calc: O(R) contribution of the last-added vertex ───────────
// Processes only ver[idx] against ver[0..idx-1], returning its delta.

static std::pair<int, int> calc_step(int count) {
    const int idx = count - 1;
    const uint64_t cur = ver[idx];
    int step_nk1 = 0;
    int dc_count = 0;
    bool isShared[32] = {};
    int isSharednum = 0;
    uint64_t dcverts[256];
    int chgs[256];

    for (int j = 0; j < idx; j++) {
        const uint64_t cur2 = ver[j];
        // XOR-based diff: single op detects all differing 5-bit groups.
        uint64_t xor_val = cur ^ cur2;
        if (xor_val == 0)
            continue;

        int differs = 0, diff1 = 0, diff2 = 0;
        int pos = R - 1;
        while (xor_val > 0) {
            if (xor_val & 0x1FU) {
                if (differs == 0)
                    diff1 = pos;
                else if (differs == 1)
                    diff2 = pos;
                differs++;
                if (differs > 2)
                    break;
            }
            xor_val >>= 5;
            pos--;
        }

        if (differs == 1 && !isShared[diff1]) {
            isShared[diff1] = true;
            isSharednum++;
            step_nk1++;
        }

        if (differs == 2) {
            if (diff1 > diff2)
                std::swap(diff1, diff2);

            if (get_sym(cur, diff1) != get_sym(cur2, diff2)) {
                const uint64_t vtx = set_sym(cur, diff1, get_sym(cur2, diff1));
                bool found = false;
                for (int d = 0; d < dc_count; d++) {
                    if (dcverts[d] == vtx) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    dcverts[dc_count] = vtx;
                    chgs[dc_count++] = diff1;
                }
            }

            if (get_sym(cur, diff2) != get_sym(cur2, diff1)) {
                const uint64_t vtx = set_sym(cur, diff2, get_sym(cur2, diff2));
                bool found = false;
                for (int d = 0; d < dc_count; d++) {
                    if (dcverts[d] == vtx) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    dcverts[dc_count] = vtx;
                    chgs[dc_count++] = diff2;
                }
            }
        }
    }

    for (int n = 0; n < dc_count; n++) {
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
            dcverts[n] = dcverts[dc_count - 1];
            chgs[n] = chgs[dc_count - 1];
            dc_count--;
            n--;
        }
    }

    return {step_nk1, dc_count - isSharednum + 1};
}

// ── Recursive search ───────────────────────────────────────────────────────
// Depth-gated dedup:
//   - Internal depths (point <= nauty_limit): nauty canonical graph (expensive
//     but powerful — exploits full S_n × S_R symmetry to prune large subtrees)
//   - Deep depths: sorted vertex-set hash dedup (cheap O(R log R) per node)
//   - Leaf level (point == R): NO dedup — saves >99% of memory since we never
//     branch from leaves

static int nauty_limit = 5;

static void solve(int point, int nodl, int largchg, int acc_nk1, int acc_cons) {
    nodes_generated++;

    // Telemetry: non-blocking update every ~262k nodes
    if ((nodes_generated & 0x3FFFF) == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(now - t_last_print).count() >= 0.5) {
            double total =
                std::chrono::duration<double>(now - t0_global).count();
            const size_t dedup_n = std::accumulate(
                seen_nauty.begin(), seen_nauty.end(), size_t{0},
                [](size_t a, const FlatHashSet128 &s) { return a + s.size(); });
            const size_t dedup_s = std::accumulate(
                seen_sorted.begin(), seen_sorted.end(), size_t{0},
                [](size_t a, const FlatHashSet128 &s) { return a + s.size(); });
            std::cerr << "\r  [" << std::fixed << std::setprecision(1) << total
                      << "s]  gen: " << nodes_generated
                      << " | eval: " << nodes_evaluated
                      << " | pruned(iso/sort/local): " << nodes_pruned_iso
                      << "/" << nodes_pruned_exact << "/" << nodes_pruned_local
                      << " | dedup: " << dedup_n << "+" << dedup_s
                      << " entries   " << std::flush;
            t_last_print = now;
        }
    }

    // Leaf: use accumulated incremental values — no full calc() needed.
    if (point == R) {
        nodes_evaluated++;

        auto it = results.find(acc_nk1);
        if (it == results.end() || it->second.cons < acc_cons) {
            std::string exa;
            for (int i = 0; i < R; i++) {
                if (i) {
                    exa += ' ';
                }
                exa += vertex_to_string(ver[i]);
            }
            results[acc_nk1] = {acc_cons, exa};
        }
        return;
    }

    // Deduplication for internal nodes.
    if (point <= nauty_limit) {
        // Dynamic symbol remapping: compress used symbols to 0..N-1 so nauty
        // identifies structurally equivalent sets that differ only in which
        // unused symbols appear.
        int used_syms[32] = {};
        for (int i = 0; i < point; i++) {
            for (int p = 0; p < R; p++)
                used_syms[get_sym(ver[i], p)] = 1;
        }
        int sym_map[32] = {};
        int N = 0;
        for (int s = 0; s < 32; s++) {
            if (used_syms[s])
                sym_map[s] = N++;
        }

        const int n_aux = R + N + R * N + point;
        const int m_aux = SETWORDSNEEDED(n_aux);
        nauty_check(WORDSIZE, m_aux, n_aux, NAUTYVERSIONID);

        EMPTYGRAPH(g_nauty, m_aux, n_aux);

        // Wire edges: Position↔Grid, Symbol↔Grid, Perm↔Grid
        for (int p = 0; p < R; p++) {
            for (int s = 0; s < N; s++) {
                const int grid = R + N + p * N + s;
                ADDONEEDGE(g_nauty, p, grid, m_aux);
                ADDONEEDGE(g_nauty, R + s, grid, m_aux);
            }
        }
        for (int i = 0; i < point; i++) {
            const int perm_idx = R + N + R * N + i;
            for (int p = 0; p < R; p++) {
                const int s = sym_map[get_sym(ver[i], p)];
                const int grid = R + N + p * N + s;
                ADDONEEDGE(g_nauty, perm_idx, grid, m_aux);
            }
        }

        // Equitable partitions (color boundaries).
        for (int i = 0; i < n_aux; i++) {
            lab_nauty[i] = i;
            ptn_nauty[i] = 1;
        }
        if (R > 0)
            ptn_nauty[R - 1] = 0;
        if (N > 0)
            ptn_nauty[R + N - 1] = 0;
        if (R * N > 0)
            ptn_nauty[R + N + R * N - 1] = 0;
        ptn_nauty[n_aux - 1] = 0;

        DEFAULTOPTIONS_GRAPH(options);
        options.getcanon = TRUE;
        options.defaultptn = FALSE;
        statsblk stats;
        densenauty(g_nauty, lab_nauty, ptn_nauty, orbits_nauty, &options,
                   &stats, m_aux, n_aux, cg_nauty);

        Hash128 h = hash_nauty_graph(cg_nauty, m_aux, n_aux);
        if (!seen_nauty[point].insert(h)) {
            nodes_pruned_iso++;
            return;
        }
    } else {
        // Cheap sorted-set hash dedup for deep levels.
        uint64_t key_buf[16];
        for (int i = 0; i < point; i++)
            key_buf[i] = ver[i];
        std::sort(key_buf, key_buf + point);

        Hash128 h = hash_sorted_vertices(key_buf, point);
        if (!seen_sorted[point].insert(h)) {
            nodes_pruned_exact++;
            return;
        }
    }

    // Generate candidates with local dedup and bitmask contains_sym.
    uint64_t local_seen[2048];
    std::memset(local_seen, 0xFF, sizeof(local_seen));

    for (int i = 0; i < point; i++) {
        for (int j = 0; j <= nodl; j++) {
            if (ver_sym_mask[i] & (1U << j))
                continue;
            for (int k = 0; k <= largchg + 1 && k < R; k++) {
                const uint64_t temp = set_sym(ver[i], k, j);
                if (in_ver_set(temp, point))
                    continue;

                // Local O(1) dedup: skip if this parent already spawned temp.
                uint32_t h = static_cast<uint32_t>(
                    (temp ^ (temp >> 27) ^ (temp >> 13)) & 2047);
                bool duplicate = false;
                while (local_seen[h] != 0xFFFFFFFFFFFFFFFFULL) {
                    if (local_seen[h] == temp) {
                        duplicate = true;
                        break;
                    }
                    h = (h + 1) & 2047;
                }
                if (duplicate) {
                    nodes_pruned_local++;
                    continue;
                }
                local_seen[h] = temp;

                ver[point] = temp;
                ver_sym_mask[point] = sym_mask(temp);

                // Incremental calc: O(R) delta for the newly-added vertex.
                const auto [step_nk1, step_cons] = calc_step(point + 1);
                solve(point + 1, std::max(nodl, j + 1), std::max(largchg, k),
                      acc_nk1 + step_nk1, acc_cons + step_cons);
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

    // Nauty depth limit: use nauty for all internal levels (up to R-1).
    // This maximizes symmetry pruning and prevents the OOM explosion.
    nauty_limit = R - 1;
    if (argc >= 3)
        nauty_limit = static_cast<int>(std::strtol(argv[2], nullptr, 10));

    t0_global = std::chrono::high_resolution_clock::now();
    t_last_print = t0_global;

    ver.resize(R);
    seen_nauty.resize(R + 1);
    seen_sorted.resize(R + 1);

    ver[0] = make_identity();
    ver[1] = set_sym(ver[0], 0, R);
    ver_sym_mask[0] = sym_mask(ver[0]);
    ver_sym_mask[1] = sym_mask(ver[1]);

    // Compute initial accumulated calc for ver[0] and ver[1].
    const auto [init_nk1, init_cons] = calc_step(2);

    std::cerr << "Searching R=" << R << " (nauty depth limit: " << nauty_limit
              << ")"
              << "  ver[0]=" << vertex_to_string(ver[0])
              << "  ver[1]=" << vertex_to_string(ver[1]) << "\n";

    if (R == 2) {
        solve(2, R + 1, 0, init_nk1, init_cons);
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
                if (contains_sym(ver[i], j))
                    continue;
                for (int k = 0; k <= init_largchg + 1 && k < R; k++) {
                    const uint64_t temp = set_sym(ver[i], k, j);
                    if (in_ver_set(temp, 2))
                        continue;
                    branches.push_back({temp, std::max(init_nodl, j + 1),
                                        std::max(init_largchg, k)});
                }
            }
        }

        const int total = static_cast<int>(branches.size());
        for (int b = 0; b < total; b++) {
            ver[2] = branches[b].temp;
            ver_sym_mask[2] = sym_mask(ver[2]);
            const auto [step_nk1, step_cons] = calc_step(3);
            solve(3, branches[b].nodl, branches[b].largchg, init_nk1 + step_nk1,
                  init_cons + step_cons);

            const double elapsed =
                std::chrono::duration<double>(
                    std::chrono::high_resolution_clock::now() - t0_global)
                    .count();
            std::cerr << "\r" << std::string(120, ' ') << "\r  " << (b + 1)
                      << "/" << total << " branches"
                      << " | " << nodes_evaluated << " evaluated"
                      << " | " << elapsed << "s\n"
                      << std::flush;
        }
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const double elapsed =
        std::chrono::duration<double>(t1 - t0_global).count();
    // Clear telemetry line
    std::cerr << "\r" << std::string(100, ' ') << "\r";

    // Compute column widths for aligned output.
    int max_nk1_w = 0, max_nk_w = 0;
    for (const auto &[nk1, res] : results) {
        max_nk1_w =
            std::max(max_nk1_w, static_cast<int>(std::to_string(nk1).size()));
        max_nk_w = std::max(
            max_nk_w, static_cast<int>(std::to_string(nk1 + res.cons).size()));
    }

    // Determine concrete n for brute-force verification.
    int max_sym_global = 0;
    for (const auto &[nk1, res] : results) {
        std::istringstream iss(res.example);
        std::string tok;
        while (iss >> tok) {
            for (char c : tok) {
                int s = c >= 'a' ? (c - 'a' + 26) : (c - 'A');
                max_sym_global = std::max(max_sym_global, s);
            }
        }
    }
    const int ver_n = std::max(2 * R, max_sym_global + 1);
    const int ver_k = R;

    // Print results with inline brute-force verification.
    bool all_ok = true;
    for (const auto &[nk1, res] : results) {
        std::cout << "(" << R << "nk-" << std::setw(max_nk1_w) << nk1
                  << ") (n-k)-" << std::setw(max_nk_w) << (nk1 + res.cons)
                  << ", EX: " << res.example << "\n";

        // Parse example back into ver[] and vset[]
        uint64_t vset[16];
        {
            std::istringstream iss(res.example);
            std::string tok;
            for (int i = 0; i < R && (iss >> tok); i++) {
                uint64_t v = 0;
                for (int p = 0; p < R; p++)
                    v = set_sym(v, p,
                                tok[p] >= 'a' ? (tok[p] - 'a' + 26)
                                              : (tok[p] - 'A'));
                ver[i] = v;
                vset[i] = v;
            }
        }

        // Oracle cross-check (independent algorithm)
        const auto [vnk1, vcons] = verify_neighbor_set();
        if (nk1 != vnk1) {
            std::cout << "  VERIFY FAIL: nk1 mismatch: calc=" << nk1
                      << " verify=" << vnk1 << "\n";
            all_ok = false;
            continue;
        }

        // Brute-force neighbor enumeration in A(ver_n, ver_k)
        std::vector<uint64_t> neighbors;
        for (int i = 0; i < R; i++) {
            for (int p = 0; p < ver_k; p++) {
                for (int s = 0; s < ver_n; s++) {
                    if (contains_sym(vset[i], s))
                        continue;
                    uint64_t nbr = set_sym(vset[i], p, s);
                    bool in_vprime = false;
                    for (int q = 0; q < R; q++) {
                        if (vset[q] == nbr) {
                            in_vprime = true;
                            break;
                        }
                    }
                    if (!in_vprime)
                        neighbors.push_back(nbr);
                }
            }
        }
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()),
                        neighbors.end());
        const int brute_count = static_cast<int>(neighbors.size());

        const int coeff = R * ver_k - nk1;
        const int constant = nk1 + res.cons;
        const int formula_val = coeff * (ver_n - ver_k) - constant;

        std::cout << "  verify(n=" << ver_n << ",k=" << ver_k
                  << "): |N(V')| = (" << R << "\xc2\xb7" << ver_k << "-" << nk1
                  << ")(" << ver_n << "-" << ver_k << ")-" << constant << " = "
                  << coeff << "\xc2\xb7" << (ver_n - ver_k) << " - " << constant
                  << " = " << formula_val << "\n";
        std::cout << "  brute-force neighbor count: " << brute_count;
        if (brute_count == formula_val) {
            std::cout << " \xe2\x9c\x93\n";
        } else {
            std::cout << " \xe2\x9c\x97 MISMATCH!\n";
            all_ok = false;
        }
    }

    std::cerr << "Done: " << std::fixed << std::setprecision(3) << elapsed
              << "s, " << nodes_generated << " generated, " << nodes_evaluated
              << " evaluated, " << nodes_pruned_iso << " iso-pruned, "
              << nodes_pruned_exact << " exact-pruned, " << nodes_pruned_local
              << " local-pruned\n";

    if (all_ok && !results.empty()) {
        std::cerr << "\xe2\x9c\x93 Verified.\n";
    } else if (!all_ok) {
        std::cerr << "\xe2\x9c\x97 Verification failed.\n";
    }

    return 0;
}
