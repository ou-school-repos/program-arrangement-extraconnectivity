// Hamming Ball Predictor for Arrangement Graph Extraconnectivity
//
// Three-tier prediction:
//   1. O(R)    — analytical: A000788 coefficient + cumulative-zeros constant
//   2. O(R³)   — construction: Hamming ball + formula computation
//   3. O(R⁴)   — verification: brute-force neighbor enumeration (R ≤ 20)
//
// Usage: ./predict [R]

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

static int R = 10;

// ── Vertex type ───────────────────────────────────────────────────────

using Vertex = std::vector<int>;

static bool contains_sym(const Vertex &v, int sym) {
    return std::any_of(v.begin(), v.end(), [sym](int s) { return s == sym; });
}

static std::string vertex_to_string(const Vertex &v) {
    std::string str(R, ' ');
    for (int i = 0; i < R; i++) {
        str[i] = (v[i] < 26) ? static_cast<char>('A' + v[i])
                             : static_cast<char>('a' + v[i] - 26);
    }
    return str;
}

// ── A000788: cumulative popcount — O(log R) ──────────────────────────

static int popcount_u(unsigned n) {
    int c = 0;
    while (n) {
        c += n & 1;
        n >>= 1;
    }
    return c;
}

static int A000788(int n) {
    if (n <= 0)
        return 0;
    int m = n / 2;
    if (n % 2 == 0)
        return 2 * A000788(m) + m;
    else
        return 2 * A000788(m) + m + popcount_u(static_cast<unsigned>(m));
}

// ── Cumulative zero-count constant — O(R) ────────────────────────────
// C(R) = (R-1) + Σ_{x=1}^{R-1} Z(x)
// where Z(x) = number of 0-bits in binary(x) up to its MSB.

static int bit_length(int x) {
    int len = 0;
    while (x > 0) {
        len++;
        x >>= 1;
    }
    return len;
}

static int zero_bits(int x) {
    if (x <= 0)
        return 0;
    return bit_length(x) - popcount_u(static_cast<unsigned>(x));
}

static int constant_analytical(int R_val) {
    int sum = R_val - 1;
    for (int x = 1; x < R_val; x++)
        sum += zero_bits(x);
    return sum;
}

// ── Hamming ball construction — O(R log R) ───────────────────────────

static std::vector<Vertex> build_hamming_ball() {
    int dims = 0;
    while ((1 << dims) < R)
        dims++;

    Vertex identity(R);
    for (int j = 0; j < R; j++)
        identity[j] = j;

    std::vector<Vertex> verts(R);
    for (int i = 0; i < R; i++) {
        verts[i] = identity;
        for (int d = 0; d < dims; d++) {
            if (i & (1 << d))
                verts[i][d] = R + d;
        }
    }
    return verts;
}

// ── Formula computation via construction — O(R³) ─────────────────────

struct FormulaResult {
    int nk1;
    int constant;
};

static FormulaResult compute_formula(const std::vector<Vertex> &verts) {
    // Collect used symbols
    std::set<int> used_set;
    for (const auto &v : verts)
        for (int s : v)
            used_set.insert(s);
    std::vector<int> used_syms(used_set.begin(), used_set.end());
    const int M = static_cast<int>(used_syms.size());

    // Anonymous coefficient: distinct groups per position
    int anon_coeff = 0;
    for (int p = 0; p < R; p++) {
        std::set<Vertex> group_keys;
        for (int i = 0; i < R; i++) {
            Vertex key = verts[i];
            key[p] = -1;
            group_keys.insert(key);
        }
        anon_coeff += static_cast<int>(group_keys.size());
    }

    // Named neighbors (using set for O(log N) dedup)
    std::set<Vertex> ball(verts.begin(), verts.end());
    std::set<Vertex> named_set;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            for (int s : used_syms) {
                if (!contains_sym(verts[i], s)) {
                    Vertex vtx = verts[i];
                    vtx[p] = s;
                    if (ball.count(vtx) == 0)
                        named_set.insert(vtx);
                }
            }
        }
    }

    const int nk1 = R * R - anon_coeff;
    const int named_sz = static_cast<int>(named_set.size());
    const int constant = anon_coeff * (M - R) - named_sz;

    return {nk1, constant};
}

// ── Brute-force verification — O(R⁴) ─────────────────────────────────

static int brute_force_neighbors(const std::vector<Vertex> &verts, int n,
                                 int k) {
    std::set<Vertex> ball(verts.begin(), verts.end());
    std::set<Vertex> neighbors;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < k; p++) {
            for (int s = 0; s < n; s++) {
                if (contains_sym(verts[i], s))
                    continue;
                Vertex nbr = verts[i];
                nbr[p] = s;
                if (ball.count(nbr) == 0)
                    neighbors.insert(nbr);
            }
        }
    }
    return static_cast<int>(neighbors.size());
}

// ── Main ──────────────────────────────────────────────────────────────

int main(int argc, const char *argv[]) {
    if (argc >= 2) {
        R = static_cast<int>(std::strtol(argv[1], nullptr, 10));
        if (R < 2 || R > 64) {
            std::cerr << "R must be between 2 and 64\n";
            return 1;
        }
    }

    // ── Tier 1: Analytical — O(R) ─────────────────────────────────────
    const int expected_nk1 = A000788(R);
    const int expected_const = constant_analytical(R);

    std::cerr << "Hamming ball prediction for R=" << R << "\n";
    std::cerr << "  [analytical] nk1 = A000788(" << R << ") = " << expected_nk1
              << "\n";
    std::cerr << "  [analytical] constant = " << expected_const << "\n";

    // ── Tier 2: Construction verification — O(R³) ─────────────────────
    if (R <= 32) {
        auto verts = build_hamming_ball();

        if (R <= 12) {
            std::cerr << "  vertex set:";
            for (int i = 0; i < R; i++)
                std::cerr << " " << vertex_to_string(verts[i]);
            std::cerr << "\n";
        }

        auto [nk1, constant] = compute_formula(verts);

        std::cerr << "  [construction] nk1 = " << nk1;
        if (nk1 == expected_nk1)
            std::cerr << " \xe2\x9c\x93\n";
        else {
            std::cerr << " \xe2\x9c\x97 MISMATCH\n";
            return 1;
        }
        std::cerr << "  [construction] constant = " << constant;
        if (constant == expected_const)
            std::cerr << " \xe2\x9c\x93\n";
        else {
            std::cerr << " \xe2\x9c\x97 MISMATCH (expected " << expected_const
                      << ")\n";
            return 1;
        }

        // ── Tier 3: Brute-force verification — O(R⁴) ─────────────────
        if (R <= 20) {
            const int ver_n = 2 * R;
            const int brute_count = brute_force_neighbors(verts, ver_n, R);
            const int coeff = R * R - nk1;
            const int formula_val = coeff * R - constant;
            std::cerr << "  [brute-force] |N(V')| = " << brute_count;
            if (brute_count == formula_val)
                std::cerr << " \xe2\x9c\x93\n";
            else {
                std::cerr << " \xe2\x9c\x97 MISMATCH (formula gives "
                          << formula_val << ")\n";
                return 1;
            }
        } else {
            std::cerr << "  [brute-force] skipped (R>20)\n";
        }
    } else {
        std::cerr << "  [construction] skipped (R>32)\n";
    }

    // ── Output ────────────────────────────────────────────────────────
    const int coeff = R * R - expected_nk1;
    const int formula_val = coeff * R - expected_const;

    std::cout << "(" << R << "nk-" << expected_nk1 << ") (n-k)-"
              << expected_const << ", EX:";
    if (R <= 12) {
        auto verts = build_hamming_ball();
        for (int i = 0; i < R; i++)
            std::cout << " " << vertex_to_string(verts[i]);
    } else {
        std::cout << " [" << R << " vertices]";
    }
    std::cout << "\n";

    std::cerr << "  formula(n=" << 2 * R << ",k=" << R
              << "): |N(V')| = " << coeff << "\xc2\xb7" << R << " - "
              << expected_const << " = " << formula_val << "\n";

    return 0;
}
