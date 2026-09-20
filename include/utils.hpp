#ifndef UTILS_HPP
#define UTILS_HPP
// ── utils.hpp ─────────────────────────────────────────────────────────────
// Shared utility functions for Arrangement Graph A(n,k) analysis.
// Vertices are packed as k 5-bit symbols in a uint64_t.
// ──────────────────────────────────────────────────────────────────────────

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

// ── Formatting helpers ─────────────────────────────────────────────────────
inline constexpr const char *PRUNE_SEP = "   ";

/// Format an integer with thousands separators, e.g. 1234567 -> "1,234,567".
inline auto fcom(uint64_t n) -> std::string {
    std::string result = std::to_string(n);
    for (auto i = static_cast<int>(result.length()) - 3; i > 0; i -= 3)
        result.insert(i, ",");
    return result;
}

/// Format a non-negative floating-point value with thousands separators and
/// fixed precision.
inline auto fcom(double n, int precision = 1) -> std::string {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << n;
    std::string result = out.str();
    const int start = (!result.empty() && result[0] == '-') ? 1 : 0;
    auto pos = result.find('.');
    if (pos == std::string::npos)
        pos = result.length();
    for (auto i = static_cast<int>(pos) - 3; i > start; i -= 3)
        result.insert(i, ",");
    return result;
}

namespace arrangement {

inline constexpr int bits_per_symbol = 5;
inline constexpr int hamming_limit = 64;

// Bit position -> 5-bit chunk index (avoids division by 5)
inline constexpr std::array<int, 64> chunk_idx = {
    0, 0, 0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,
    3, 3, 3,  3,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,
    6, 6, 6,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  9,  9,  9,
    9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12,
};

/// Count the number of 5-bit chunks (positions) where two packed vertices
/// differ.  Returns early once `limit` differences are exceeded.
inline auto hamming_distance(uint64_t lhs, uint64_t rhs,
                             int limit = hamming_limit) -> int {
    uint64_t xv = lhs ^ rhs;
    int diffs = 0;
    while (xv != 0 && diffs <= limit) {
        const int chunk =
            chunk_idx[static_cast<std::size_t>(__builtin_ctzll(xv))];
        xv &=
            ~(UINT64_C(0x1F) << static_cast<unsigned>(chunk * bits_per_symbol));
        diffs++;
    }
    return diffs;
}

/// Two packed vertices are adjacent in A(n,k) iff they differ at exactly 1
/// position (5-bit chunk).
inline auto are_adjacent(uint64_t lhs, uint64_t rhs) -> bool {
    return hamming_distance(lhs, rhs, 1) == 1;
}

/// Count internal edges within a vertex subset: the number of pairs that
/// differ in exactly 1 position.
///   verts: array of packed vertices
///   n:     number of vertices in the array
inline auto count_internal_edges(const uint64_t *verts, int n) -> int {
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
/// Legacy placeholder retained for callers that still include this header.
/// The XOR-based implementation was never completed; keep the warning local
/// until the helper is either implemented or removed with its callers.
[[maybe_unused]] inline auto
count_external_neighbors(const uint64_t * /*verts*/, int /*n*/, int /*r_val*/)
    -> int64_t {
    return -1;
}

} // namespace arrangement

#endif
