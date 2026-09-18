#include "bfs_utils.hpp"
#include "star_sweep/checkpoint_manager.hpp"
#include "star_sweep/frontier_chunk.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int run(int argc, char **argv) {
    if (argc != 6 || std::string(argv[4]) != "--partition-count") {
        std::cerr << "Usage: " << argv[0]
                  << " n k checkpoint-prefix --partition-count C\n";
        return 1;
    }

    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    const std::string prefix = argv[3];
    const std::size_t partition_count = std::stoull(argv[5]);
    if (n <= k || k < 1 || n > 64 || k > 21 || partition_count == 0) {
        std::cerr << "Error: invalid graph or partition arguments.\n";
        return 1;
    }

    const PackedArrangementGraph graph(n, k);
    const star_sweep::CheckpointSignature signature{
        n, k, graph.valid_count,
        ((graph.valid_count + 63) / 64) * sizeof(std::uint64_t)};
    star_sweep::CheckpointManager manager(prefix);
    if (!manager.has_current()) {
        std::cerr << "Error: checkpoint CURRENT is missing.\n";
        return 1;
    }

    const std::uint64_t base_generation = manager.current_generation();
    std::string base_delta_name;
    const star_sweep::CheckpointState base =
        manager.read_state(base_generation, signature, base_delta_name);
    if (base.phase != star_sweep::CheckpointPhase::LayerBoundary) {
        std::cerr << "Error: CURRENT does not identify an active frontier.\n";
        return 1;
    }

    AtomicBitset canonical_next(graph.valid_count);
    bool have_direction = false;
    bool bottom_up = false;
    for (std::size_t index = 0; index < partition_count; ++index) {
        const std::uint64_t target_generation = base_generation + 1;
        const std::string generation_dir =
            manager.generation_directory(target_generation);
        const std::string chunk_name =
            "frontier.chunk." + std::to_string(index) + ".delta";
        const std::string chunk_path = generation_dir + "/" + chunk_name;
        const std::string metadata_path = chunk_path + ".meta";
        const star_sweep::PartitionChunkState chunk =
            star_sweep::read_partition_chunk_metadata(metadata_path, signature);
        const star_sweep::PartitionSpec partition{index, partition_count};
        const auto expected_words = partition.range(canonical_next.num_words());
        const auto expected_blocks =
            partition.range(canonical_next.num_blocks());
        if (chunk.base_generation != base_generation ||
            chunk.target_generation != target_generation ||
            chunk.layer != base.layer + 1 || chunk.partition_index != index ||
            chunk.partition_count != partition_count ||
            chunk.first_word != expected_words.first ||
            chunk.last_word != expected_words.second ||
            chunk.first_block != expected_blocks.first ||
            chunk.last_block != expected_blocks.second ||
            (have_direction && chunk.bottom_up != bottom_up)) {
            throw std::runtime_error("partition chunk does not match layer");
        }
        if (!have_direction) {
            bottom_up = chunk.bottom_up;
            have_direction = true;
        }
        std::uint64_t chunk_bits = 0;
        star_sweep::replay_frontier_delta(chunk_path, canonical_next,
                                          &chunk_bits);
        if (chunk_bits != chunk.local_frontier_size)
            throw std::runtime_error("partition chunk cardinality mismatch");
    }

    std::uint64_t canonical_size = 0;
    for (std::size_t word = 0; word < canonical_next.num_words(); ++word)
        canonical_size += __builtin_popcountll(canonical_next.load_word(word));

    const std::uint64_t target_generation = base_generation + 1;
    const std::string target_dir =
        manager.generation_directory(target_generation);
    std::filesystem::create_directories(target_dir);
    const std::string delta_name = "frontier.delta";
    star_sweep::write_frontier_delta(canonical_next,
                                     target_dir + "/" + delta_name);

    star_sweep::CheckpointState state = base;
    state.generation = target_generation;
    state.layer = base.layer + 1;
    state.component_size += base.active_frontier_size;
    state.discovered_survivors += base.active_frontier_size;
    state.active_frontier_size = canonical_size;
    state.direction_history.push_back(bottom_up);
    if (canonical_size == 0) {
        state.phase = star_sweep::CheckpointPhase::ComponentComplete;
        state.component_sizes.push_back(state.component_size);
    } else {
        state.phase = star_sweep::CheckpointPhase::LayerBoundary;
    }
    manager.publish(state, delta_name);

    std::cout << "Merged partition generation " << target_generation << ": "
              << partition_count << " chunks, canonical frontier "
              << canonical_size << ".\n";
    return 0;
}

} // namespace

int main(int argc, char **argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
