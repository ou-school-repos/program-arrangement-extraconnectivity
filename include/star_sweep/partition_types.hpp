#ifndef STAR_SWEEP_PARTITION_TYPES_HPP
#define STAR_SWEEP_PARTITION_TYPES_HPP

#include "checkpoint_types.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace star_sweep {

inline constexpr std::uint64_t partition_chunk_metadata_version = 1;

struct PartitionSpec {
    std::size_t index = 0;
    std::size_t count = 1;

    void validate() const {
        if (count == 0 || index >= count)
            throw std::invalid_argument("invalid partition specification");
    }

    std::pair<std::size_t, std::size_t> range(std::size_t total_items) const {
        validate();
        const std::size_t items_per_partition =
            (total_items + count - 1) / count;
        const std::size_t first = index * items_per_partition;
        return {first, std::min(total_items, first + items_per_partition)};
    }
};

struct PartitionChunkState {
    CheckpointSignature signature;
    std::uint64_t base_generation = 0;
    std::uint64_t target_generation = 0;
    std::uint64_t layer = 0;
    std::uint64_t partition_index = 0;
    std::uint64_t partition_count = 0;
    std::uint64_t first_word = 0;
    std::uint64_t last_word = 0;
    std::uint64_t first_block = 0;
    std::uint64_t last_block = 0;
    std::uint64_t local_frontier_size = 0;
    bool bottom_up = false;

    void validate(const CheckpointSignature &expected) const {
        if (!signature_matches(expected, signature))
            throw std::invalid_argument("partition chunk signature mismatch");
        if (partition_count == 0 || partition_index >= partition_count ||
            first_block > last_block)
            throw std::invalid_argument("invalid partition chunk metadata");
        if (target_generation != base_generation + 1)
            throw std::invalid_argument("invalid partition chunk generation");
    }
};

} // namespace star_sweep

#endif
