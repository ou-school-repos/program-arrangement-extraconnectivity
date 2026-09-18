#ifndef STAR_SWEEP_FRONTIER_DELTA_HPP
#define STAR_SWEEP_FRONTIER_DELTA_HPP

#include "bfs_utils.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace star_sweep {

inline void durable_delta_file(const std::string &path) {
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0 || fsync(fd) != 0) {
        const std::string error = std::strerror(errno);
        if (fd >= 0)
            close(fd);
        throw std::runtime_error("cannot fsync frontier delta: " + error);
    }
    close(fd);

    const std::size_t separator = path.find_last_of('/');
    const std::string directory =
        separator == std::string::npos ? "." : path.substr(0, separator);
    const int directory_fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY);
    if (directory_fd < 0 || fsync(directory_fd) != 0) {
        const std::string error = std::strerror(errno);
        if (directory_fd >= 0)
            close(directory_fd);
        throw std::runtime_error("cannot fsync frontier directory: " + error);
    }
    close(directory_fd);
}

struct DeltaHeader {
    static constexpr std::uint64_t magic = 0x535344454c544131ULL;
    std::uint64_t magic_value = magic;
    std::uint32_t version = 1;
    std::uint32_t reserved = 0;
    std::uint64_t block_words = AtomicBitset::block_size;
    std::uint64_t num_words = 0;
    std::uint64_t block_count = 0;
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
    std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
    if (!output)
        throw std::runtime_error("cannot create frontier delta: " + path);

    DeltaHeader header;
    header.num_words = frontier.num_words();
    for (std::size_t block = 0; block < frontier.num_blocks(); ++block) {
        if (frontier.block_dirty(block))
            ++header.block_count;
    }
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));

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
        for (std::size_t word = first; word < last; ++word) {
            const std::uint64_t value = frontier.load_word(word);
            output.write(reinterpret_cast<const char *>(&value), sizeof(value));
        }
        ++written_blocks;
    }

    if (!output)
        throw std::runtime_error("cannot finish frontier delta: " + path);
    output.close();
    if (written_blocks != header.block_count)
        throw std::runtime_error("frontier delta changed while being written");

    durable_delta_file(temporary_path);
    if (rename(temporary_path.c_str(), path.c_str()) != 0)
        throw std::runtime_error("cannot publish frontier delta: " +
                                 std::string(std::strerror(errno)));
    durable_delta_file(path);
    return written_blocks;
}

} // namespace star_sweep

#endif
