// Hamming Ball Predictor for Arrangement Graph Extraconnectivity
//
// Three-tier prediction:
//   1. O(R)    — analytical: A000788 coefficient + cumulative-zeros constant
//   2. O(R³)   — construction: Hamming ball + formula computation
//   3. O(R³logR)— verification: brute-force neighbor enumeration (R ≤ 40)
//
// Usage: ./predict [R]       Single R prediction (R ≤ 64)
//        ./predict --csv N   CSV output for R=2..N
//
// Vertex representation: stack-allocated SymT[MaxK] with memcmp ordering.
// SymT = uint8_t when R ≤ 127, uint16_t for R ≥ 128.
// Zero heap allocation in the hot path enables instant verification.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// __int128 is a GCC/Clang extension, not ISO C++17.
// __extension__ suppresses -Wpedantic for this type.
__extension__ typedef __int128 int128_t;
static inline int128_t widen(int64_t x) { return x; }

static int R = 10;

// ── Vertex type: fixed-size stack struct, templatized on symbol type ──

template <typename SymT> struct Vertex {
    static constexpr SymT SENTINEL = static_cast<SymT>(~SymT{0}); // max value
    SymT syms[256] = {};
    bool operator<(const Vertex &o) const {
        return std::memcmp(syms, o.syms, R * sizeof(SymT)) < 0;
    }
    bool operator==(const Vertex &o) const {
        return std::memcmp(syms, o.syms, R * sizeof(SymT)) == 0;
    }
};

template <typename SymT>
static bool contains_sym(const Vertex<SymT> &v, int sym) {
    for (int i = 0; i < R; i++)
        if (v.syms[i] == static_cast<SymT>(sym))
            return true;
    return false;
}

template <typename SymT>
static std::string vertex_to_string(const Vertex<SymT> &v) {
    std::string str(R, ' ');
    for (int i = 0; i < R; i++) {
        int s = static_cast<int>(v.syms[i]);
        if (s < 26)
            str[i] = static_cast<char>('A' + s);
        else if (s < 52)
            str[i] = static_cast<char>('a' + s - 26);
        else
            str[i] = '?'; // for symbols beyond a-z range, use placeholder
    }
    return str;
}

// ── 128-bit integer printing (for large R where coeff*R > 2^63) ──────
static std::string i128_to_string(int128_t x) {
    if (x == 0)
        return "0";
    bool neg = x < 0;
    if (neg)
        x = -x;
    std::string s;
    while (x > 0) {
        s += static_cast<char>('0' + static_cast<int>(x % 10));
        x /= 10;
    }
    if (neg)
        s += '-';
    std::reverse(s.begin(), s.end());
    return s;
}

// ── A000788: cumulative popcount — O(log R) ──────────────────────────

static uint64_t popcount_u(uint64_t n) {
    return static_cast<uint64_t>(__builtin_popcountll(n));
}

static uint64_t bit_length_u(uint64_t n) {
    return n == 0 ? 0 : 64 - static_cast<uint64_t>(__builtin_clzll(n));
}

static int64_t A000788(int64_t n) {
    if (n <= 0)
        return 0;
    int64_t m = n / 2;
    if (n % 2 == 0)
        return 2 * A000788(m) + m;
    else
        return 2 * A000788(m) + m +
               static_cast<int64_t>(popcount_u(static_cast<uint64_t>(m)));
}

// ── Constant C(R) — O(R) ─────────────────────────────────────────────
// C(R) = (R-1) + Σ_{x=1}^{R-1} bit_length(x) - E(R)
// Equivalently: (R-1) + Σ zero-bits in binary(1..R-1)

static int64_t constant_analytical(int64_t R_val) {
    int64_t nk1 = A000788(R_val);
    int64_t L = 0;
    for (int64_t x = 1; x < R_val; x++)
        L += static_cast<int64_t>(bit_length_u(static_cast<uint64_t>(x)));
    return (R_val - 1) + L - nk1;
}

// ── Hamming ball construction ────────────────────────────────────────

template <typename SymT> static std::vector<Vertex<SymT>> build_hamming_ball() {
    int dims = 0;
    while ((1 << dims) < R)
        dims++;

    std::vector<Vertex<SymT>> verts(R);
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++)
            verts[i].syms[p] = static_cast<SymT>(p);
        for (int d = 0; d < dims; d++) {
            if (i & (1 << d))
                verts[i].syms[d] = static_cast<SymT>(R + d);
        }
    }
    return verts;
}

// ── Formula computation via construction — O(R³) ─────────────────────

struct FormulaResult {
    int64_t nk1;
    int64_t constant;
};

template <typename SymT>
static FormulaResult compute_formula(const std::vector<Vertex<SymT>> &verts) {
    // Collect used symbols
    std::vector<SymT> used_syms;
    for (const auto &v : verts) {
        for (int p = 0; p < R; p++) {
            SymT s = v.syms[p];
            if (!std::any_of(used_syms.begin(), used_syms.end(),
                             [s](SymT u) { return u == s; }))
                used_syms.push_back(s);
        }
    }
    const int M = static_cast<int>(used_syms.size());

    // Anonymous coefficient: distinct groups per position
    int anon_coeff = 0;
    for (int p = 0; p < R; p++) {
        std::vector<Vertex<SymT>> group_keys;
        for (int i = 0; i < R; i++) {
            Vertex<SymT> key = verts[i];
            key.syms[p] = Vertex<SymT>::SENTINEL;
            if (!std::any_of(
                    group_keys.begin(), group_keys.end(),
                    [&key](const Vertex<SymT> &g) { return g == key; }))
                group_keys.push_back(key);
        }
        anon_coeff += static_cast<int>(group_keys.size());
    }

    // Named neighbors (sort-based dedup, zero heap alloc in hot path)
    std::vector<Vertex<SymT>> sorted_verts = verts;
    std::sort(sorted_verts.begin(), sorted_verts.end());

    std::vector<Vertex<SymT>> named_nbrs;
    named_nbrs.reserve(R * R * M);
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            for (auto s : used_syms) {
                if (!contains_sym(verts[i], s)) {
                    Vertex<SymT> vtx = verts[i];
                    vtx.syms[p] = s;
                    if (!std::binary_search(sorted_verts.begin(),
                                            sorted_verts.end(), vtx))
                        named_nbrs.push_back(vtx);
                }
            }
        }
    }
    std::sort(named_nbrs.begin(), named_nbrs.end());
    named_nbrs.erase(std::unique(named_nbrs.begin(), named_nbrs.end()),
                     named_nbrs.end());

    const int64_t nk1 = R * R - anon_coeff;
    const int64_t named_sz = static_cast<int64_t>(named_nbrs.size());
    const int64_t constant =
        static_cast<int64_t>(anon_coeff) * (M - R) - named_sz;

    return {nk1, constant};
}

// ── Brute-force verification — O(R³ log R) ───────────────────────────

template <typename SymT>
static int64_t brute_force_neighbors(const std::vector<Vertex<SymT>> &verts,
                                     int n, int k) {
    std::vector<Vertex<SymT>> sorted_verts = verts;
    std::sort(sorted_verts.begin(), sorted_verts.end());

    std::vector<Vertex<SymT>> nbrs;
    nbrs.reserve(R * k * n);
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < k; p++) {
            for (int s = 0; s < n; s++) {
                if (contains_sym(verts[i], s))
                    continue;
                Vertex<SymT> nbr = verts[i];
                nbr.syms[p] = static_cast<SymT>(s);
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

// ── Verify runner — templated on symbol type ─────────────────────────

template <typename SymT>
static int run_verify(int64_t expected_nk1, int64_t expected_const,
                      bool quiet = false) {
    auto verts = build_hamming_ball<SymT>();

    if (!quiet && R <= 12) {
        std::cerr << "  vertex set:";
        for (int i = 0; i < R; i++)
            std::cerr << " " << vertex_to_string(verts[i]);
        std::cerr << "\n";
    }

    auto [nk1, constant] = compute_formula(verts);

    if (nk1 != expected_nk1) {
        std::cerr << "nk1 MISMATCH: got " << nk1 << " expected " << expected_nk1
                  << "\n";
        return 1;
    }
    if (constant != expected_const) {
        std::cerr << "constant MISMATCH: got " << constant << " expected "
                  << expected_const << "\n";
        return 1;
    }

    // ── Tier 3: Brute-force verification — O(R³ log R) ───────────
    const int ver_n = 2 * R;
    const int64_t brute_count = brute_force_neighbors(verts, ver_n, R);
    const int128_t coeff = widen(R) * R - nk1;
    const int128_t formula_val = coeff * R - constant;

    if (brute_count != formula_val) {
        std::cerr << "|N(V')| MISMATCH: brute=" << brute_count
                  << " formula=" << i128_to_string(formula_val) << "\n";
        return 1;
    }

    if (quiet) {
        std::cerr << "✓\n";
    } else {
        std::cerr << "  [construction] nk1 = " << nk1 << " ✓\n";
        std::cerr << "  [construction] constant = " << constant << " ✓\n";
        std::cerr << "  [brute-force] |N(V')| = " << brute_count << " ✓\n";
    }
    return 0;
}

// ── Main ──────────────────────────────────────────────────────────────

int main(int argc, const char *argv[]) {
    // CSV mode: ./predict --csv N
    if (argc >= 3 && std::string(argv[1]) == "--csv") {
        int max_r = static_cast<int>(std::strtol(argv[2], nullptr, 10));
        if (max_r < 2)
            max_r = 2;
        std::cout << "R,nk1,constant,coeff,formula_at_2R\n";
        for (int r = 2; r <= max_r; r++) {
            int64_t nk1 = A000788(r);
            int64_t c = constant_analytical(r);
            int128_t coeff = widen(r) * r - nk1;
            int128_t val = coeff * r - c;
            std::cout << r << "," << nk1 << "," << c << ","
                      << i128_to_string(coeff) << "," << i128_to_string(val)
                      << "\n";
        }
        return 0;
    }

    // --verify-range mode: sweep R=start..end with brute-force cross-check
    // Outputs a deterministic FNV-1a digest as proof-of-work.
    if (argc >= 3 && std::string(argv[1]) == "--verify-range") {
        int start_r = 2, end_r = 0;
        if (argc >= 4) {
            start_r = static_cast<int>(std::strtol(argv[2], nullptr, 10));
            end_r = static_cast<int>(std::strtol(argv[3], nullptr, 10));
        } else {
            end_r = static_cast<int>(std::strtol(argv[2], nullptr, 10));
        }
        if (start_r < 2 || end_r < start_r || end_r > 255) {
            std::cerr
                << "Error: --verify-range requires 2 <= start <= end <= 255\n";
            return 1;
        }

        // FNV-1a 64-bit hash for deterministic digest
        uint64_t hash = UINT64_C(14695981039346656037);
        auto fnv_feed = [&hash](int64_t val) {
            for (int i = 0; i < 8; i++) {
                hash ^= static_cast<uint64_t>(val & 0xFF);
                hash *= UINT64_C(1099511628211);
                val >>= 8;
            }
        };

        for (int r = start_r; r <= end_r; r++) {
            R = r;
            const int64_t nk1 = A000788(R);
            const int64_t cst = constant_analytical(R);
            // Feed verified values into running digest
            fnv_feed(R);
            fnv_feed(nk1);
            fnv_feed(cst);
            char hex[17];
            std::snprintf(hex, sizeof(hex), "%016llx",
                          static_cast<unsigned long long>(hash));
            std::cerr << "R=" << R << " ... ";
            int rc;
            if (R <= 127)
                rc = run_verify<uint8_t>(nk1, cst, /*quiet=*/true);
            else
                rc = run_verify<uint16_t>(nk1, cst, /*quiet=*/true);
            if (rc != 0) {
                std::cerr << "FAILED at R=" << R << "\n";
                return 1;
            }
            std::cout << "R=" << R << " nk1=" << nk1 << " C=" << cst
                      << " digest=" << hex << "\n";
        }

        // Output reproducible digest
        char hex[17];
        std::snprintf(hex, sizeof(hex), "%016llx",
                      static_cast<unsigned long long>(hash));
        std::cerr << "All R=" << start_r << ".." << end_r
                  << " verified ✓  digest=" << hex << "\n";
        std::cout << "VERIFIED R=" << start_r << ".." << end_r
                  << " digest=" << hex << "\n";
        return 0;
    }

    // --verify mode: explicit brute-force O(R³) cross-check
    bool verify_mode = false;
    if (argc >= 3 && std::string(argv[1]) == "--verify") {
        verify_mode = true;
        R = static_cast<int>(std::strtol(argv[2], nullptr, 10));
        if (R < 2 || R > 255) {
            std::cerr << "Error: --verify requires 2 <= R <= 255\n";
            return 1;
        }
    } else if (argc >= 2) {
        R = static_cast<int>(std::strtol(argv[1], nullptr, 10));
        if (R < 2) {
            std::cerr << "Usage:\n"
                      << "  ./predict <R>                     O(log R) "
                         "analytical formula\n"
                      << "  ./predict --verify <R>            O(R³) "
                         "brute-force cross-check\n"
                      << "  ./predict --verify-range [s] <e>  Sweep R=s..e "
                         "(default s=2)\n"
                      << "  ./predict --csv <max_R>           CSV table for "
                         "R=2..max_R\n";
            return 1;
        }
    }

    // ── Tier 1: Analytical — O(log R) ─────────────────────────────────
    const int64_t expected_nk1 = A000788(R);
    const int64_t expected_const = constant_analytical(R);

    std::cerr << "Hamming ball prediction for R=" << R << "\n";
    std::cerr << "  [analytical] nk1 = A000788(" << R << ") = " << expected_nk1
              << "\n";
    std::cerr << "  [analytical] constant = " << expected_const << "\n";

    // ── Tier 2+3: Construction + Brute-force — O(R³) ──────────────────
    if (verify_mode) {
        int rc;
        if (R <= 127) {
            // uint8_t path: symbols fit in [0, R+dims) < 128+7 = 135 < 256
            std::cerr << "  [type] uint8_t symbols\n";
            rc = run_verify<uint8_t>(expected_nk1, expected_const);
        } else {
            // uint16_t path: symbols >= 128
            std::cerr << "  [type] uint16_t symbols\n";
            rc = run_verify<uint16_t>(expected_nk1, expected_const);
        }
        if (rc != 0)
            return rc;
    }

    // ── Output ────────────────────────────────────────────────────────
    const int128_t coeff = widen(R) * R - expected_nk1;
    const int128_t formula_val = coeff * R - expected_const;

    std::cout << "(" << R << "nk-" << expected_nk1 << ") (n-k)-"
              << expected_const << ", EX:";
    if (R <= 12) {
        auto verts = build_hamming_ball<uint8_t>();
        for (int i = 0; i < R; i++)
            std::cout << " " << vertex_to_string(verts[i]);
    } else {
        std::cout << " [" << R << " vertices]";
    }
    std::cout << "\n";

    std::cerr << "  formula(n=" << 2 * R << ",k=" << R
              << "): |N(V')| = " << i128_to_string(coeff) << "\xc2\xb7" << R
              << " - " << expected_const << " = " << i128_to_string(formula_val)
              << "\n";

    return 0;
}
