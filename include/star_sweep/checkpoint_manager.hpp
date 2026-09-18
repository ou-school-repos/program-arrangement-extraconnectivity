#ifndef STAR_SWEEP_CHECKPOINT_MANAGER_HPP
#define STAR_SWEEP_CHECKPOINT_MANAGER_HPP

#include "checkpoint_types.hpp"
#include "frontier_delta.hpp"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>

namespace star_sweep {

class CheckpointManager {
  public:
    explicit CheckpointManager(std::string directory)
        : directory_(std::move(directory)) {}

    std::string generation_directory(std::uint64_t generation) const {
        return directory_ + "/" + generation_name(generation);
    }

    std::uint64_t current_generation() const {
        const std::string current = directory_ + "/CURRENT";
        char target[128];
        const ssize_t length =
            readlink(current.c_str(), target, sizeof(target) - 1);
        if (length < 0)
            throw std::runtime_error("cannot read checkpoint CURRENT");
        target[length] = '\0';
        const std::string name(target);
        const std::string prefix = "gen-";
        if (name.rfind(prefix, 0) != 0 || name.size() == prefix.size())
            throw std::runtime_error("invalid checkpoint CURRENT target");
        try {
            std::size_t consumed = 0;
            const std::uint64_t generation =
                std::stoull(name.substr(prefix.size()), &consumed);
            if (consumed != name.size() - prefix.size())
                throw std::runtime_error("invalid checkpoint generation");
            return generation;
        } catch (const std::exception &) {
            throw std::runtime_error("invalid checkpoint CURRENT target");
        }
    }

    CheckpointState read_state(std::uint64_t generation,
                               const CheckpointSignature &expected,
                               std::string &delta_filename) const {
        const std::string metadata_path =
            generation_directory(generation) + "/meta";
        std::ifstream input(metadata_path);
        if (!input)
            throw std::runtime_error("cannot open checkpoint metadata: " +
                                     metadata_path);

        CheckpointState state;
        input >> state.signature.n >> state.signature.k >>
            state.signature.valid_count >> state.signature.bitmap_bytes;
        input >> state.generation >> state.layer >> state.component_anchor >>
            state.component_size >> state.discovered_survivors >>
            state.boundary_size;
        input >> delta_filename;
        if (!input || state.generation != generation ||
            !signature_matches(expected, state.signature) ||
            delta_filename.empty() ||
            delta_filename.find('/') != std::string::npos ||
            delta_filename == "." || delta_filename == "..")
            throw std::runtime_error("invalid checkpoint metadata: " +
                                     metadata_path);
        state.validate();
        return state;
    }

    // Replays committed generations 1..CURRENT. Gen 0 is reconstructed by
    // the caller from the deterministic Star and its immediate boundary.
    std::uint64_t replay_chain(const CheckpointSignature &expected,
                               AtomicBitset &visited) const {
        const std::uint64_t target = current_generation();
        for (std::uint64_t generation = 1; generation <= target; ++generation) {
            std::string delta_filename;
            read_state(generation, expected, delta_filename);
            replay_frontier_delta(generation_directory(generation) + "/" +
                                      delta_filename,
                                  visited);
        }
        return target;
    }

    // Publishes metadata only after its complete generation directory exists.
    // Bitmap/delta files must be written and fsynced by the caller first.
    void publish(const CheckpointState &state,
                 const std::string &delta_filename) const {
        state.validate();
        const std::string generation_dir =
            generation_directory(state.generation);
        if (mkdir(directory_.c_str(), 0755) != 0 && errno != EEXIST)
            throw std::runtime_error("cannot create checkpoint directory");
        if (mkdir(generation_dir.c_str(), 0755) != 0 && errno != EEXIST)
            throw std::runtime_error("cannot create generation directory");

        const std::string metadata_path = generation_dir + "/meta";
        const std::string temporary_path = metadata_path + ".tmp";
        std::ofstream output(temporary_path, std::ios::trunc);
        if (!output)
            throw std::runtime_error("cannot create checkpoint metadata");
        output << state.signature.n << ' ' << state.signature.k << ' '
               << state.signature.valid_count << ' '
               << state.signature.bitmap_bytes << '\n'
               << state.generation << ' ' << state.layer << ' '
               << state.component_anchor << ' ' << state.component_size << ' '
               << state.discovered_survivors << ' ' << state.boundary_size
               << '\n'
               << delta_filename << '\n';
        output.close();
        durable_fsync_path(temporary_path);
        if (rename(temporary_path.c_str(), metadata_path.c_str()) != 0)
            throw std::runtime_error("cannot publish checkpoint metadata");
        durable_fsync_directory(generation_dir);

        const std::string current_tmp = directory_ + "/CURRENT.tmp";
        const std::string current = directory_ + "/CURRENT";
        unlink(current_tmp.c_str());
        if (symlink(generation_name(state.generation).c_str(),
                    current_tmp.c_str()) != 0 ||
            rename(current_tmp.c_str(), current.c_str()) != 0)
            throw std::runtime_error("cannot publish CURRENT checkpoint");
        durable_fsync_directory(directory_);
    }

  private:
    std::string directory_;
};

} // namespace star_sweep

#endif
