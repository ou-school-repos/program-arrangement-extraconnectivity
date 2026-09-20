// Heuristically search for a fixed-volume set beating the Hamming boundary.
//
// This searches arbitrary R-subsets, not only connected ones. Every objective
// value is computed by direct external-neighbor enumeration. A found witness
// is valid evidence; failure to find one is not a lower-bound certificate.
//
// Usage: witness_finder R k m seconds seed

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Vertex = std::vector<std::uint8_t>;

struct VertexHash {
    std::size_t operator()(const Vertex &vertex) const noexcept {
        std::size_t hash = 0xcbf29ce484222325ULL;
        for (const std::uint8_t symbol : vertex) {
            hash ^= symbol;
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }
};

using VertexSet = std::unordered_set<Vertex, VertexHash>;

bool is_injective(const Vertex &vertex) {
    std::uint64_t used = 0;
    for (const std::uint8_t symbol : vertex) {
        const std::uint64_t bit = std::uint64_t{1} << symbol;
        if ((used & bit) != 0)
            return false;
        used |= bit;
    }
    return true;
}

std::size_t external_boundary(const std::vector<Vertex> &set,
                              const int alphabet_size) {
    const VertexSet members(set.begin(), set.end());
    VertexSet boundary;

    for (const Vertex &vertex : set) {
        std::uint64_t used =
            std::accumulate(vertex.begin(), vertex.end(), std::uint64_t{0},
                            [](std::uint64_t acc, std::uint8_t sym) {
                                return acc | (std::uint64_t{1} << sym);
                            });

        for (std::size_t coordinate = 0; coordinate < vertex.size();
             ++coordinate) {
            for (int symbol = 0; symbol < alphabet_size; ++symbol) {
                const std::uint64_t bit = std::uint64_t{1}
                                          << static_cast<unsigned int>(symbol);
                if ((used & bit) != 0)
                    continue;

                Vertex neighbor = vertex;
                neighbor[coordinate] = static_cast<std::uint8_t>(symbol);
                if (members.count(neighbor) == 0)
                    boundary.insert(std::move(neighbor));
            }
        }
    }
    return boundary.size();
}

std::int64_t defect_sum(const int volume) {
    std::int64_t result = 0;
    for (int value = 0; value < volume; ++value)
        result += __builtin_popcount(static_cast<unsigned int>(value));
    return result;
}

std::int64_t collision_constant(const int volume) {
    std::int64_t bit_lengths = 0;
    for (int value = 1; value < volume; ++value)
        bit_lengths += static_cast<std::int64_t>(
            32 - __builtin_clz(static_cast<unsigned int>(value)));
    return static_cast<std::int64_t>(volume - 1) + bit_lengths -
           defect_sum(volume);
}

std::vector<Vertex> hamming_ball(const int volume, const int dimensions,
                                 const int word_length) {
    std::vector<Vertex> result;
    result.reserve(static_cast<std::size_t>(volume));
    for (int code = 0; code < volume; ++code) {
        Vertex vertex(static_cast<std::size_t>(word_length));
        for (int coordinate = 0; coordinate < word_length; ++coordinate)
            vertex[static_cast<std::size_t>(coordinate)] =
                static_cast<std::uint8_t>(coordinate);
        for (int bit = 0; bit < dimensions; ++bit) {
            if ((code & (1 << bit)) != 0)
                vertex[static_cast<std::size_t>(bit)] =
                    static_cast<std::uint8_t>(word_length + bit);
        }
        result.push_back(std::move(vertex));
    }
    return result;
}

std::vector<Vertex> rook_star(const int volume, const int word_length,
                              const int free_symbols) {
    std::vector<Vertex> result;
    result.reserve(static_cast<std::size_t>(volume));
    Vertex center(static_cast<std::size_t>(word_length));
    for (int coordinate = 0; coordinate < word_length; ++coordinate)
        center[static_cast<std::size_t>(coordinate)] =
            static_cast<std::uint8_t>(coordinate);
    result.push_back(center);

    for (int arm = 0; arm < volume - 1; ++arm) {
        Vertex vertex = center;
        const int coordinate = arm % word_length;
        const int symbol = word_length + (arm / word_length) % free_symbols;
        vertex[static_cast<std::size_t>(coordinate)] =
            static_cast<std::uint8_t>(symbol);
        result.push_back(std::move(vertex));
    }
    return result;
}

bool contains_except(const std::vector<Vertex> &set, const Vertex &candidate,
                     const std::size_t excluded) {
    for (std::size_t index = 0; index < set.size(); ++index) {
        if (index != excluded && set[index] == candidate)
            return true;
    }
    return false;
}

std::vector<Vertex> random_connected_set(const int volume,
                                         const int word_length,
                                         const int alphabet_size,
                                         std::mt19937 &generator) {
    std::vector<Vertex> result;
    Vertex center(static_cast<std::size_t>(word_length));
    for (int coordinate = 0; coordinate < word_length; ++coordinate)
        center[static_cast<std::size_t>(coordinate)] =
            static_cast<std::uint8_t>(coordinate);
    result.push_back(std::move(center));

    std::uniform_int_distribution<std::size_t> member_pick;
    std::uniform_int_distribution<int> coordinate_pick(0, word_length - 1);
    std::uniform_int_distribution<int> symbol_pick(0, alphabet_size - 1);
    std::size_t failed_attempts = 0;
    const std::size_t attempt_limit =
        static_cast<std::size_t>(volume) * 100000U;

    while (result.size() < static_cast<std::size_t>(volume)) {
        member_pick =
            std::uniform_int_distribution<std::size_t>(0, result.size() - 1);
        Vertex candidate = result[member_pick(generator)];
        const int coordinate = coordinate_pick(generator);
        candidate[static_cast<std::size_t>(coordinate)] =
            static_cast<std::uint8_t>(symbol_pick(generator));
        if (is_injective(candidate) && std::find(result.begin(), result.end(),
                                                 candidate) == result.end()) {
            result.push_back(std::move(candidate));
            failed_attempts = 0;
        } else if (++failed_attempts > attempt_limit) {
            throw std::runtime_error(
                "could not construct a random connected starting set");
        }
    }
    return result;
}

void consider(const std::vector<Vertex> &candidate, const std::size_t value,
              std::size_t &best_value, std::vector<Vertex> &best_set) {
    if (value < best_value) {
        best_value = value;
        best_set = candidate;
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " R k m seconds seed\n";
        return 2;
    }

    try {
        const int volume = std::stoi(argv[1]);
        const int word_length = std::stoi(argv[2]);
        const int free_symbols = std::stoi(argv[3]);
        const double seconds = std::stod(argv[4]);
        const unsigned int seed =
            static_cast<unsigned int>(std::stoul(argv[5]));
        const int alphabet_size = word_length + free_symbols;

        if (volume < 2 || word_length < 1 || free_symbols < 1 ||
            !std::isfinite(seconds) || seconds <= 0.0 || alphabet_size > 64) {
            throw std::invalid_argument(
                "require R >= 2, k >= 1, m >= 1, seconds > 0, and k+m <= 64");
        }
        const int dimension =
            32 - __builtin_clz(static_cast<unsigned int>(volume - 1));
        if (dimension > word_length || dimension > free_symbols) {
            throw std::invalid_argument(
                "Hamming comparison requires bit_length(R-1) <= k,m");
        }
        const std::int64_t hamming_formula =
            (static_cast<std::int64_t>(volume) * word_length -
             defect_sum(volume)) *
                free_symbols -
            collision_constant(volume);
        std::vector<Vertex> hamming =
            hamming_ball(volume, dimension, word_length);
        const std::size_t hamming_direct =
            external_boundary(hamming, alphabet_size);
        if (hamming_formula < 0 ||
            hamming_direct != static_cast<std::size_t>(hamming_formula)) {
            throw std::runtime_error(
                "direct Hamming boundary disagrees with its formula");
        }

        std::mt19937 generator(seed);
        std::vector<std::vector<Vertex>> starts;
        starts.push_back(std::move(hamming));
        if (static_cast<std::int64_t>(volume - 1) <=
            static_cast<std::int64_t>(word_length) * free_symbols) {
            starts.push_back(rook_star(volume, word_length, free_symbols));
        }
        starts.push_back(random_connected_set(volume, word_length,
                                              alphabet_size, generator));

        std::size_t best_value = std::numeric_limits<std::size_t>::max();
        std::vector<Vertex> best_set;
        const auto start_time = std::chrono::steady_clock::now();
        const auto deadline =
            start_time +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(seconds));
        std::uint64_t iterations = 0;
        std::size_t run = 0;
        std::uniform_real_distribution<double> probability(0.0, 1.0);

        while (std::chrono::steady_clock::now() < deadline) {
            std::vector<Vertex> current =
                run < starts.size()
                    ? starts[run]
                    : random_connected_set(volume, word_length, alphabet_size,
                                           generator);
            std::size_t current_value =
                external_boundary(current, alphabet_size);
            consider(current, current_value, best_value, best_set);

            double temperature = 3.0;
            for (int step = 0;
                 step < 4000 && std::chrono::steady_clock::now() < deadline;
                 ++step, ++iterations) {
                std::uniform_int_distribution<std::size_t> index_pick(
                    0, current.size() - 1);
                std::uniform_int_distribution<int> coordinate_pick(
                    0, word_length - 1);
                std::uniform_int_distribution<int> symbol_pick(
                    0, alphabet_size - 1);
                const std::size_t removed_index = index_pick(generator);
                Vertex candidate = current[index_pick(generator)];
                const int coordinate = coordinate_pick(generator);
                candidate[static_cast<std::size_t>(coordinate)] =
                    static_cast<std::uint8_t>(symbol_pick(generator));
                if (!is_injective(candidate) ||
                    contains_except(current, candidate, removed_index)) {
                    temperature = std::max(0.05, temperature * 0.999);
                    continue;
                }

                std::vector<Vertex> next = current;
                next[removed_index] = std::move(candidate);
                const std::size_t next_value =
                    external_boundary(next, alphabet_size);
                consider(next, next_value, best_value, best_set);

                const double change = static_cast<double>(current_value) -
                                      static_cast<double>(next_value);
                if (next_value <= current_value ||
                    probability(generator) < std::exp(change / temperature)) {
                    current = std::move(next);
                    current_value = next_value;
                }
                temperature = std::max(0.05, temperature * 0.999);
            }
            ++run;
        }

        const bool gate_open =
            dimension <= word_length && dimension <= free_symbols;
        std::cout << "R=" << volume << " A(" << alphabet_size << ","
                  << word_length << ") m=" << free_symbols << " d=" << dimension
                  << " gate=" << (gate_open ? "open" : "closed")
                  << " Hamming=" << hamming_formula
                  << " best found=" << best_value
                  << (best_value < static_cast<std::size_t>(hamming_formula)
                          ? "  <-- BEATS HAMMING"
                          : "")
                  << " iterations=" << iterations << " seed=" << seed
                  << " (heuristic; no-witness is not a proof)\n";

        if (best_value < static_cast<std::size_t>(hamming_formula)) {
            for (const Vertex &vertex : best_set) {
                std::cout << " (";
                for (std::size_t coordinate = 0; coordinate < vertex.size();
                     ++coordinate) {
                    if (coordinate != 0)
                        std::cout << ',';
                    std::cout << static_cast<int>(vertex[coordinate]);
                }
                std::cout << ')';
            }
            std::cout << '\n';
            std::cout << "direct boundary="
                      << external_boundary(best_set, alphabet_size) << '\n';
        }
    } catch (const std::exception &error) {
        std::cerr << "witness_finder: " << error.what() << '\n';
        return 2;
    }
    return 0;
}
