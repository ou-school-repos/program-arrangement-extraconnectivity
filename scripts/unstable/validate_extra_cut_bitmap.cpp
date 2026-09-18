// Serial bitmap-frontier experiment for A(n,k).
// The trusted flat and ranked validators remain separate references.
// Usage: validate_extra_cut_bitmap n k

#include "arrangement_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

class AtomicBitset {
  public:
    explicit AtomicBitset(std::size_t bits)
        : num_words_((bits + 63) / 64),
          words_(std::make_unique<std::atomic<std::uint64_t>[]>(num_words_)) {
        for (std::size_t i = 0; i < num_words_; ++i)
            words_[i].store(0, std::memory_order_relaxed);
    }

    bool test(std::size_t bit) const {
        return (words_[bit / 64].load(std::memory_order_relaxed) >>
                (bit % 64)) &
               1;
    }

    void set_atomic(std::size_t bit) {
        const std::uint64_t mask = std::uint64_t{1} << (bit % 64);
        words_[bit / 64].fetch_or(mask, std::memory_order_relaxed);
    }

    void merge_from(const AtomicBitset &other) {
        for (std::size_t index = 0; index < num_words_; ++index) {
            const std::uint64_t bits = other.load_word(index);
            if (bits)
                words_[index].fetch_or(bits, std::memory_order_relaxed);
        }
    }

    void swap(AtomicBitset &other) { words_.swap(other.words_); }

    void clear() {
        for (std::size_t index = 0; index < num_words_; ++index)
            words_[index].store(0, std::memory_order_relaxed);
    }

    std::size_t num_words() const { return num_words_; }

    std::uint64_t load_word(std::size_t index) const {
        return words_[index].load(std::memory_order_relaxed);
    }

  private:
    std::size_t num_words_;
    std::unique_ptr<std::atomic<std::uint64_t>[]> words_;
};

static bool star_connected(const PackedArrangementGraph &graph,
                           const std::vector<std::uint64_t> &star) {
    if (star.empty())
        return true;

    std::vector<bool> seen(star.size(), false);
    std::vector<std::size_t> pending{0};
    seen[0] = true;
    std::size_t reached = 1;
    while (!pending.empty()) {
        const std::uint64_t current = star[pending.back()];
        pending.pop_back();
        graph.for_each_neighbor(current, [&](const std::uint64_t neighbor) {
            const auto match = std::find(star.begin(), star.end(), neighbor);
            if (match == star.end())
                return;
            const std::size_t index =
                static_cast<std::size_t>(match - star.begin());
            if (!seen[index]) {
                seen[index] = true;
                pending.push_back(index);
                ++reached;
            }
        });
    }
    return reached == star.size();
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " n k\n";
        return 1;
    }
    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    if (n <= k || k < 1 || n > 64 || k > 10) {
        std::cerr << "Error: require 64 >= n > k >= 1 and k <= 10.\n";
        return 1;
    }

    const PackedArrangementGraph graph(n, k);
    const FullStarParameters parameters = full_star_parameters(n, k);

    std::vector<int> center(k);
    std::iota(center.begin(), center.end(), 0);
    const std::uint64_t center_code = graph.encode(center);
    std::vector<std::uint64_t> star{center_code};
    for (int position = 0; position < k; ++position) {
        for (int symbol = k; symbol < n; ++symbol) {
            const std::uint64_t mask = ~(std::uint64_t{63} << (6 * position));
            star.push_back(
                (center_code & mask) |
                (static_cast<std::uint64_t>(symbol) << (6 * position)));
        }
    }

    if (!star_connected(graph, star)) {
        std::cerr << "Error: constructed Star is not connected.\n";
        return 1;
    }

    AtomicBitset visited(graph.valid_count);
    for (const std::uint64_t code : star)
        visited.set_atomic(graph.rank_code(code));

    std::vector<std::uint64_t> boundary;
    for (const std::uint64_t code : star) {
        graph.for_each_neighbor(code, [&](const std::uint64_t neighbor) {
            const std::size_t rank = graph.rank_code(neighbor);
            if (!visited.test(rank)) {
                visited.set_atomic(rank);
                boundary.push_back(neighbor);
            }
        });
    }

    AtomicBitset current_frontier(graph.valid_count);
    AtomicBitset next_frontier(graph.valid_count);
    std::vector<std::uint64_t> component_sizes{star.size()};

    graph.for_each_valid_code([&](const std::uint64_t start) {
        const std::size_t start_rank = graph.rank_code(start);
        if (visited.test(start_rank))
            return;

        std::uint64_t size = 0;
        current_frontier.set_atomic(start_rank);
        visited.set_atomic(start_rank);
        while (true) {
            std::uint64_t level_size = 0;
            for (std::size_t word_index = 0;
                 word_index < current_frontier.num_words(); ++word_index) {
                std::uint64_t word = current_frontier.load_word(word_index);
                while (word != 0) {
                    const int bit = __builtin_ctzll(word);
                    word &= word - 1;
                    const std::size_t rank = word_index * 64 + bit;
                    ++level_size;
                    const std::uint64_t code = graph.decode_rank(rank);
                    graph.for_each_neighbor(
                        code, [&](const std::uint64_t neighbor) {
                            const std::size_t neighbor_rank =
                                graph.rank_code(neighbor);
                            if (!visited.test(neighbor_rank))
                                next_frontier.set_atomic(neighbor_rank);
                        });
                }
            }

            if (level_size == 0)
                break;
            size += level_size;
            visited.merge_from(next_frontier);
            current_frontier.swap(next_frontier);
            next_frontier.clear();
        }
        component_sizes.push_back(size);
    });

    std::sort(component_sizes.begin(), component_sizes.end());
    const bool valid = component_sizes.size() >= 2 &&
                       std::all_of(component_sizes.begin(),
                                   component_sizes.end(), [&](const auto size) {
                                       return size > static_cast<std::uint64_t>(
                                                         parameters.g());
                                   });
    std::cout << "A(" << n << ',' << k << ") bitmap validation\n"
              << "R = " << parameters.volume() << ", g = " << parameters.g()
              << ", m = " << parameters.m() << ", d = " << parameters.d()
              << "\n"
              << "|N(S)| = " << boundary.size() << " (formula "
              << parameters.boundary() << ")\n"
              << "Hamming baseline = " << parameters.hamming_boundary()
              << ", embedding gate = "
              << (parameters.embedding_gate() ? "open" : "closed") << "\n"
              << "component sizes after deletion:\n";
    for (const std::uint64_t size : component_sizes)
        std::cout << "  " << size << '\n';
    std::cout << "valid " << parameters.g()
              << "-extra cut: " << (valid ? "yes" : "no") << '\n';
    return valid ? 0 : 1;
}
