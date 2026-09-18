#ifndef STAR_SWEEP_FRONTIER_DELTA_HPP
#define STAR_SWEEP_FRONTIER_DELTA_HPP

#include "bfs_utils.hpp"
#include "checkpoint_types.hpp"
#include "checksum.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace star_sweep {

struct DeltaHeader {
    static constexpr std::uint64_t magic = 0x535344454c544131ULL;
    std::uint64_t magic_value = magic;
    std::uint32_t version = 1;
    std::uint32_t reserved = 0;
    std::uint64_t block_words = AtomicBitset::block_size;
    std::uint64_t num_words = 0;
    std::uint64_t block_count = 0;
    std::uint64_t payload_checksum = 0;
};

struct DeltaBlockHeader {
    std::uint64_t block_index = 0;
    std::uint64_t word_count = 0;
};

// Writes a self-describing immutable delta. The caller must invoke this only
// after the producer frontier has reached a synchronization point.
inline std::uint64_t write_frontier_delta(const AtomicBitset &frontier,
                                          const std::string &path) {
    const std::string temporary_path = path + ".tmp";
    std::fstream output(temporary_path, std::ios::binary | std::ios::in |
                                            std::ios::out | std::ios::trunc);
    if (!output)
        throw std::runtime_error("cannot create frontier delta: " + path);

    DeltaHeader header;
    header.num_words = frontier.num_words();
    for (std::size_t block = 0; block < frontier.num_blocks(); ++block) {
        if (frontier.block_dirty(block))
            ++header.block_count;
    }
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));

    Xxh64 checksum;
    std::uint64_t written_blocks = 0;
    for (std::size_t block = 0; block < frontier.num_blocks(); ++block) {
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

    if (!output)
        throw std::runtime_error("cannot finish frontier delta: " + path);
    output.close();
    if (written_blocks != header.block_count)
        throw std::runtime_error("frontier delta changed while being written");

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

    namespace fs = std::filesystem;
    const fs::path file_path(path);
    const fs::path directory = file_path.parent_path().empty()
                                   ? fs::path(".")
                                   : file_path.parent_path();
    durable_fsync_directory(directory.string());
    return written_blocks;
}

// Replays one immutable frontier delta into a fresh working bitmap.
inline void replay_frontier_delta(const std::string &path,
                                  AtomicBitset &frontier,
                                  std::uint64_t *payload_bits = nullptr) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open frontier delta for replay: " +
                                 path);

    DeltaHeader header;
    if (!input.read(reinterpret_cast<char *>(&header), sizeof(header)))
        throw std::runtime_error("failed to read delta header: " + path);
    if (header.magic_value != DeltaHeader::magic || header.version != 1 ||
        header.block_words != AtomicBitset::block_size)
        throw std::runtime_error("invalid frontier delta header: " + path);
    if (header.num_words != frontier.num_words())
        throw std::runtime_error("frontier delta word-count mismatch: " + path);
    if (header.block_count > frontier.num_blocks())
        throw std::runtime_error("frontier delta block-count mismatch: " +
                                 path);

    Xxh64 checksum;
    std::size_t previous_block = 0;
    bool have_previous_block = false;
    for (std::uint64_t index = 0; index < header.block_count; ++index) {
        DeltaBlockHeader block;
        if (!input.read(reinterpret_cast<char *>(&block), sizeof(block)))
            throw std::runtime_error("failed to read delta block header: " +
                                     path);
        if (block.block_index >= frontier.num_blocks() ||
            (have_previous_block && block.block_index <= previous_block))
            throw std::runtime_error("invalid delta block index: " + path);
        checksum.update(&block, sizeof(block));

        const std::size_t first = static_cast<std::size_t>(block.block_index) *
                                  AtomicBitset::block_size;
        const std::size_t available =
            std::min(AtomicBitset::block_size, frontier.num_words() - first);
        if (block.word_count > available)
            throw std::runtime_error("invalid delta block length: " + path);

        for (std::uint64_t word = 0; word < block.word_count; ++word) {
            std::uint64_t value = 0;
            if (!input.read(reinterpret_cast<char *>(&value), sizeof(value)))
                throw std::runtime_error("failed to read delta block data: " +
                                         path);
            checksum.update(&value, sizeof(value));
            if (payload_bits != nullptr)
                *payload_bits += __builtin_popcountll(value);
        }
        previous_block = static_cast<std::size_t>(block.block_index);
        have_previous_block = true;
    }

    if (checksum.digest() != header.payload_checksum)
        throw std::runtime_error("frontier delta checksum mismatch: " + path);

    input.clear();
    input.seekg(static_cast<std::streamoff>(sizeof(DeltaHeader)),
                std::ios::beg);
    for (std::uint64_t index = 0; index < header.block_count; ++index) {
        DeltaBlockHeader block;
        if (!input.read(reinterpret_cast<char *>(&block), sizeof(block)))
            throw std::runtime_error("failed to reread delta block header: " +
                                     path);
        const std::size_t first = static_cast<std::size_t>(block.block_index) *
                                  AtomicBitset::block_size;
        for (std::uint64_t word = 0; word < block.word_count; ++word) {
            std::uint64_t value = 0;
            if (!input.read(reinterpret_cast<char *>(&value), sizeof(value)))
                throw std::runtime_error("failed to reread delta block data: " +
                                         path);
            if (value != 0)
                frontier.set_word_atomic(first + static_cast<std::size_t>(word),
                                         value);
        }
    }
}

} // namespace star_sweep

#endif
