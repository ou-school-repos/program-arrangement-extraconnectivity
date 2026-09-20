#ifndef ARRANGEMENT_UTILS_HPP
#define ARRANGEMENT_UTILS_HPP

#include <cstddef>
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

    static constexpr int bits_per_symbol = 6;
    static constexpr int bits_per_word = 64;

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

    [[nodiscard]] auto encode(const std::vector<int> &vertex) const
        -> packed_code_t {
        packed_code_t code = 0;
        for (int position = 0; position < k; ++position)
            code |= static_cast<packed_code_t>(
                        static_cast<unsigned>(vertex[position]))
                    << static_cast<unsigned>(bits_per_symbol * position);
        return code;
    }

    [[nodiscard]] auto rank_code(packed_code_t code) const -> std::size_t {
        std::uint64_t used = 0;
        std::size_t rank = 0;
        for (int position = 0; position < k; ++position) {
            const int symbol = static_cast<int>(
                (code >> static_cast<unsigned>(bits_per_symbol * position)) &
                static_cast<packed_code_t>(bits_per_word - 1));
            const std::uint64_t lower =
                symbol == 0
                    ? 0
                    : (std::uint64_t{1} << static_cast<unsigned>(symbol)) - 1;
            const int smaller_unused = __builtin_popcountll(lower & ~used);
            rank += static_cast<std::size_t>(smaller_unused) *
                    rank_weight[position];
            used |= std::uint64_t{1} << static_cast<unsigned>(symbol);
        }
        return rank;
    }

    [[nodiscard]] auto decode_rank(std::size_t rank) const -> packed_code_t {
        packed_code_t code = 0;
        const std::uint64_t all_symbols =
            n == bits_per_word
                ? std::numeric_limits<std::uint64_t>::max()
                : (std::uint64_t{1} << static_cast<unsigned>(n)) - 1;
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
                    << static_cast<unsigned>(bits_per_symbol * position);
            unused &= ~(std::uint64_t{1} << static_cast<unsigned>(symbol));
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
            const std::uint64_t bit = std::uint64_t{1}
                                      << static_cast<unsigned>(symbol);
            if (used & bit)
                continue;
            enumerate_valid(
                depth + 1,
                code | (static_cast<packed_code_t>(symbol)
                        << static_cast<unsigned>(bits_per_symbol * depth)),
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
            const int symbol = static_cast<int>(
                (code >> static_cast<unsigned>(bits_per_symbol * position)) &
                static_cast<packed_code_t>(bits_per_word - 1));
            used |= std::uint64_t{1} << static_cast<unsigned>(symbol);
        }
        for (int position = 0; position < k; ++position) {
            const packed_code_t mask =
                ~(static_cast<packed_code_t>(bits_per_word - 1)
                  << static_cast<unsigned>(bits_per_symbol * position));
            const packed_code_t base = code & mask;
            for (int symbol = 0; symbol < n; ++symbol) {
                if ((used >> static_cast<unsigned>(symbol)) & 1)
                    continue;
                function(base | (static_cast<packed_code_t>(symbol)
                                 << static_cast<unsigned>(bits_per_symbol *
                                                          position)));
            }
        }
    }
};

[[nodiscard]] inline auto arrangement_defect_sum(int count) -> std::int64_t {
    std::int64_t result = 0;
    for (int value = 0; value < count; ++value)
        result += __builtin_popcount(static_cast<unsigned>(value));
    return result;
}

[[nodiscard]] inline auto arrangement_bit_length(int value) -> int {
    int result = 0;
    while (value > 0) {
        ++result;
        value /= 2;
    }
    return result;
}

[[nodiscard]] inline auto arrangement_collision_constant(int count)
    -> std::int64_t {
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

    [[nodiscard]] auto m() const -> int { return m_; }
    [[nodiscard]] auto volume() const -> int { return volume_; }
    [[nodiscard]] auto g() const -> int { return g_; }
    [[nodiscard]] auto boundary() const -> std::int64_t { return boundary_; }
    [[nodiscard]] auto d() const -> int { return d_; }
    [[nodiscard]] auto embedding_gate() const -> bool {
        return embedding_gate_;
    }
    [[nodiscard]] auto hamming_boundary() const -> std::int64_t {
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

[[nodiscard]] inline auto full_star_parameters(int n, int k)
    -> FullStarParameters {
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
