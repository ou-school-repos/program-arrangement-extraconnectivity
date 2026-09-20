#include "star_sweep/checkpoint_manager.hpp"
#include "star_sweep/checksum.hpp"
#include "star_sweep/frontier_delta.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

int main() {
    namespace fs = std::filesystem;
    star_sweep::Xxh64 checksum;
    const std::uint64_t empty_checksum = checksum.digest();
    const std::string probe = "checkpoint";
    checksum.update(probe.data(), probe.size());
    assert(checksum.digest() != empty_checksum);
    std::cout << "[checkpoint] checksum changes after input: PASS\n";

    const fs::path base =
        fs::temp_directory_path() / "star-sweep-checkpoint-test";
    fs::remove_all(base);
    fs::create_directories(base / star_sweep::generation_name(1));

    AtomicBitset frontier(4096);
    frontier.set_atomic(3);
    frontier.set_atomic(2048);

    const fs::path delta =
        base / star_sweep::generation_name(1) / "frontier.delta";
    const std::uint64_t written_blocks =
        star_sweep::write_frontier_delta(frontier, delta.string());
    assert(written_blocks == 1);
    std::cout << "[checkpoint] wrote frontier delta: 1 dirty block: PASS\n";
    AtomicBitset restored(4096);
    star_sweep::replay_frontier_delta(delta.string(), restored);
    assert(restored.test(3));
    assert(restored.test(2048));
    std::cout << "[checkpoint] replay restored bits 3 and 2048: PASS\n";

    const star_sweep::CheckpointSignature signature{11, 7, 4096, 512};
    assert(star_sweep::signature_matches(signature, signature));
    std::cout << "[checkpoint] signature validation: PASS\n";

    star_sweep::CheckpointState state;
    state.signature = signature;
    state.generation = 1;
    state.layer = 1;
    state.component_anchor = 17;
    state.component_size = 1;
    state.discovered_survivors = 1;
    state.star_boundary_size = 42;
    state.active_frontier_size = 2;
    state.component_sizes = {10};
    state.direction_history = {false};

    star_sweep::CheckpointManager manager(base.string());
    manager.publish(state, "frontier.delta");
    const bool current_is_symlink = fs::is_symlink(base / "CURRENT");
    assert(current_is_symlink);
    std::cout
        << "[checkpoint] published generation 1 and CURRENT symlink: PASS\n";
    AtomicBitset chain_restored(4096);
    const std::size_t replayed_generations =
        manager.replay_chain(signature, chain_restored);
    assert(replayed_generations == 1);
    assert(chain_restored.test(3));
    assert(chain_restored.test(2048));
    std::cout << "[checkpoint] replayed 1 generation and restored bits: PASS\n";

    fs::remove_all(base);
    std::cout << "Checkpoint smoke test passed.\n";
}
