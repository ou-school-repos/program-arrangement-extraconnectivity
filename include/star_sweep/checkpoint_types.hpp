#ifndef STAR_SWEEP_CHECKPOINT_TYPES_HPP
#define STAR_SWEEP_CHECKPOINT_TYPES_HPP

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace star_sweep {

inline constexpr std::uint64_t checkpoint_metadata_version = 1;

inline void durable_fsync_path(const std::string &path) {
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0 || fsync(fd) != 0) {
        const std::string error = std::strerror(errno);
        if (fd >= 0)
            close(fd);
        throw std::runtime_error("cannot fsync " + path + ": " + error);
    }
    close(fd);
}

inline void durable_fsync_directory(const std::string &path) {
    const int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0 || fsync(fd) != 0) {
        const std::string error = std::strerror(errno);
        if (fd >= 0)
            close(fd);
        throw std::runtime_error("cannot fsync directory " + path + ": " +
                                 error);
    }
    close(fd);
}

struct CheckpointSignature {
    int n = 0;
    int k = 0;
    std::uint64_t valid_count = 0;
    std::uint64_t bitmap_bytes = 0;
};

enum class CheckpointPhase : int {
    LayerBoundary = 0,
    ComponentComplete = 1,
};

struct CheckpointState {
    CheckpointSignature signature;
    std::uint64_t generation = 0;
    CheckpointPhase phase = CheckpointPhase::LayerBoundary;
    std::uint64_t layer = 0;
    std::uint64_t component_anchor = 0;
    std::uint64_t component_size = 0;
    std::uint64_t discovered_survivors = 0;
    std::uint64_t star_boundary_size = 0;
    std::uint64_t active_frontier_size = 0;
    std::vector<std::uint64_t> component_sizes;
    std::vector<bool> direction_history;

    void validate() const {
        if (signature.n <= 0 || signature.k <= 0 ||
            signature.valid_count == 0 || signature.bitmap_bytes == 0)
            throw std::invalid_argument(
                "invalid star-sweep checkpoint signature");
        if (phase != CheckpointPhase::LayerBoundary &&
            phase != CheckpointPhase::ComponentComplete)
            throw std::invalid_argument("invalid star-sweep checkpoint phase");
        if (component_sizes.empty() || direction_history.size() != layer)
            throw std::invalid_argument(
                "invalid star-sweep checkpoint history");
        for (const std::uint64_t size : component_sizes) {
            if (size == 0 || size > signature.valid_count)
                throw std::invalid_argument(
                    "invalid checkpoint component size");
        }
        if (component_anchor >= signature.valid_count)
            throw std::invalid_argument(
                "checkpoint component anchor out of range");
        if (component_size > signature.valid_count)
            throw std::invalid_argument(
                "checkpoint component size exceeds valid count");
        if (discovered_survivors > signature.valid_count)
            throw std::invalid_argument(
                "checkpoint discovered survivors exceeds valid count");
        if (star_boundary_size > signature.valid_count)
            throw std::invalid_argument(
                "checkpoint star boundary exceeds valid count");
        if (active_frontier_size > signature.valid_count)
            throw std::invalid_argument(
                "checkpoint active frontier size exceeds valid count");
        if (phase == CheckpointPhase::ComponentComplete &&
            active_frontier_size != 0)
            throw std::invalid_argument(
                "completed component must have zero active frontier");
        if (phase == CheckpointPhase::LayerBoundary &&
            active_frontier_size == 0)
            throw std::invalid_argument(
                "layer boundary must have non-zero active frontier");
    }
};

inline bool signature_matches(const CheckpointSignature &expected,
                              const CheckpointSignature &actual) {
    return expected.n == actual.n && expected.k == actual.k &&
           expected.valid_count == actual.valid_count &&
           expected.bitmap_bytes == actual.bitmap_bytes;
}

inline std::string generation_name(std::uint64_t generation) {
    std::ostringstream name;
    name << "gen-" << std::setw(5) << std::setfill('0') << generation;
    return name.str();
}

} // namespace star_sweep

#endif
