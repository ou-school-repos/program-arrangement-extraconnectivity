// Hamming Ball Predictor for Arrangement Graph Extraconnectivity
//
// Three-tier prediction:
//   1. O(R)         — analytical: A000788 coefficient + cumulative-zeros
//   constant
//   2. O(R^3)       — construction: Hamming ball + formula computation
//   3. O(R^3 log R) — verification: brute-force neighbor enumeration (R ≤ 260)
//
// Note on the R ≤ 260 verification cap: brute_force_neighbors's final
// deduplicated neighbor count is itself Theta(R^3) (see
// |N(V')| = coeff*R - constant), and each Vertex<K,SymT> costs a fixed
// K*sizeof(SymT) bytes regardless of the actual R being verified — so peak
// memory is Theta(R^3 * K), not just Theta(R^3). K is chosen per tier as the
// smallest power-of-two-ish bound at least R, EXCEPT that reusing a single
// K=512 tier for the whole 257..512 range (as this file previously did)
// makes R=260 pay for K=512 (~18 GiB) when a right-sized K=260 tier needs
// only ~9 GiB. R=512 itself is a different story regardless of tier sizing:
// at K=512 that is upwards of 137 GiB, which exhausts typical machines
// (16-32 GiB RAM) well before sorting/dedup can even run, and no reserve()/
// streaming trick fixes it — the *final* answer set is that large. So 260
// (matching the range historically verified for this project; see
// docs/verifications.csv and paper.tex) is the enforced ceiling for
// --verify / --verify-range, served by a right-sized K=260 tier rather than
// the old oversized K=512 one.
//
// Usage: ./predict [R]       Single R prediction
//        ./predict --csv N   CSV output for R=2..N
//
// Vertex representation: stack-allocated SymT[MaxK] with memcmp ordering.
// SymT = uint8_t when R ≤ 127, uint16_t for R ≥ 128.
// Vertex storage is inline; verify still allocates large neighbor vectors.

#include <algorithm>
#include <charconv>
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

#include <array>

// -- Vertex type: dynamic-size stack struct, templatized on size and symbol
// type --

template <int K, typename SymT> struct Vertex {
    static constexpr SymT SENTINEL = static_cast<SymT>(~SymT{0}); // max value
    std::array<SymT, K> syms = {};
    bool operator<(const Vertex &o) const { return syms < o.syms; }
    bool operator==(const Vertex &o) const { return syms == o.syms; }
};

template <int K, typename SymT>
static bool contains_sym(const Vertex<K, SymT> &v, int sym, int width) {
    for (int i = 0; i < width; i++)
        if (v.syms[i] == static_cast<SymT>(sym))
            return true;
    return false;
}

template <int K, typename SymT>
static std::string vertex_to_string(const Vertex<K, SymT> &v, int width) {
    std::string str(width, ' ');
    for (int i = 0; i < width; i++) {
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

// -- 128-bit integer printing (for large R where coeff*R > 2^63) ----
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

// -- A000788: cumulative popcount — O(log R) ------------------------

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

// -- Constant C(R) — O(R) ---------------------------------------------
// C(R) = (R-1) + Σ_{x=1}^{R-1} bit_length(x) - E(R)
// Equivalently: (R-1) + Σ zero-bits in binary(1..R-1)

static int64_t constant_analytical(int64_t R_val) {
    if (R_val <= 0)
        return 0;
    int64_t nk1 = A000788(R_val);
    int64_t L = 0;
    for (int64_t x = 1; x < R_val; x++)
        L += static_cast<int64_t>(bit_length_u(static_cast<uint64_t>(x)));
    return (R_val - 1) + L - nk1;
}

static bool parse_int_arg(const std::string &arg, int &out) {
    const char *begin = arg.data();
    const char *end = begin + arg.size();
    auto [ptr, ec] = std::from_chars(begin, end, out);
    return ec == std::errc{} && ptr == end;
}

// -- Hamming ball construction ----------------------------------------

template <int K, typename SymT>
static std::vector<Vertex<K, SymT>> build_hamming_ball(int width) {
    if (width > K) {
        std::cerr << "Internal error: width exceeds vertex capacity\n";
        std::abort();
    }
    const int active_width = std::min(width, K);

    int dims = 0;
    while ((1 << dims) < active_width)
        dims++;

    std::vector<Vertex<K, SymT>> verts(active_width);
    for (int i = 0; i < active_width; i++) {
        std::fill(verts[i].syms.begin(), verts[i].syms.end(),
                  Vertex<K, SymT>::SENTINEL);
        for (int p = 0; p < active_width; p++)
            verts[i].syms[p] = static_cast<SymT>(p);
        for (int d = 0; d < dims; d++) {
            if (i & (1 << d))
                verts[i].syms[d] = static_cast<SymT>(active_width + d);
        }
    }
    return verts;
}

// -- Formula computation via construction — O(R^3) --------------------

struct FormulaResult {
    int64_t nk1;
    int64_t constant;
};

template <int K, typename SymT>
static FormulaResult compute_formula(const std::vector<Vertex<K, SymT>> &verts,
                                     int width) {
    // Collect used symbols
    std::vector<SymT> used_syms;
    for (const auto &v : verts) {
        for (int p = 0; p < width; p++) {
            SymT s = v.syms[p];
            if (!std::any_of(used_syms.begin(), used_syms.end(),
                             [s](SymT u) { return u == s; }))
                used_syms.push_back(s);
        }
    }
    const int M = static_cast<int>(used_syms.size());

    // Anonymous coefficient: distinct groups per position
    int anon_coeff = 0;
    for (int p = 0; p < width; p++) {
        std::vector<Vertex<K, SymT>> group_keys;
        for (int i = 0; i < width; i++) {
            Vertex<K, SymT> key = verts[i];
            key.syms[p] = Vertex<K, SymT>::SENTINEL;
            if (!std::any_of(
                    group_keys.begin(), group_keys.end(),
                    [&key](const Vertex<K, SymT> &g) { return g == key; }))
                group_keys.push_back(key);
        }
        anon_coeff += static_cast<int>(group_keys.size());
    }

    // Named neighbors (sort-based dedup, zero heap alloc in hot path)
    std::vector<Vertex<K, SymT>> sorted_verts = verts;
    std::sort(sorted_verts.begin(), sorted_verts.end());

    std::vector<Vertex<K, SymT>> named_nbrs;
    const size_t reserve_hint = static_cast<size_t>(width) *
                                static_cast<size_t>(width) *
                                static_cast<size_t>(M - width);
    named_nbrs.reserve(reserve_hint);
    for (int i = 0; i < width; i++) {
        for (int p = 0; p < width; p++) {
            for (auto s : used_syms) {
                if (!contains_sym(verts[i], s, width)) {
                    Vertex<K, SymT> vtx = verts[i];
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

    const int64_t nk1 = static_cast<int64_t>(width) * width - anon_coeff;
    const int64_t named_sz = static_cast<int64_t>(named_nbrs.size());
    const int64_t constant =
        static_cast<int64_t>(anon_coeff) * (M - width) - named_sz;

    return {nk1, constant};
}

// -- Brute-force verification — O(R^3 * log R) -------------------------

template <int K, typename SymT>
static int64_t brute_force_neighbors(const std::vector<Vertex<K, SymT>> &verts,
                                     int n, int k) {
    std::vector<Vertex<K, SymT>> sorted_verts = verts;
    std::sort(sorted_verts.begin(), sorted_verts.end());

    std::vector<Vertex<K, SymT>> nbrs;
    // Tight upper bound on pre-dedup pushes: for each of the k ball vertices
    // and each of its k positions, exactly (n-k) of the n symbols are absent
    // from that vertex and can trigger a push. size_t arithmetic avoids
    // overflow at supported R; it therefore also bounds post-dedup size,
    // which is asymptotically close to this allocation (see header comment).
    const size_t reserve_hint = static_cast<size_t>(k) *
                                static_cast<size_t>(k) *
                                static_cast<size_t>(n - k);
    nbrs.reserve(reserve_hint);
    Vertex<K, SymT> nbr;
    for (int i = 0; i < k; i++) {
        for (int p = 0; p < k; p++) {
            for (int s = 0; s < n; s++) {
                if (contains_sym(verts[i], s, k))
                    continue;
                nbr.syms = verts[i].syms;
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

// -- Verify runner — templated on symbol type -------------------------

template <int K, typename SymT>
static int run_verify(int R, int64_t expected_nk1, int64_t expected_const,
                      bool quiet = false) {
    auto verts = build_hamming_ball<K, SymT>(R);

    if (!quiet && R <= 12) {
        std::cerr << "  vertex set:";
        for (int i = 0; i < R; i++)
            std::cerr << " " << vertex_to_string(verts[i], R);
        std::cerr << "\n";
    }

    auto [nk1, constant] = compute_formula(verts, R);

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

    // Brute-force neighbor enumeration
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

// -- Main -------------------------------------------------------------

int main(int argc, const char *argv[]) {
    // -- Flag parsing -------------------------------------------------
    bool csv_mode = false;
    bool verify_mode = false;
    bool range_mode = false;
    bool no_header = false;
    int start_r = 2, end_r = 0;
    int R = 10;

    std::vector<std::string> positional;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--csv")
            csv_mode = true;
        else if (arg == "--verify")
            verify_mode = true;
        else if (arg == "--verify-range")
            range_mode = true;
        else if (arg == "--no-header")
            no_header = true;
        else
            positional.push_back(arg);
    }

    // Parse positional args based on mode
    if (range_mode) {
        if (positional.size() == 1) {
            if (!parse_int_arg(positional[0], end_r)) {
                std::cerr << "Error: invalid integer argument '"
                          << positional[0] << "'\n";
                return 1;
            }
        } else if (positional.size() >= 2) {
            if (!parse_int_arg(positional[0], start_r) ||
                !parse_int_arg(positional[1], end_r)) {
                std::cerr << "Error: invalid integer argument\n";
                return 1;
            }
        }
        if (start_r < 2 || end_r < start_r || end_r > 260) {
            std::cerr
                << "Error: --verify-range requires 2 <= start <= end <= 260 "
                   "(brute-force verification memory is Theta(R^3 * K); "
                   "R > 260 needs a larger tier and can require 100+ GiB)\n";
            return 1;
        }
    } else if (csv_mode && !positional.empty()) {
        if (!parse_int_arg(positional[0], end_r)) {
            std::cerr << "Error: invalid integer argument '" << positional[0]
                      << "'\n";
            return 1;
        }
        if (end_r < 2)
            end_r = 2;
    } else if (!positional.empty()) {
        if (!parse_int_arg(positional[0], R)) {
            std::cerr << "Error: invalid integer argument '" << positional[0]
                      << "'\n";
            return 1;
        }
    }

    if (csv_mode && verify_mode && !range_mode) {
        std::cerr << "Error: --verify is only valid in single-R mode or with "
                     "--verify-range\n";
        return 1;
    }

    // -- Usage ----------------------------------------------------------
    if (positional.empty() && !range_mode && !csv_mode) {
        std::cerr
            << "Usage:\n"
            << "  ./predict <R>                     Analytical formula\n"
            << "  ./predict --verify <R>             Brute-force cross-check\n"
            << "  ./predict --verify-range [s] <e>   Sweep R=s..e\n"
            << "  ./predict --csv <N>                CSV table for R=2..N\n"
            << "  ./predict --csv --verify-range <N> Verified CSV for R=2..N\n";
        return 1;
    }

    // -- Range/CSV mode -----------------------------------------------
    if (range_mode || (csv_mode && end_r > 0)) {
        if (csv_mode && !range_mode)
            start_r = 2;

        if (!no_header)
            std::cout << "R,nk1,constant,coeff,formula_at_2R\n" << std::flush;

        for (int r = start_r; r <= end_r; r++) {
            R = r;
            const int64_t nk1 = A000788(R);
            const int64_t cst = constant_analytical(R);
            const int128_t coeff = widen(R) * R - nk1;
            const int128_t val = coeff * R - cst;

            if (range_mode) {
                std::cerr << "R=" << R << " ... ";
                int rc;
                if (R <= 16)
                    rc = run_verify<16, uint8_t>(R, nk1, cst, true);
                else if (R <= 32)
                    rc = run_verify<32, uint8_t>(R, nk1, cst, true);
                else if (R <= 64)
                    rc = run_verify<64, uint8_t>(R, nk1, cst, true);
                else if (R <= 127)
                    rc = run_verify<128, uint8_t>(R, nk1, cst, true);
                else if (R <= 256)
                    rc = run_verify<256, uint16_t>(R, nk1, cst, true);
                else
                    rc = run_verify<260, uint16_t>(R, nk1, cst, true);

                if (rc != 0) {
                    std::cerr << "FAILED at R=" << R << "\n";
                    return 1;
                }
            }

            std::cout << R << "," << nk1 << "," << cst << ","
                      << i128_to_string(coeff) << "," << i128_to_string(val)
                      << "\n"
                      << std::flush;
        }

        if (range_mode)
            std::cerr << "All R=" << start_r << ".." << end_r
                      << " verified ✓\n";
        return 0;
    }

    // -- Single R mode ------------------------------------------------
    if (R < 0) {
        std::cerr << "Error: R must be >= 0\n";
        return 1;
    }

    const int64_t expected_nk1 = A000788(R);
    const int64_t expected_const = constant_analytical(R);

    std::cerr << "Hamming ball prediction for R=" << R << "\n";
    std::cerr << "  [analytical] nk1 = A000788(" << R << ") = " << expected_nk1
              << "\n";
    std::cerr << "  [analytical] constant = " << expected_const << "\n";

    if (verify_mode) {
        if (R > 260) {
            std::cerr << "Error: --verify requires R <= 260 (brute-force "
                         "verification memory is Theta(R^3 * K); R > 260 "
                         "needs a larger tier and can require 100+ GiB)\n";
            return 1;
        }
        int rc;
        if (R <= 16)
            rc = run_verify<16, uint8_t>(R, expected_nk1, expected_const);
        else if (R <= 32)
            rc = run_verify<32, uint8_t>(R, expected_nk1, expected_const);
        else if (R <= 64)
            rc = run_verify<64, uint8_t>(R, expected_nk1, expected_const);
        else if (R <= 127)
            rc = run_verify<128, uint8_t>(R, expected_nk1, expected_const);
        else if (R <= 256)
            rc = run_verify<256, uint16_t>(R, expected_nk1, expected_const);
        else
            rc = run_verify<260, uint16_t>(R, expected_nk1, expected_const);

        if (rc != 0)
            return rc;
    }

    const int128_t coeff = widen(R) * R - expected_nk1;
    const int128_t formula_val = coeff * R - expected_const;

    std::cout << "(" << R << "nk-" << expected_nk1 << ") (n-k)-"
              << expected_const << ", EX:";
    if (R <= 12) {
        auto verts = build_hamming_ball<16, uint8_t>(R);
        for (int i = 0; i < R; i++)
            std::cout << " " << vertex_to_string(verts[i], R);
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
