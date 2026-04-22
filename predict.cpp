// Hamming Ball Predictor for Arrangement Graph Extraconnectivity
//
// Constructs the optimal (Hamming ball) vertex set of size R and computes
// the extraconnectivity formula directly, without exhaustive search.
//
// The Hamming ball of size R consists of the first R binary strings in
// lexicographic order, mapped to arrangement graph vertices by flipping
// positions corresponding to set bits to fresh symbols.
//
// Usage: ./predict [R]

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static int R = 10;

// ── Vertex type: vector<int> of length R ──────────────────────────────

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

// ── A000788: cumulative popcount ──────────────────────────────────────

static int popcount_u(unsigned n) {
    int c = 0;
    while (n) {
        c += n & 1;
        n >>= 1;
    }
    return c;
}

// Efficient O(log n) halving recurrence
static int A000788(int n) {
    if (n <= 0)
        return 0;
    int m = n / 2;
    if (n % 2 == 0)
        return 2 * A000788(m) + m;
    else
        return 2 * A000788(m) + m + popcount_u(static_cast<unsigned>(m));
}

// ── Hamming ball construction ─────────────────────────────────────────

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
                verts[i][d] = R + d; // fresh symbol per dimension
        }
    }
    return verts;
}

// ── Neighbor set computation ──────────────────────────────────────────

struct FormulaResult {
    int nk1;
    int constant;
};

static FormulaResult compute_formula(const std::vector<Vertex> &verts) {
    // Collect used symbols
    std::vector<int> used_syms;
    for (const auto &v : verts) {
        for (int s : v) {
            if (!std::any_of(used_syms.begin(), used_syms.end(),
                             [s](int x) { return x == s; }))
                used_syms.push_back(s);
        }
    }
    const int M = static_cast<int>(used_syms.size());

    // Anonymous coefficient: distinct groups per position
    int anon_coeff = 0;
    for (int p = 0; p < R; p++) {
        std::vector<Vertex> group_keys;
        for (int i = 0; i < R; i++) {
            Vertex key = verts[i];
            key[p] = -1; // blank position p
            if (!std::any_of(group_keys.begin(), group_keys.end(),
                             [&key](const Vertex &k) { return k == key; }))
                group_keys.push_back(key);
        }
        anon_coeff += static_cast<int>(group_keys.size());
    }

    // Named neighbors
    std::vector<Vertex> named_nbrs;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            for (int s : used_syms) {
                if (!contains_sym(verts[i], s)) {
                    Vertex vtx = verts[i];
                    vtx[p] = s;
                    if (std::any_of(
                            verts.begin(), verts.end(),
                            [&vtx](const Vertex &v) { return v == vtx; }))
                        continue;
                    if (!std::any_of(
                            named_nbrs.begin(), named_nbrs.end(),
                            [&vtx](const Vertex &v) { return v == vtx; }))
                        named_nbrs.push_back(vtx);
                }
            }
        }
    }

    const int nk1 = R * R - anon_coeff;
    const int named_sz = static_cast<int>(named_nbrs.size());
    const int constant = anon_coeff * (M - R) - named_sz;

    return {nk1, constant};
}

// ── Main ──────────────────────────────────────────────────────────────

int main(int argc, const char *argv[]) {
    if (argc >= 2) {
        R = static_cast<int>(std::strtol(argv[1], nullptr, 10));
        if (R < 2 || R > 32) {
            std::cerr << "R must be between 2 and 32\n";
            return 1;
        }
    }

    const int expected_nk1 = A000788(R);

    std::cerr << "Hamming ball prediction for R=" << R << "\n";
    std::cerr << "  A000788(" << R << ") = " << expected_nk1
              << " (expected nk1)\n";

    auto verts = build_hamming_ball();

    if (R <= 12) {
        std::cerr << "  vertex set:";
        for (int i = 0; i < R; i++)
            std::cerr << " " << vertex_to_string(verts[i]);
        std::cerr << "\n";
    }

    auto [nk1, constant] = compute_formula(verts);

    std::cerr << "  computed nk1 = " << nk1;
    if (nk1 == expected_nk1)
        std::cerr << " \xe2\x9c\x93 (matches A000788)\n";
    else
        std::cerr << " \xe2\x9c\x97 MISMATCH (expected " << expected_nk1
                  << ")\n";

    const int ver_n = 2 * R;
    const int ver_k = R;
    const int coeff = R * ver_k - nk1;
    const int formula_val = coeff * (ver_n - ver_k) - constant;

    // Output in same format as arrangementoptimized
    std::cout << "(" << R << "nk-" << nk1 << ") (n-k)-" << constant << ", EX:";
    if (R <= 12) {
        for (int i = 0; i < R; i++)
            std::cout << " " << vertex_to_string(verts[i]);
    } else {
        std::cout << " [" << R << " vertices, "
                  << (R + (1 + __builtin_clz(1) - __builtin_clz(R)))
                  << " symbols]";
    }
    std::cout << "\n";

    std::cerr << "  verify(n=" << ver_n << ",k=" << ver_k
              << "): |N(V')| = " << coeff << "\xc2\xb7" << (ver_n - ver_k)
              << " - " << constant << " = " << formula_val << "\n";

    return (nk1 == expected_nk1) ? 0 : 1;
}
