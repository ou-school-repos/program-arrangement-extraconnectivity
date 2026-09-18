#ifndef BFS_UTILS_HPP
#define BFS_UTILS_HPP

#include "arrangement_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

class AtomicBitset {
  public:
    explicit AtomicBitset(std::size_t bits)
        : num_words_((bits + 63) / 64),
          words_(std::make_unique<std::atomic<std::uint64_t>[]>(num_words_)) {
        for (std::size_t index = 0; index < num_words_; ++index)
            words_[index].store(0, std::memory_order_relaxed);
    }

    bool test(std::size_t bit) const {
        return (words_[bit / 64].load(std::memory_order_relaxed) >>
                (bit % 64)) &
               1;
    }

    bool set_atomic(std::size_t bit) {
        const std::uint64_t mask = std::uint64_t{1} << (bit % 64);
        const std::uint64_t old =
            words_[bit / 64].fetch_or(mask, std::memory_order_relaxed);
        return !(old & mask);
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

inline void report_bfs_progress(std::uint64_t discovered,
                                std::uint64_t total_survivors) {
    const double percent =
        total_survivors == 0 ? 100.0 : 100.0 * discovered / total_survivors;
    std::cerr << "\rBFS progress: " << discovered << " / " << total_survivors
              << " (" << std::fixed << std::setprecision(1) << percent << "%)"
              << std::flush;
}

inline bool star_connected(const PackedArrangementGraph &graph,
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

#endif
