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
    static constexpr std::size_t block_size = 512;

    explicit AtomicBitset(std::size_t bits)
        : num_words_((bits + 63) / 64),
          words_(std::make_unique<std::atomic<std::uint64_t>[]>(num_words_)),
          dirty_blocks_(std::make_unique<std::atomic<std::uint8_t>[]>(
              (num_words_ + block_size - 1) / block_size)) {
        for (std::size_t index = 0; index < num_words_; ++index)
            words_[index].store(0, std::memory_order_relaxed);
        for (std::size_t index = 0; index < num_blocks(); ++index)
            dirty_blocks_[index].store(0, std::memory_order_relaxed);
    }

    bool test(std::size_t bit) const {
        return (words_[bit / 64].load(std::memory_order_relaxed) >>
                (bit % 64)) &
               1;
    }

    bool set_atomic(std::size_t bit) {
        const std::size_t word_index = bit / 64;
        const std::uint64_t mask = std::uint64_t{1} << (bit % 64);
        const std::uint64_t old =
            words_[word_index].fetch_or(mask, std::memory_order_relaxed);
        dirty_blocks_[word_index / block_size].store(1,
                                                     std::memory_order_relaxed);
        return !(old & mask);
    }

    bool set_atomic_check(std::size_t bit) {
        const std::size_t word_index = bit / 64;
        const std::uint64_t mask = std::uint64_t{1} << (bit % 64);
        const std::uint64_t old =
            words_[word_index].fetch_or(mask, std::memory_order_relaxed);
        dirty_blocks_[word_index / block_size].store(1,
                                                     std::memory_order_relaxed);
        return (old & mask) == 0;
    }

    void merge_from(const AtomicBitset &other) {
#pragma omp parallel for schedule(static)
        for (std::size_t block = 0; block < other.num_blocks(); ++block) {
            if (other.dirty_blocks_[block].load(std::memory_order_relaxed) == 0)
                continue;
            const std::size_t first = block * block_size;
            const std::size_t last = std::min(first + block_size, num_words_);
            for (std::size_t index = first; index < last; ++index) {
                const std::uint64_t bits = other.load_word(index);
                if (bits)
                    words_[index].fetch_or(bits, std::memory_order_relaxed);
            }
        }
    }

    void swap(AtomicBitset &other) {
        words_.swap(other.words_);
        dirty_blocks_.swap(other.dirty_blocks_);
    }

    void clear() {
#pragma omp parallel for schedule(static)
        for (std::size_t block = 0; block < num_blocks(); ++block) {
            if (dirty_blocks_[block].load(std::memory_order_relaxed) == 0)
                continue;
            const std::size_t first = block * block_size;
            const std::size_t last = std::min(first + block_size, num_words_);
            for (std::size_t index = first; index < last; ++index)
                words_[index].store(0, std::memory_order_relaxed);
            dirty_blocks_[block].store(0, std::memory_order_relaxed);
        }
    }

    std::size_t num_words() const { return num_words_; }

    std::size_t num_blocks() const {
        return (num_words_ + block_size - 1) / block_size;
    }

    bool block_dirty(std::size_t block) const {
        return dirty_blocks_[block].load(std::memory_order_relaxed) != 0;
    }

    std::uint64_t load_word(std::size_t index) const {
        return words_[index].load(std::memory_order_relaxed);
    }

  private:
    std::size_t num_words_;
    std::unique_ptr<std::atomic<std::uint64_t>[]> words_;
    std::unique_ptr<std::atomic<std::uint8_t>[]> dirty_blocks_;
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
                           const std::vector<packed_code_t> &star) {
    if (star.empty())
        return true;

    std::vector<bool> seen(star.size(), false);
    std::vector<std::size_t> pending{0};
    seen[0] = true;
    std::size_t reached = 1;
    while (!pending.empty()) {
        const packed_code_t current = star[pending.back()];
        pending.pop_back();
        graph.for_each_neighbor(current, [&](const packed_code_t neighbor) {
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
