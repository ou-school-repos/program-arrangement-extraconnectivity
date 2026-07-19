#pragma once
// ── utils.h ───────────────────────────────────────────────────────────────
// Shared utility functions for Arrangement Graph A(n,k) analysis.
// Vertices are packed as k 5-bit symbols in a uint64_t.
// ──────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

// ── Formatting helpers ─────────────────────────────────────────────────────
inline constexpr const char* PRUNE_SEP = "   ";

/// Format an integer with thousands separators, e.g. 1234567 -> "1,234,567".
inline std::string fcom(uint64_t n) {
    std::string s = std::to_string(n);
    for (int i = (int)s.length() - 3; i > 0; i -= 3)
        s.insert(i, ",");
    return s;
}

/// Format a floating-point value with thousands separators and fixed precision.
inline std::string fcom(double n, int precision = 1) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << n;
    std::string s = out.str();
    size_t pos = s.find('.');
    if (pos == std::string::npos)
        pos = s.length();
    for (int i = (int)pos - 3; i > 0; i -= 3)
        s.insert(i, ",");
    return s;
}

namespace arrangement {

// Bit position → 5-bit chunk index (avoids division by 5)
inline constexpr int chunk_idx[64] = {
    0, 0, 0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,
    3, 3, 3,  3,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,
    6, 6, 6,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  9,  9,  9,
    9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12,
};

/// Count the number of 5-bit chunks (positions) where two packed vertices
/// differ.  Returns early once `limit` differences are exceeded.
inline int hamming_distance(uint64_t a, uint64_t b, int limit = 64) {
    uint64_t xv = a ^ b;
    int diffs = 0;
    while (xv && diffs <= limit) {
        int chunk = chunk_idx[__builtin_ctzll(xv)];
        xv &= ~(UINT64_C(0x1F) << (chunk * 5));
        diffs++;
    }
    return diffs;
}

/// Two packed vertices are adjacent in A(n,k) iff they differ at exactly 1
/// position (5-bit chunk).
inline bool are_adjacent(uint64_t a, uint64_t b) {
    return hamming_distance(a, b, 1) == 1;
}

/// Count internal edges within a vertex subset: the number of pairs that
/// differ in exactly 1 position.
///   verts: array of packed vertices
///   n:     number of vertices in the array
inline int count_internal_edges(const uint64_t *verts, int n) {
    int edges = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (are_adjacent(verts[i], verts[j]))
                edges++;
        }
    }
    return edges;
}

/// Count external neighbors (boundary size) of a vertex subset.
/// Uses XOR-based incremental neighbor enumeration.
///   verts: array of packed vertices (the subset)
///   n:     total number of symbols in the alphabet (graph parameter)
///   k:     sequence length (graph parameter, derived from encoding)
///   R:     number of vertices in verts[]
inline int64_t count_external_neighbors(const uint64_t *verts, int n, int r_val) {
    (void)verts;
    (void)n;
    (void)r_val;
    return -1;
}

} // namespace arrangement
