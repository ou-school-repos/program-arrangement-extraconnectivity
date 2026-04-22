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
#include <sstream>
#include <string>
#include <vector>

static int R = 10;

// ── Vertex representation (same as arrangementoptimized.cpp) ──────────
// Each r-permutation packed into uint64_t with 5-bit nibbles.

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

static std::string vertex_to_string(uint64_t vertex) {
    std::string str(R, ' ');
    for (int i = 0; i < R; i++) {
        const int sym = get_sym(vertex, i);
        str[i] = (sym < 26) ? static_cast<char>('A' + sym)
                            : static_cast<char>('a' + sym - 26);
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

static int A000788(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += popcount_u(static_cast<unsigned>(i));
    return sum;
}

// ── Hamming ball construction ─────────────────────────────────────────
// Build R vertices: for each i in 0..R-1, vertex i is the identity
// permutation with positions corresponding to set bits of i changed
// to fresh symbols.
//
// The number of "dimensions" = ceil(log2(R)).
// Each dimension d maps to position d and fresh symbol R+d.

static std::vector<uint64_t> build_hamming_ball() {
    // Determine number of dimensions needed
    int dims = 0;
    while ((1 << dims) < R)
        dims++;

    std::vector<uint64_t> verts(R);

    // Identity permutation: symbol j at position j
    uint64_t identity = 0;
    for (int j = 0; j < R; j++)
        identity = set_sym(identity, j, j);

    for (int i = 0; i < R; i++) {
        uint64_t v = identity;
        for (int d = 0; d < dims; d++) {
            if (i & (1 << d)) {
                // Flip position d to fresh symbol R+d
                v = set_sym(v, d, R + d);
            }
        }
        verts[i] = v;
    }
    return verts;
}

// ── Neighbor set computation (same algorithm as verify_neighbor_set) ──

struct FormulaResult {
    int nk1;      // coefficient: formula is (Rk - nk1)(n-k) - constant
    int constant; // = nk1 + cons
};

static FormulaResult compute_formula(const std::vector<uint64_t> &verts) {
    // Compute used symbols mask
    uint32_t used_syms_mask = 0;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++)
            used_syms_mask |= (1U << get_sym(verts[i], p));
    }
    const int M = __builtin_popcount(used_syms_mask);

    // Anonymous coefficient: sum of distinct groups per position
    int anon_coeff = 0;
    for (int p = 0; p < R; p++) {
        std::vector<uint64_t> group_keys;
        for (int i = 0; i < R; i++) {
            uint64_t key = set_sym(verts[i], p, 0x1F); // blank position p
            if (!std::any_of(group_keys.begin(), group_keys.end(),
                             [key](uint64_t k) { return k == key; }))
                group_keys.push_back(key);
        }
        anon_coeff += static_cast<int>(group_keys.size());
    }

    // Named neighbors: neighbors using symbols already in V' but not in V'
    std::vector<uint64_t> named_nbrs;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            for (int s = 0; s < 32; s++) {
                if ((used_syms_mask & (1U << s)) == 0)
                    continue;
                if (!contains_sym(verts[i], s)) {
                    uint64_t vtx = set_sym(verts[i], p, s);
                    if (std::any_of(verts.begin(), verts.end(),
                                    [vtx](uint64_t v) { return v == vtx; }))
                        continue;
                    if (!std::any_of(named_nbrs.begin(), named_nbrs.end(),
                                     [vtx](uint64_t v) { return v == vtx; }))
                        named_nbrs.push_back(vtx);
                }
            }
        }
    }

    const int nk1 = R * R - anon_coeff;
    const int named_sz = static_cast<int>(named_nbrs.size());
    const int total_const = anon_coeff * (M - R) - named_sz;
    const int constant = total_const; // This is nk1 + cons as printed

    return {nk1, constant};
}

// ── Brute-force verification ──────────────────────────────────────────

static int brute_force_neighbors(const std::vector<uint64_t> &verts, int n,
                                 int k) {
    std::vector<uint64_t> neighbors;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < k; p++) {
            for (int s = 0; s < n; s++) {
                if (contains_sym(verts[i], s))
                    continue;
                uint64_t nbr = set_sym(verts[i], p, s);
                if (!std::any_of(verts.begin(), verts.end(),
                                 [nbr](uint64_t v) { return v == nbr; }))
                    neighbors.push_back(nbr);
            }
        }
    }
    std::sort(neighbors.begin(), neighbors.end());
    neighbors.erase(std::unique(neighbors.begin(), neighbors.end()),
                    neighbors.end());
    return static_cast<int>(neighbors.size());
}

// ── Main ──────────────────────────────────────────────────────────────

int main(int argc, const char *argv[]) {
    if (argc >= 2) {
        R = static_cast<int>(std::strtol(argv[1], nullptr, 10));
        if (R < 2 || R > 12) {
            std::cerr << "R must be between 2 and 12\n";
            return 1;
        }
    }

    const int expected_nk1 = A000788(R);

    std::cerr << "Hamming ball prediction for R=" << R << "\n";
    std::cerr << "  A000788(" << R << ") = " << expected_nk1
              << " (expected nk1)\n";

    auto verts = build_hamming_ball();

    std::cerr << "  vertex set:";
    for (int i = 0; i < R; i++)
        std::cerr << " " << vertex_to_string(verts[i]);
    std::cerr << "\n";

    auto [nk1, constant] = compute_formula(verts);

    std::cerr << "  computed nk1 = " << nk1;
    if (nk1 == expected_nk1)
        std::cerr << " \xe2\x9c\x93 (matches A000788)\n";
    else
        std::cerr << " \xe2\x9c\x97 MISMATCH (expected " << expected_nk1
                  << ")\n";

    // Determine n for verification
    int max_sym = 0;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < R; p++) {
            max_sym = std::max(max_sym, get_sym(verts[i], p));
        }
    }
    const int ver_n = std::max(2 * R, max_sym + 1);
    const int ver_k = R;

    const int coeff = R * ver_k - nk1;
    const int formula_val = coeff * (ver_n - ver_k) - constant;

    // Output in same format as arrangementoptimized
    std::cout << "(" << R << "nk-" << nk1 << ") (n-k)-" << constant << ", EX:";
    for (int i = 0; i < R; i++)
        std::cout << " " << vertex_to_string(verts[i]);
    std::cout << "\n";

    // Brute-force verification
    const int brute_count = brute_force_neighbors(verts, ver_n, ver_k);
    std::cerr << "  verify(n=" << ver_n << ",k=" << ver_k << "): |N(V')| = ("
              << R << "\xc2\xb7" << ver_k << "-" << nk1 << ")(" << ver_n << "-"
              << ver_k << ")-" << constant << " = " << coeff << "\xc2\xb7"
              << (ver_n - ver_k) << " - " << constant << " = " << formula_val
              << "\n";
    std::cerr << "  brute-force neighbor count: " << brute_count;
    if (brute_count == formula_val)
        std::cerr << " \xe2\x9c\x93\n";
    else
        std::cerr << " \xe2\x9c\x97 MISMATCH!\n";

    return (brute_count == formula_val) ? 0 : 1;
}
