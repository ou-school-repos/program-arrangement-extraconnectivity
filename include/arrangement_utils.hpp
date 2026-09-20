#ifndef ARRANGEMENT_UTILS_HPP
#define ARRANGEMENT_UTILS_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

__extension__ using packed_code_t = unsigned __int128;

struct PackedArrangementGraph {
    int n;
    int k;
    std::size_t valid_count{1};
    std::vector<std::size_t> rank_weight;

    PackedArrangementGraph(int n_value, int k_value)
        : n(n_value), k(k_value), rank_weight(k_value, 1) {
        for (int position = 0; position < k; ++position) {
            for (int factor = 0; factor < k - position - 1; ++factor) {
                rank_weight[position] *=
                    static_cast<std::size_t>(n - position - 1 - factor);
            }
        }
        for (int position = 0; position < k; ++position) {
            const auto factor = static_cast<std::size_t>(n - position);
            if (valid_count > std::numeric_limits<std::size_t>::max() / factor)
                throw std::overflow_error(
                    "P(n,k) overflows the rank address space.");
            valid_count *= factor;
        }
    }

    [[nodiscard]] packed_code_t encode(const std::vector<int> &vertex) const {
        constexpr int bits_per_symbol = 6;
        packed_code_t code = 0;
        for (int position = 0; position < k; ++position)
            code |= static_cast<packed_code_t>(vertex[position])
                    << (bits_per_symbol * position);
        return code;
    }

    [[nodiscard]] std::size_t rank_code(packed_code_t code) const {
        constexpr int bits_per_symbol = 6;
        std::uint64_t used = 0;
        std::size_t rank = 0;
        for (int position = 0; position < k; ++position) {
            const int symbol =
                static_cast<int>((code >> (bits_per_symbol * position)) & 63);
            const std::uint64_t lower =
                symbol == 0 ? 0 : (std::uint64_t{1} << symbol) - 1;
            const int smaller_unused = __builtin_popcountll(lower & ~used);
            rank += static_cast<std::size_t>(smaller_unused) *
                    rank_weight[position];
            used |= std::uint64_t{1} << symbol;
        }
        return rank;
    }

    [[nodiscard]] packed_code_t decode_rank(std::size_t rank) const {
        constexpr int bits_per_symbol = 6;
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
            code |= static_cast<packed_code_t>(symbol)
                    << (bits_per_symbol * position);
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
        constexpr int bits_per_symbol = 6;
        for (int symbol = 0; symbol < n; ++symbol) {
            const std::uint64_t bit = std::uint64_t{1} << symbol;
            if (used & bit)
                continue;
            enumerate_valid(depth + 1,
                            code | (static_cast<packed_code_t>(symbol)
                                    << (bits_per_symbol * depth)),
                            used | bit, function);
        }
    }

    template <typename Function>
    void for_each_valid_code(Function function) const {
        enumerate_valid(0, 0, 0, function);
    }

    template <typename Function>
    void for_each_neighbor(packed_code_t code, Function function) const {
        constexpr int bits_per_symbol = 6;
        std::uint64_t used = 0;
        for (int position = 0; position < k; ++position) {
            const int symbol =
                static_cast<int>((code >> (bits_per_symbol * position)) & 63);
            used |= std::uint64_t{1} << symbol;
        }
        for (int position = 0; position < k; ++position) {
            const packed_code_t mask = ~(static_cast<packed_code_t>(63)
                                         << (bits_per_symbol * position));
            const packed_code_t base = code & mask;
            for (int symbol = 0; symbol < n; ++symbol) {
                if ((used >> symbol) & 1)
                    continue;
                function(base | (static_cast<packed_code_t>(symbol)
                                 << (bits_per_symbol * position)));
            }
        }
    }
};

[[nodiscard]] inline std::int64_t arrangement_defect_sum(int count) {
    std::int64_t result = 0;
    for (int value = 0; value < count; ++value)
        result += __builtin_popcount(static_cast<unsigned>(value));
    return result;
}

[[nodiscard]] inline int arrangement_bit_length(int value) {
    int result = 0;
    while (value > 0) {
        ++result;
        value >>= 1;
    }
    return result;
}

[[nodiscard]] inline std::int64_t arrangement_collision_constant(int count) {
    if (count == 0)
        return 0;
    std::int64_t result = count - 1 - arrangement_defect_sum(count);
    for (int value = 1; value < count; ++value)
        result += arrangement_bit_length(value);
    return result;
}

class FullStarParameters {
  public:
    FullStarParameters(int m_value, int volume_value, int g_value,
                       std::int64_t boundary_value, int d_value,
                       bool embedding_gate_value,
                       std::int64_t hamming_boundary_value)
        : m_(m_value), volume_(volume_value), g_(g_value),
          boundary_(boundary_value), d_(d_value),
          embedding_gate_(embedding_gate_value),
          hamming_boundary_(hamming_boundary_value) {}

    [[nodiscard]] int m() const { return m_; }
    [[nodiscard]] int volume() const { return volume_; }
    [[nodiscard]] int g() const { return g_; }
    [[nodiscard]] std::int64_t boundary() const { return boundary_; }
    [[nodiscard]] int d() const { return d_; }
    [[nodiscard]] bool embedding_gate() const { return embedding_gate_; }
    [[nodiscard]] std::int64_t hamming_boundary() const {
        return hamming_boundary_;
    }

  private:
    int m_;
    int volume_;
    int g_;
    std::int64_t boundary_;
    int d_;
    bool embedding_gate_;
    std::int64_t hamming_boundary_;
};

[[nodiscard]] inline FullStarParameters full_star_parameters(int n, int k) {
    const int m = n - k;
    const int degree = k * m;
    const int volume = 1 + degree;
    const std::int64_t boundary =
        (static_cast<std::int64_t>(k) * (k - 1) * m) +
        (static_cast<std::int64_t>(k) * (k - 1) / 2 * m * (m - 1));
    const int d = arrangement_bit_length(volume - 1);
    const std::int64_t potential =
        arrangement_collision_constant(volume) +
        (static_cast<std::int64_t>(m) * arrangement_defect_sum(volume));
    return {m,
            volume,
            volume - 1,
            boundary,
            d,
            d <= k && d <= m,
            (static_cast<std::int64_t>(volume) * degree) - potential};
}

#endif
