#ifndef STAR_SWEEP_CHECKPOINT_TYPES_HPP
#define STAR_SWEEP_CHECKPOINT_TYPES_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace star_sweep {

struct CheckpointSignature {
    int n = 0;
    int k = 0;
    std::uint64_t valid_count = 0;
    std::uint64_t bitmap_bytes = 0;
};

struct CheckpointState {
    CheckpointSignature signature;
    std::uint64_t generation = 0;
    std::uint64_t layer = 0;
    std::uint64_t component_anchor = 0;
    std::uint64_t component_size = 0;
    std::uint64_t discovered_survivors = 0;
    std::uint64_t boundary_size = 0;

    void validate() const {
        if (signature.n <= 0 || signature.k <= 0 ||
            signature.valid_count == 0 || signature.bitmap_bytes == 0)
            throw std::invalid_argument(
                "invalid star-sweep checkpoint signature");
    }
};

inline bool signature_matches(const CheckpointSignature &expected,
                              const CheckpointSignature &actual) {
    return expected.n == actual.n && expected.k == actual.k &&
           expected.valid_count == actual.valid_count &&
           expected.bitmap_bytes == actual.bitmap_bytes;
}

inline std::string generation_name(std::uint64_t generation) {
    return "gen-" + std::to_string(generation);
}

} // namespace star_sweep

#endif
