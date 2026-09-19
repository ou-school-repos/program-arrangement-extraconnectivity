#ifndef STAR_SWEEP_FRONTIER_CHUNK_HPP
#define STAR_SWEEP_FRONTIER_CHUNK_HPP

#include "frontier_delta.hpp"
#include "partition_types.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace star_sweep {

struct PartitionChunkHeader {
    static constexpr std::uint64_t magic = 0x53534348554e4b31ULL;

    std::uint64_t magic_value = magic;
    std::uint64_t version = partition_chunk_metadata_version;
    std::int32_t n = 0;
    std::int32_t k = 0;
    std::uint64_t valid_count = 0;
    std::uint64_t bitmap_bytes = 0;
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
    std::uint8_t bottom_up = 0;
    std::uint8_t reserved[7] = {};
    std::uint32_t metadata_crc32c = 0;
};

static_assert(std::is_trivially_copyable_v<PartitionChunkHeader>);

inline std::uint64_t write_frontier_delta(const AtomicBitset &frontier,
                                          const std::string &path,
                                          std::size_t first_block,
                                          std::size_t last_block) {
    if (first_block > last_block || last_block > frontier.num_blocks())
        throw std::invalid_argument("invalid frontier delta block range");

    const std::string temporary_path = path + ".tmp";
    std::fstream output(temporary_path, std::ios::binary | std::ios::in |
                                            std::ios::out | std::ios::trunc);
    if (!output)
        throw std::runtime_error("cannot create frontier delta: " + path);

    DeltaHeader header;
    header.num_words = frontier.num_words();
    for (std::size_t block = first_block; block < last_block; ++block)
        if (frontier.block_dirty(block))
            ++header.block_count;
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));

    Xxh64 checksum;
    std::uint64_t written_blocks = 0;
    for (std::size_t block = first_block; block < last_block; ++block) {
        if (!frontier.block_dirty(block))
            continue;
        const std::size_t first = block * AtomicBitset::block_size;
        const std::size_t last =
            std::min(first + AtomicBitset::block_size, frontier.num_words());
        const DeltaBlockHeader block_header{
            static_cast<std::uint64_t>(block),
            static_cast<std::uint64_t>(last - first)};
        output.write(reinterpret_cast<const char *>(&block_header),
                     sizeof(block_header));
        checksum.update(&block_header, sizeof(block_header));
        for (std::size_t word = first; word < last; ++word) {
            const std::uint64_t value = frontier.load_word(word);
            output.write(reinterpret_cast<const char *>(&value), sizeof(value));
            checksum.update(&value, sizeof(value));
        }
        ++written_blocks;
    }
    if (!output || written_blocks != header.block_count)
        throw std::runtime_error("cannot finish frontier delta: " + path);
    output.close();

    header.payload_checksum = checksum.digest();
    output.open(temporary_path,
                std::ios::binary | std::ios::in | std::ios::out);
    if (!output)
        throw std::runtime_error("cannot reopen frontier delta: " + path);
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));
    output.close();

    durable_fsync_path(temporary_path);
    if (rename(temporary_path.c_str(), path.c_str()) != 0)
        throw std::runtime_error("cannot publish frontier delta: " +
                                 std::string(std::strerror(errno)));
    const std::filesystem::path file_path(path);
    const std::filesystem::path directory = file_path.parent_path().empty()
                                                ? std::filesystem::path(".")
                                                : file_path.parent_path();
    durable_fsync_directory(directory.string());
    return written_blocks;
}

inline void write_partition_chunk_metadata(const std::string &path,
                                           const PartitionChunkState &state) {
    const std::string temporary_path = path + ".tmp";
    PartitionChunkHeader header{};
    header.n = state.signature.n;
    header.k = state.signature.k;
    header.valid_count = state.signature.valid_count;
    header.bitmap_bytes = state.signature.bitmap_bytes;
    header.base_generation = state.base_generation;
    header.target_generation = state.target_generation;
    header.layer = state.layer;
    header.partition_index = state.partition_index;
    header.partition_count = state.partition_count;
    header.first_word = state.first_word;
    header.last_word = state.last_word;
    header.first_block = state.first_block;
    header.last_block = state.last_block;
    header.local_frontier_size = state.local_frontier_size;
    header.bottom_up = state.bottom_up ? 1 : 0;
    std::fill(std::begin(header.reserved), std::end(header.reserved), 0);
    Crc32c checksum;
    checksum.update(&header, offsetof(PartitionChunkHeader, metadata_crc32c));
    header.metadata_crc32c = checksum.digest();

    std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
    if (!output)
        throw std::runtime_error("cannot create partition metadata: " + path);
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));
    if (!output)
        throw std::runtime_error("cannot finish partition metadata: " + path);
    output.close();
    durable_fsync_path(temporary_path);
    if (rename(temporary_path.c_str(), path.c_str()) != 0)
        throw std::runtime_error("cannot publish partition metadata: " +
                                 std::string(std::strerror(errno)));
    const std::filesystem::path file_path(path);
    const std::filesystem::path directory = file_path.parent_path().empty()
                                                ? std::filesystem::path(".")
                                                : file_path.parent_path();
    durable_fsync_directory(directory.string());
}

inline PartitionChunkState
read_partition_chunk_metadata(const std::string &path,
                              const CheckpointSignature &expected) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open partition metadata: " + path);
    PartitionChunkHeader header;
    if (!input.read(reinterpret_cast<char *>(&header), sizeof(header)))
        throw std::runtime_error("invalid partition metadata: " + path);
    Crc32c checksum;
    checksum.update(&header, offsetof(PartitionChunkHeader, metadata_crc32c));
    if (header.magic_value != PartitionChunkHeader::magic ||
        header.version != partition_chunk_metadata_version ||
        std::any_of(std::begin(header.reserved), std::end(header.reserved),
                    [](const std::uint8_t value) { return value != 0; }) ||
        checksum.digest() != header.metadata_crc32c)
        throw std::runtime_error("partition metadata checksum mismatch: " +
                                 path);

    PartitionChunkState state;
    state.signature.n = header.n;
    state.signature.k = header.k;
    state.signature.valid_count = header.valid_count;
    state.signature.bitmap_bytes = header.bitmap_bytes;
    state.base_generation = header.base_generation;
    state.target_generation = header.target_generation;
    state.layer = header.layer;
    state.partition_index = header.partition_index;
    state.partition_count = header.partition_count;
    state.first_word = header.first_word;
    state.last_word = header.last_word;
    state.first_block = header.first_block;
    state.last_block = header.last_block;
    state.local_frontier_size = header.local_frontier_size;
    if (header.bottom_up > 1)
        throw std::runtime_error("invalid partition direction: " + path);
    state.bottom_up = header.bottom_up != 0;
    state.validate(expected);
    return state;
}

} // namespace star_sweep

#endif
