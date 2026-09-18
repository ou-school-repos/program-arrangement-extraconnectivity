#ifndef BFS_UTILS_HPP
#define BFS_UTILS_HPP

#include "arrangement_utils.hpp"
#include "build_info.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

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

    AtomicBitset(std::size_t bits, const std::string &path)
        : num_words_((bits + 63) / 64),
          dirty_blocks_(std::make_unique<std::atomic<std::uint8_t>[]>(
              (num_words_ + block_size - 1) / block_size)),
          mapped_bytes_(num_words_ * sizeof(std::uint64_t)),
          mapped_path_(path) {
        mapped_fd_ = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (mapped_fd_ < 0)
            throw std::runtime_error("cannot open bitmap file " + path + ": " +
                                     std::strerror(errno));
        if (ftruncate(mapped_fd_, static_cast<off_t>(mapped_bytes_)) != 0) {
            const std::string message =
                "cannot size bitmap file " + path + ": " + std::strerror(errno);
            close(mapped_fd_);
            mapped_fd_ = -1;
            throw std::runtime_error(message);
        }
        mapped_words_ = static_cast<std::uint64_t *>(
            mmap(nullptr, mapped_bytes_, PROT_READ | PROT_WRITE, MAP_SHARED,
                 mapped_fd_, 0));
        if (mapped_words_ == MAP_FAILED) {
            const std::string message =
                "cannot mmap bitmap file " + path + ": " + std::strerror(errno);
            mapped_words_ = nullptr;
            close(mapped_fd_);
            mapped_fd_ = -1;
            throw std::runtime_error(message);
        }
        madvise(mapped_words_, mapped_bytes_, MADV_SEQUENTIAL);
        for (std::size_t index = 0; index < num_blocks(); ++index)
            dirty_blocks_[index].store(0, std::memory_order_relaxed);
    }

    ~AtomicBitset() {
        if (mapped_words_ != nullptr)
            munmap(mapped_words_, mapped_bytes_);
        if (mapped_fd_ >= 0)
            close(mapped_fd_);
    }

    AtomicBitset(const AtomicBitset &) = delete;
    AtomicBitset &operator=(const AtomicBitset &) = delete;

    bool test(std::size_t bit) const {
        return (load_word(bit / 64) >> (bit % 64)) & 1;
    }

    bool set_atomic(std::size_t bit) {
        const std::size_t word_index = bit / 64;
        const std::uint64_t mask = std::uint64_t{1} << (bit % 64);
        const std::uint64_t old = fetch_or_word(word_index, mask);
        dirty_blocks_[word_index / block_size].store(1,
                                                     std::memory_order_relaxed);
        return !(old & mask);
    }

    bool set_atomic_check(std::size_t bit) {
        const std::size_t word_index = bit / 64;
        const std::uint64_t mask = std::uint64_t{1} << (bit % 64);
        const std::uint64_t old = fetch_or_word(word_index, mask);
        dirty_blocks_[word_index / block_size].store(1,
                                                     std::memory_order_relaxed);
        return (old & mask) == 0;
    }

    void set_word_atomic(std::size_t word_index, std::uint64_t mask) {
        fetch_or_word(word_index, mask);
        dirty_blocks_[word_index / block_size].store(1,
                                                     std::memory_order_relaxed);
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
                    fetch_or_word(index, bits);
            }
        }
    }

    void swap(AtomicBitset &other) {
        std::swap(mapped_words_, other.mapped_words_);
        std::swap(mapped_fd_, other.mapped_fd_);
        std::swap(mapped_bytes_, other.mapped_bytes_);
        std::swap(mapped_path_, other.mapped_path_);
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
                store_word(index, 0);
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
        if (mapped_words_ != nullptr)
            return __atomic_load_n(mapped_words_ + index, __ATOMIC_RELAXED);
        return words_[index].load(std::memory_order_relaxed);
    }

  private:
    std::uint64_t fetch_or_word(std::size_t index, std::uint64_t mask) {
        if (mapped_words_ != nullptr)
            return __atomic_fetch_or(mapped_words_ + index, mask,
                                     __ATOMIC_RELAXED);
        return words_[index].fetch_or(mask, std::memory_order_relaxed);
    }

    void store_word(std::size_t index, std::uint64_t value) {
        if (mapped_words_ != nullptr) {
            __atomic_store_n(mapped_words_ + index, value, __ATOMIC_RELAXED);
            return;
        }
        words_[index].store(value, std::memory_order_relaxed);
    }

    std::size_t num_words_;
    std::unique_ptr<std::atomic<std::uint64_t>[]> words_;
    std::unique_ptr<std::atomic<std::uint8_t>[]> dirty_blocks_;
    std::uint64_t *mapped_words_ = nullptr;
    int mapped_fd_ = -1;
    std::size_t mapped_bytes_ = 0;
    std::string mapped_path_;
};

struct alignas(64) PaddedScanCounter {
    std::atomic<std::size_t> value{0};
};

inline void report_bfs_progress(std::uint64_t discovered,
                                std::uint64_t total_survivors) {
    const double percent =
        total_survivors == 0 ? 100.0 : 100.0 * discovered / total_survivors;
    std::cerr << "\rBFS progress: " << discovered << " / " << total_survivors
              << " (" << std::fixed << std::setprecision(1) << percent << "%)"
              << std::flush;
}

inline void report_bfs_scan_progress(std::size_t scanned,
                                     std::size_t total_words) {
    const double percent =
        total_words == 0 ? 100.0 : 100.0 * scanned / total_words;
    std::cerr << "\rBottom-up scan: " << scanned << " / " << total_words
              << " words (" << std::fixed << std::setprecision(1) << percent
              << "%)" << std::flush;
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
