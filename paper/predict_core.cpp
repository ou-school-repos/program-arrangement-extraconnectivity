// -- A000788: cumulative popcount -- O(log R) ------------------------

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

// -- Constant C(R) -- O(R) ---------------------------------------------
// C(R) = (R-1) + ?_{x=1}^{R-1} bit_length(x) - E(R)
// Equivalently: (R-1) + ? zero-bits in binary(1..R-1)

static int64_t constant_analytical(int64_t R_val) {
    if (R_val <= 0)
        return 0;
    int64_t nk1 = A000788(R_val);
    int64_t L = 0;
    for (int64_t x = 1; x < R_val; x++)
        L += static_cast<int64_t>(bit_length_u(static_cast<uint64_t>(x)));
    return (R_val - 1) + L - nk1;
}

// -- Hamming ball construction ----------------------------------------

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

// -- Formula computation via construction -- O(R^3) --------------------

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

// -- Brute-force verification -- O(R^3 * log R) -------------------------

template <typename SymT>
static int64_t brute_force_neighbors(const std::vector<Vertex<SymT>> &verts,
                                     int n, int k) {
    std::vector<Vertex<SymT>> sorted_verts = verts;
    std::sort(sorted_verts.begin(), sorted_verts.end());

    std::vector<Vertex<SymT>> nbrs;
    Vertex<SymT> nbr;
    for (int i = 0; i < R; i++) {
        for (int p = 0; p < k; p++) {
            for (int s = 0; s < n; s++) {
                if (contains_sym(verts[i], s))
                    continue;
                std::memcpy(nbr.syms, verts[i].syms, k * sizeof(SymT));
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
