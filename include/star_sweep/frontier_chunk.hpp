#ifndef STAR_SWEEP_FRONTIER_CHUNK_HPP
#define STAR_SWEEP_FRONTIER_CHUNK_HPP

#include "frontier_delta.hpp"
#include "partition_types.hpp"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace star_sweep {

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
    std::ofstream output(temporary_path, std::ios::trunc);
    if (!output)
        throw std::runtime_error("cannot create partition metadata: " + path);
    output << partition_chunk_metadata_version << ' ' << state.signature.n
           << ' ' << state.signature.k << ' ' << state.signature.valid_count
           << ' ' << state.signature.bitmap_bytes << '\n'
           << state.base_generation << ' ' << state.target_generation << ' '
           << state.layer << ' ' << state.partition_index << ' '
           << state.partition_count << ' ' << state.first_word << ' '
           << state.last_word << ' ' << state.first_block << ' '
           << state.last_block << ' ' << state.local_frontier_size << ' '
           << (state.bottom_up ? 1 : 0) << '\n';
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
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("cannot open partition metadata: " + path);
    std::uint64_t version = 0;
    PartitionChunkState state;
    int bottom_up = 0;
    input >> version >> state.signature.n >> state.signature.k >>
        state.signature.valid_count >> state.signature.bitmap_bytes >>
        state.base_generation >> state.target_generation >> state.layer >>
        state.partition_index >> state.partition_count >> state.first_word >>
        state.last_word >> state.first_block >> state.last_block >>
        state.local_frontier_size >> bottom_up;
    if (!input || version != partition_chunk_metadata_version ||
        (bottom_up != 0 && bottom_up != 1))
        throw std::runtime_error("invalid partition metadata: " + path);
    state.bottom_up = bottom_up != 0;
    state.validate(expected);
    return state;
}

} // namespace star_sweep

#endif
