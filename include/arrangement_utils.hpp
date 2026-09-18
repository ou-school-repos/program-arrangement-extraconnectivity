#ifndef ARRANGEMENT_UTILS_HPP
#define ARRANGEMENT_UTILS_HPP

#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

__extension__ typedef unsigned __int128 packed_code_t;

struct PackedArrangementGraph {
    int n;
    int k;
    std::size_t valid_count;
    std::vector<std::size_t> rank_weight;

    PackedArrangementGraph(int n_value, int k_value)
        : n(n_value), k(k_value), valid_count(1), rank_weight(k_value, 1) {
        for (int position = 0; position < k; ++position) {
            for (int factor = 0; factor < k - position - 1; ++factor) {
                rank_weight[position] *=
                    static_cast<std::size_t>(n - position - 1 - factor);
            }
        }
        for (int position = 0; position < k; ++position) {
            const std::size_t factor = static_cast<std::size_t>(n - position);
            if (valid_count > std::numeric_limits<std::size_t>::max() / factor)
                throw std::overflow_error(
                    "P(n,k) overflows the rank address space.");
            valid_count *= factor;
        }
    }

    packed_code_t encode(const std::vector<int> &vertex) const {
        packed_code_t code = 0;
        for (int position = 0; position < k; ++position)
            code |= static_cast<packed_code_t>(vertex[position])
                    << (6 * position);
        return code;
    }

    std::size_t rank_code(packed_code_t code) const {
        std::uint64_t used = 0;
        std::size_t rank = 0;
        for (int position = 0; position < k; ++position) {
            const int symbol = static_cast<int>((code >> (6 * position)) & 63);
            const std::uint64_t lower =
                symbol == 0 ? 0 : (std::uint64_t{1} << symbol) - 1;
            const int smaller_unused = __builtin_popcountll(lower & ~used);
            rank += static_cast<std::size_t>(smaller_unused) *
                    rank_weight[position];
            used |= std::uint64_t{1} << symbol;
        }
        return rank;
    }

    packed_code_t decode_rank(std::size_t rank) const {
        packed_code_t code = 0;
        const std::uint64_t all_symbols =
            n == 64 ? std::numeric_limits<std::uint64_t>::max()
                    : (std::uint64_t{1} << n) - 1;
        std::uint64_t unused = all_symbols;
        for (int position = 0; position < k; ++position) {
            const std::size_t weight = rank_weight[position];
            const std::size_t ordinal = rank / weight;
            rank %= weight;
            std::uint64_t candidates = unused;
            for (std::size_t count = 0; count < ordinal; ++count)
                candidates &= candidates - 1;
            const int symbol = __builtin_ctzll(candidates);
            code |= static_cast<packed_code_t>(symbol) << (6 * position);
            unused &= ~(std::uint64_t{1} << symbol);
        }
        return code;
    }

    template <typename Function>
    void enumerate_valid(int depth, packed_code_t code, std::uint64_t used,
                         Function &function) const {
        if (depth == k) {
            function(code);
            return;
        }
        for (int symbol = 0; symbol < n; ++symbol) {
            const std::uint64_t bit = std::uint64_t{1} << symbol;
            if (used & bit)
                continue;
            enumerate_valid(
                depth + 1,
                code | (static_cast<packed_code_t>(symbol) << (6 * depth)),
                used | bit, function);
        }
    }

    template <typename Function>
    void for_each_valid_code(Function function) const {
        enumerate_valid(0, 0, 0, function);
    }

    template <typename Function>
    void for_each_neighbor(packed_code_t code, Function function) const {
        std::uint64_t used = 0;
        for (int position = 0; position < k; ++position) {
            const int symbol = static_cast<int>((code >> (6 * position)) & 63);
            used |= std::uint64_t{1} << symbol;
        }
        for (int position = 0; position < k; ++position) {
            const packed_code_t mask =
                ~(static_cast<packed_code_t>(63) << (6 * position));
            const packed_code_t base = code & mask;
            for (int symbol = 0; symbol < n; ++symbol) {
                if ((used >> symbol) & 1)
                    continue;
                function(base | (static_cast<packed_code_t>(symbol)
                                 << (6 * position)));
            }
        }
    }
};

inline long long arrangement_defect_sum(int r) {
    long long result = 0;
    for (int value = 0; value < r; ++value)
        result += __builtin_popcount(static_cast<unsigned>(value));
    return result;
}

inline int arrangement_bit_length(int value) {
    int result = 0;
    while (value > 0) {
        ++result;
        value >>= 1;
    }
    return result;
}

inline long long arrangement_collision_constant(int r) {
    if (r == 0)
        return 0;
    long long result = r - 1 - arrangement_defect_sum(r);
    for (int value = 1; value < r; ++value)
        result += arrangement_bit_length(value);
    return result;
}

class FullStarParameters {
  public:
    FullStarParameters(int m_value, int volume_value, int g_value,
                       long long boundary_value, int d_value,
                       bool embedding_gate_value,
                       long long hamming_boundary_value)
        : m_(m_value), volume_(volume_value), g_(g_value),
          boundary_(boundary_value), d_(d_value),
          embedding_gate_(embedding_gate_value),
          hamming_boundary_(hamming_boundary_value) {}

    int m() const { return m_; }
    int volume() const { return volume_; }
    int g() const { return g_; }
    long long boundary() const { return boundary_; }
    int d() const { return d_; }
    bool embedding_gate() const { return embedding_gate_; }
    long long hamming_boundary() const { return hamming_boundary_; }

  private:
    int m_;
    int volume_;
    int g_;
    long long boundary_;
    int d_;
    bool embedding_gate_;
    long long hamming_boundary_;
};

inline FullStarParameters full_star_parameters(int n, int k) {
    const int m = n - k;
    const int degree = k * m;
    const int volume = 1 + degree;
    const long long boundary =
        static_cast<long long>(k) * (k - 1) * m +
        static_cast<long long>(k) * (k - 1) / 2 * m * (m - 1);
    const int d = arrangement_bit_length(volume - 1);
    const long long potential =
        arrangement_collision_constant(volume) +
        static_cast<long long>(m) * arrangement_defect_sum(volume);
    return {m,
            volume,
            volume - 1,
            boundary,
            d,
            d <= k && d <= m,
            static_cast<long long>(volume) * degree - potential};
}

#endif
