#include "star_sweep/checkpoint_manager.hpp"
#include "star_sweep/frontier_delta.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>

int main() {
    namespace fs = std::filesystem;
    const fs::path base =
        fs::temp_directory_path() / "star-sweep-checkpoint-test";
    fs::remove_all(base);
    fs::create_directories(base / "gen-00001");

    AtomicBitset frontier(4096);
    frontier.set_atomic(3);
    frontier.set_atomic(2048);

    const fs::path delta = base / "gen-00001" / "frontier.delta";
    assert(star_sweep::write_frontier_delta(frontier, delta.string()) == 1);

    const star_sweep::CheckpointSignature signature{11, 7, 7920, 990};
    assert(star_sweep::signature_matches(signature, signature));

    star_sweep::CheckpointState state;
    state.signature = signature;
    state.generation = 1;
    state.layer = 4;
    state.component_anchor = 17;
    state.component_size = 128;
    state.discovered_survivors = 256;
    state.boundary_size = 42;

    star_sweep::CheckpointManager manager(base.string());
    manager.publish(state, "frontier.delta");
    const bool current_is_symlink = fs::is_symlink(base / "CURRENT");
    assert(current_is_symlink);

    fs::remove_all(base);
}
