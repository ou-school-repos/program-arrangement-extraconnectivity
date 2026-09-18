#ifndef STAR_SWEEP_CHECKPOINT_MANAGER_HPP
#define STAR_SWEEP_CHECKPOINT_MANAGER_HPP

#include "checkpoint_types.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace star_sweep {

inline void fsync_path(const std::string &path) {
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0 || fsync(fd) != 0) {
        const std::string error = std::strerror(errno);
        if (fd >= 0)
            close(fd);
        throw std::runtime_error("cannot fsync " + path + ": " + error);
    }
    close(fd);
}

inline void fsync_directory(const std::string &path) {
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

class CheckpointManager {
  public:
    explicit CheckpointManager(std::string directory)
        : directory_(std::move(directory)) {}

    std::string generation_directory(std::uint64_t generation) const {
        return directory_ + "/" + generation_name(generation);
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
        fsync_path(temporary_path);
        if (rename(temporary_path.c_str(), metadata_path.c_str()) != 0)
            throw std::runtime_error("cannot publish checkpoint metadata");
        fsync_directory(generation_dir);

        const std::string current_tmp = directory_ + "/CURRENT.tmp";
        const std::string current = directory_ + "/CURRENT";
        unlink(current_tmp.c_str());
        if (symlink(generation_name(state.generation).c_str(),
                    current_tmp.c_str()) != 0 ||
            rename(current_tmp.c_str(), current.c_str()) != 0)
            throw std::runtime_error("cannot publish CURRENT checkpoint");
        fsync_directory(directory_);
    }

  private:
    std::string directory_;
};

} // namespace star_sweep

#endif
