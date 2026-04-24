// Extraconnectivity of Arrangement Graphs — v5 Clean Enumerator (POC)
//
// Based on ideas-12: streamlined single-threaded architecture with:
//   1. Hardware-accelerated chunk_idx SWAR bit-scan (no O(R) diff loops)
//   2. Implicit diff1 < diff2 from ctzll ordering (no branch swap)
//   3. isSharedMask bitmask (no array zeroing)
//   4. Parameterized calc_step (explicit array passing)
//   5. Global hash dedup sharing symmetry across all branches
//
// Usage: ./arrangementv5 [R] [nauty_limit]

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <nauty/nauty.h>
}

static int R = 5;
static int global_nauty_limit = 5;

// ── Vertex representation ──────────────────────────────────────────────────
static inline int get_sym(uint64_t vertex, int pos) {
    return static_cast<int>((vertex >> ((R - 1 - pos) * 5)) & 0x1FU);
}

static inline uint64_t set_sym(uint64_t vertex, int pos, int sym) {
    const int shift = (R - 1 - pos) * 5;
    return (vertex & ~(0x1FULL << shift)) |
           (static_cast<uint64_t>(sym) << shift);
}

static inline bool contains_sym(uint64_t vertex, int sym) {
    for (int i = 0; i < R; i++)
        if (get_sym(vertex, i) == sym)
            return true;
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

static inline bool in_ver_set(uint64_t v, int point, const uint64_t *arr) {
    for (int i = 0; i < point; i++)
        if (arr[i] == v)
            return true;
    return false;
}

// ── 128-bit hash fingerprint ───────────────────────────────────────────────
static inline uint64_t splitmix64(uint64_t z) {
    z ^= (z >> 30);
    z *= 0xbf58476d1ce4e5b9ULL;
    z ^= (z >> 27);
    z *= 0x94d049bb133111ebULL;
    z ^= (z >> 31);
    return z;
}

namespace {

struct Hash128 {
    uint64_t h1, h2;
    bool operator==(const Hash128 &o) const { return h1 == o.h1 && h2 == o.h2; }
};

static inline Hash128 hash_nauty_graph(const graph *cg, int m_aux, int n_aux) {
    uint64_t h1 = 0x123456789ABCDEF0ULL, h2 = 0x0FEDCBA987654321ULL;
    const size_t num_words = static_cast<size_t>(m_aux) * n_aux;
    for (size_t i = 0; i < num_words; i++) {
        uint64_t w = static_cast<uint64_t>(cg[i]) ^ (i * 0x9E3779B97F4A7C15ULL);
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

// ── Open-addressing Flat Hash Set ──────────────────────────────────────────
class FlatHashSet128 {
    std::vector<Hash128> data_;
    size_t count_ = 0, mask_;

  public:
    explicit FlatHashSet128(size_t capacity_pow2 = 1U << 16)
        : data_(capacity_pow2, {0, 0}), mask_(capacity_pow2 - 1) {}
    bool insert(Hash128 key) {
        if (key.h1 == 0 && key.h2 == 0)
            key.h1 = 1;
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
    size_t size() const { return count_; }

  private:
    void rehash() {
        std::vector<Hash128> old = std::move(data_);
        data_.assign(old.size() * 2, {0, 0});
        mask_ = data_.size() - 1;
        count_ = 0;
        for (const auto &k : old)
            if (k.h1 != 0 || k.h2 != 0)
                insert(k);
    }
};

} // anonymous namespace

static std::vector<FlatHashSet128> seen_nauty;
static std::vector<FlatHashSet128> seen_sorted;

// ── Nauty static buffers ───────────────────────────────────────────────────
constexpr int MAX_NAUTY_N = 512;
constexpr int MAX_NAUTY_M = 16;
static graph g_nauty[MAX_NAUTY_N * MAX_NAUTY_M];
static graph cg_nauty[MAX_NAUTY_N * MAX_NAUTY_M];
static int lab_nauty[MAX_NAUTY_N], ptn_nauty[MAX_NAUTY_N],
    orbits_nauty[MAX_NAUTY_N];

// ── Global state ───────────────────────────────────────────────────────────
static uint64_t ver[16];
static uint32_t ver_sym_mask[16];

struct Result {
    int cons = 0;
    int internal_edges = 0;
    std::string example;
};
static std::map<int, Result> results;

static uint64_t nodes_generated = 0, nodes_evaluated = 0;
static uint64_t nodes_pruned_iso = 0, nodes_pruned_exact = 0,
                nodes_pruned_local = 0;
static std::chrono::high_resolution_clock::time_point t0_global, t_last_print;

// ── chunk_idx SWAR lookup ──────────────────────────────────────────────────
// Bit position → 5-bit chunk index (avoids division by 5)
static constexpr int chunk_idx[64] = {
    0, 0, 0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,
    3, 3, 3,  3,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,
    6, 6, 6,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  9,  9,  9,
    9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12,
};

// ── O(1) SWAR-Accelerated Incremental Calculation ──────────────────────────
static inline std::pair<int, int> calc_step(int count) {
    const int idx = count - 1;
    const uint64_t cur = ver[idx];
    int step_nk1 = 0, dc_count = 0, isSharednum = 0;
    uint32_t isSharedMask = 0;
    uint64_t dcverts[256];
    int chgs[256];

    for (int j = 0; j < idx; j++) {
        const uint64_t cur2 = ver[j];

        // SWAR bit-scan: instantly isolate differing 5-bit nibbles
        uint64_t xor_val = cur ^ cur2;
        if (xor_val == 0)
            continue;

        int differs = 0, diff1 = 0, diff2 = 0;

        while (xor_val) {
            int bit = __builtin_ctzll(xor_val);
            int chunk = chunk_idx[bit];
            int pos = (R - 1) - chunk;

            // ctzll finds lowest bits first → highest pos first.
            // Assign diff2 first (high pos), diff1 second (low pos).
            // This guarantees diff1 < diff2 without any swap.
            if (differs == 0)
                diff2 = pos;
            else
                diff1 = pos;

            differs++;
            if (differs > 2)
                break;

            xor_val &= ~(0x1FULL << (chunk * 5));
        }

        if (differs > 2)
            continue;

        if (differs == 1) {
            if (!(isSharedMask & (1U << diff2))) {
                isSharedMask |= (1U << diff2);
                isSharednum++;
                step_nk1++;
            }
        } else {
            // diff1 < diff2 guaranteed by ctzll ordering
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
        if (isSharedMask & (1U << chgs[n])) {
            prune = true;
        } else {
            const int pos = chgs[n], ch = get_sym(dcverts[n], pos);
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

// ── Internal edge count ────────────────────────────────────────────────────
// Count edges within the subset: pairs differing in exactly 1 position.
static int count_internal_edges(const uint64_t *verts, int n) {
    int edges = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            uint64_t xv = verts[i] ^ verts[j];
            int diffs = 0;
            while (xv && diffs <= 1) {
                int chunk = chunk_idx[__builtin_ctzll(xv)];
                xv &= ~(0x1FULL << (chunk * 5));
                diffs++;
            }
            if (diffs == 1)
                edges++;
        }
    }
    return edges;
}

// ── Recursive search ───────────────────────────────────────────────────────
static void solve(int point, int nodl, int largchg, uint32_t overall_sym_mask,
                  int current_nk1, int current_cons) {
    nodes_generated++;

    // Telemetry
    if ((nodes_generated & 0x3FFFF) == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(now - t_last_print).count() >= 0.5) {
            double total_time =
                std::chrono::duration<double>(now - t0_global).count();
            std::cerr << "\r  [" << std::fixed << std::setprecision(1)
                      << total_time << "s] gen: " << nodes_generated
                      << " | eval: " << nodes_evaluated
                      << " | pruned: " << nodes_pruned_iso << " iso / "
                      << nodes_pruned_exact << " exact" << std::flush;
            t_last_print = now;
        }
    }

    // Leaf
    if (point == R) {
        nodes_evaluated++;
        auto it = results.find(current_nk1);
        if (it == results.end() || it->second.cons < current_cons) {
            std::string exa;
            for (int i = 0; i < R; i++) {
                if (i)
                    exa += ' ';
                exa += vertex_to_string(ver[i]);
            }
            results[current_nk1] = {current_cons, count_internal_edges(ver, R),
                                    exa};
        }
        return;
    }

    // Deduplication
    if (point <= global_nauty_limit) {
        int sym_map[32] = {0}, N = 0;
        uint32_t m = overall_sym_mask;
        while (m) {
            sym_map[__builtin_ctz(m)] = N++;
            m &= m - 1;
        }

        const int n_aux = R + N + R * N + point;
        const int m_aux = SETWORDSNEEDED(n_aux);
        EMPTYGRAPH(g_nauty, m_aux, n_aux);

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
                ADDONEEDGE(g_nauty, perm_idx, R + N + p * N + s, m_aux);
            }
        }

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

        static DEFAULTOPTIONS_GRAPH(options);
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
    } else if (point < R) {
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

    // Generate candidates
    uint64_t local_seen[2048];
    std::memset(local_seen, 0xFF, sizeof(local_seen));

    for (int i = 0; i < point; i++) {
        const uint32_t mask_i = ver_sym_mask[i];
        for (int j = 0; j <= nodl; j++) {
            if (mask_i & (1U << j))
                continue;
            for (int k = 0; k <= largchg + 1 && k < R; k++) {
                const uint64_t temp = set_sym(ver[i], k, j);
                if (in_ver_set(temp, point, ver))
                    continue;

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

                auto [step_nk1, step_cons] = calc_step(point + 1);
                solve(point + 1, std::max(nodl, j + 1), std::max(largchg, k),
                      overall_sym_mask | ver_sym_mask[point],
                      current_nk1 + step_nk1, current_cons + step_cons);
            }
        }
    }
}

// ── A000788: cumulative popcount — O(log R) ──────────────────────────
static uint64_t popcount_u(uint64_t n) {
    return static_cast<uint64_t>(__builtin_popcountll(n));
}

static uint64_t bit_length_u(uint64_t n) {
    return n == 0 ? 0 : 64 - static_cast<uint64_t>(__builtin_clzll(n));
}

static int64_t A000788_fn(int64_t n) {
    if (n <= 0)
        return 0;
    int64_t m = n / 2;
    if (n % 2 == 0)
        return 2 * A000788_fn(m) + m;
    else
        return 2 * A000788_fn(m) + m +
               static_cast<int64_t>(popcount_u(static_cast<uint64_t>(m)));
}

static int64_t constant_analytical(int64_t R_val) {
    int64_t nk1 = A000788_fn(R_val);
    int64_t L = 0;
    for (int64_t x = 1; x < R_val; x++)
        L += static_cast<int64_t>(bit_length_u(static_cast<uint64_t>(x)));
    return (R_val - 1) + L - nk1;
}

// ── Brute-force verification — O(R³ log R) ───────────────────────────
static int64_t count_neighbors(const uint64_t *verts, int n, int k) {
    std::vector<uint64_t> sorted_verts(verts, verts + R);
    std::sort(sorted_verts.begin(), sorted_verts.end());

    std::vector<uint64_t> nbrs;
    nbrs.reserve(R * k * n);
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < k; p++) {
            for (int s = 0; s < n; s++) {
                if (contains_sym(verts[i], s))
                    continue;
                uint64_t nbr = set_sym(verts[i], p, s);
                if (!std::binary_search(sorted_verts.begin(),
                                        sorted_verts.end(), nbr))
                    nbrs.push_back(nbr);
            }
        }
    }
    std::sort(nbrs.begin(), nbrs.end());
    nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
    return static_cast<int64_t>(nbrs.size());
}

// ── Main ───────────────────────────────────────────────────────────────────
int main(int argc, const char *argv[]) {
    nauty_check(WORDSIZE, MAX_NAUTY_M, MAX_NAUTY_N, NAUTYVERSIONID);
    if (argc >= 2)
        R = static_cast<int>(std::strtol(argv[1], nullptr, 10));

    global_nauty_limit = std::max(3, R - 2);
    if (argc >= 3)
        global_nauty_limit =
            static_cast<int>(std::strtol(argv[2], nullptr, 10));

    seen_nauty.resize(R + 1);
    seen_sorted.resize(R + 1);

    t0_global = std::chrono::high_resolution_clock::now();
    t_last_print = t0_global;

    ver[0] = make_identity();
    ver[1] = set_sym(ver[0], 0, R);
    ver_sym_mask[0] = sym_mask(ver[0]);
    ver_sym_mask[1] = sym_mask(ver[1]);

    std::cerr << "Searching R=" << R << " (nauty limit: " << global_nauty_limit
              << ")\n"
              << "  ver[0]=" << vertex_to_string(ver[0])
              << "  ver[1]=" << vertex_to_string(ver[1]) << "\n";

    auto [init_nk1, init_cons] = calc_step(2);
    solve(2, R + 1, 0, ver_sym_mask[0] | ver_sym_mask[1], init_nk1, init_cons);

    const double elapsed =
        std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - t0_global)
            .count();
    std::cerr << "\r" << std::string(120, ' ') << "\r";

    int max_nk1_w = 0, max_nk_w = 0;
    for (const auto &[nk1, res] : results) {
        max_nk1_w =
            std::max(max_nk1_w, static_cast<int>(std::to_string(nk1).size()));
        max_nk_w = std::max(
            max_nk_w, static_cast<int>(std::to_string(nk1 + res.cons).size()));
    }

    int best_nk1 = -1;
    for (const auto &[nk1, res] : results) {
        std::cout << "(" << R << "nk-" << std::setw(max_nk1_w) << nk1
                  << ") (n-k)-" << std::setw(max_nk_w) << (nk1 + res.cons)
                  << ", iedges=" << res.internal_edges
                  << ", EX: " << res.example << "\n";
        best_nk1 = nk1;
    }

    std::cerr << "Done: " << std::fixed << std::setprecision(3) << elapsed
              << "s | Gen: " << nodes_generated
              << " | Eval: " << nodes_evaluated
              << "\nPruned | Iso: " << nodes_pruned_iso
              << " | Exact: " << nodes_pruned_exact
              << " | Local: " << nodes_pruned_local << "\n";

    if (best_nk1 != -1) {
        // Parse example back into array
        std::string ex = results[best_nk1].example;
        uint64_t best_verts[16] = {0};
        size_t pos = 0;
        for (int i = 0; i < R; i++) {
            std::string vstr = ex.substr(pos, R);
            uint64_t v = 0;
            for (int p = 0; p < R; p++) {
                int sym;
                if (vstr[p] >= 'A' && vstr[p] <= 'Z')
                    sym = vstr[p] - 'A';
                else
                    sym = vstr[p] - 'a' + 26;
                v = set_sym(v, p, sym);
            }
            best_verts[i] = v;
            pos += R + 1;
        }

        int64_t brute_count = count_neighbors(best_verts, 2 * R, R);
        int64_t theory_nk1 = A000788_fn(R);
        int64_t theory_const = constant_analytical(R);
        int64_t coeff = (int64_t)R * R - theory_nk1;
        int64_t theory_val = coeff * R - theory_const;

        std::cerr << "  [brute-force] |N(V')| = " << brute_count;
        if (brute_count == theory_val) {
            std::cerr << " \xe2\x9c\x93\n";
        } else {
            std::cerr << " \xe2\x9c\x97 MISMATCH (theory gives " << theory_val
                      << ")\n";
        }

        std::cerr << "  formula(n=" << 2 * R << ",k=" << R
                  << "): |N(V')| = " << coeff << "\xc2\xb7" << R << " - "
                  << theory_const << " = " << theory_val << "\n";
    }

    return 0;
}
