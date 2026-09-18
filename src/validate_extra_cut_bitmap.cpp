// Serial bitmap-frontier experiment for A(n,k).
// The trusted flat and ranked validators remain separate references.
// Usage: validate_extra_cut_bitmap n k [--disk-backed prefix]
//        [--partition-index P --partition-count C]

#include "bfs_utils.hpp"
#include "star_sweep/checkpoint_manager.hpp"
#include "star_sweep/frontier_chunk.hpp"

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_max_threads() { return 1; }
inline int omp_get_thread_num() { return 0; }
#endif

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " n k [--disk-backed prefix]\n";
        return 1;
    }
    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    bool disk_backed = false;
    std::string disk_prefix;
    std::uint64_t interrupt_generation = 0;
    std::size_t partition_index = 0;
    std::size_t partition_count = 1;
    bool partition_index_seen = false;
    bool partition_count_seen = false;
    for (int index = 3; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--disk-backed" && index + 1 < argc) {
            disk_backed = true;
            disk_prefix = argv[++index];
        } else if (argument == "--partition-index" && index + 1 < argc) {
            partition_index = std::stoull(argv[++index]);
            partition_index_seen = true;
        } else if (argument == "--partition-count" && index + 1 < argc) {
            partition_count = std::stoull(argv[++index]);
            partition_count_seen = true;
        } else if (argument == "--interrupt-at-generation" &&
                   index + 1 < argc) {
            interrupt_generation = std::stoull(argv[++index]);
        } else {
            std::cerr << "Usage: " << argv[0]
                      << " n k [--disk-backed prefix]"
                         " [--partition-index P --partition-count C]"
                         " [--interrupt-at-generation N]\n";
            return 1;
        }
    }
    if (n <= k || k < 1 || n > 64 || k > 21) {
        std::cerr << "Error: require 64 >= n > k >= 1 and k <= 21.\n";
        return 1;
    }
    if (partition_index_seen != partition_count_seen) {
        std::cerr << "Error: --partition-index and --partition-count must "
                     "be provided together.\n";
        return 1;
    }
    const bool partition_mode = partition_index_seen;
    if (partition_mode && (!disk_backed || partition_count == 0 ||
                           partition_index >= partition_count)) {
        std::cerr << "Error: partitioned execution requires valid disk-backed "
                     "partition arguments.\n";
        return 1;
    }
    const star_sweep::PartitionSpec partition{partition_index, partition_count};

    const PackedArrangementGraph graph(n, k);
    const FullStarParameters parameters = full_star_parameters(n, k);

    std::unique_ptr<star_sweep::CheckpointManager> checkpoint_manager;
    bool resume_mode = false;
    if (disk_backed) {
        checkpoint_manager =
            std::make_unique<star_sweep::CheckpointManager>(disk_prefix);
        if (checkpoint_manager->has_current())
            resume_mode = true;
        else if (checkpoint_manager->has_checkpoint_artifacts()) {
            std::cerr << "Error: checkpoint artifacts exist but CURRENT is "
                         "missing; refusing to overwrite the broken chain.\n";
            return 1;
        }
    }
    if (partition_mode && !resume_mode) {
        std::cerr << "Error: partitioned execution requires an existing "
                     "active checkpoint.\n";
        return 1;
    }

    std::cout << "Build: " << build_version << "\n"
              << "Building rank-indexed A(" << n << ',' << k << ")...\n"
              << "Total valid vertices: " << graph.valid_count << "\n"
              << "Visited bitset: " << (graph.valid_count + 7) / 8 << " bytes\n"
              << "Storage: "
              << (disk_backed ? (resume_mode ? "disk-backed (resuming)"
                                             : "disk-backed (mmap)")
                              : "RAM")
              << "\n"
              << "R = " << parameters.volume() << "\n"
              << "Candidate g = " << parameters.g() << "\n";
    if (partition_mode)
        std::cout << "Partition: " << partition.index << "/" << partition.count
                  << "\n";

    std::vector<int> center(k);
    std::iota(center.begin(), center.end(), 0);
    const packed_code_t center_code = graph.encode(center);
    std::vector<packed_code_t> star{center_code};
    for (int position = 0; position < k; ++position) {
        for (int symbol = k; symbol < n; ++symbol) {
            const packed_code_t mask =
                ~(static_cast<packed_code_t>(63) << (6 * position));
            star.push_back(
                (center_code & mask) |
                (static_cast<packed_code_t>(symbol) << (6 * position)));
        }
    }

    if (!star_connected(graph, star)) {
        std::cerr << "Error: constructed Star is not connected.\n";
        return 1;
    }
    std::cout << "Subset S connectivity verified.\n";

    const auto bitmap_path = [&](const char *name) {
        const std::string working_prefix =
            partition_mode ? disk_prefix + ".partition-" +
                                 std::to_string(partition.index) + "-of-" +
                                 std::to_string(partition.count)
                           : disk_prefix;
        return working_prefix + "." + name + ".bitmap";
    };
    auto visited_storage =
        disk_backed ? std::make_unique<AtomicBitset>(graph.valid_count,
                                                     bitmap_path("visited"))
                    : std::make_unique<AtomicBitset>(graph.valid_count);
    auto current_storage =
        disk_backed ? std::make_unique<AtomicBitset>(graph.valid_count,
                                                     bitmap_path("current"))
                    : std::make_unique<AtomicBitset>(graph.valid_count);
    auto next_storage =
        disk_backed ? std::make_unique<AtomicBitset>(graph.valid_count,
                                                     bitmap_path("next"))
                    : std::make_unique<AtomicBitset>(graph.valid_count);
    AtomicBitset &visited = *visited_storage;
    AtomicBitset &current_frontier = *current_storage;
    AtomicBitset &next_frontier = *next_storage;

    star_sweep::CheckpointSignature expected_signature{
        n, k, graph.valid_count,
        ((graph.valid_count + 63) / 64) * sizeof(std::uint64_t)};
    for (const packed_code_t code : star)
        visited.set_atomic(graph.rank_code(code));

    std::vector<packed_code_t> boundary;
    for (const packed_code_t code : star) {
        graph.for_each_neighbor(code, [&](const packed_code_t neighbor) {
            const std::size_t rank = graph.rank_code(neighbor);
            if (!visited.test(rank)) {
                visited.set_atomic(rank);
                boundary.push_back(neighbor);
            }
        });
    }

    std::vector<std::uint64_t> component_sizes{star.size()};
    const std::uint64_t total_survivors =
        graph.valid_count - star.size() - boundary.size();
    std::uint64_t discovered_survivors = 0;
    std::uint64_t active_generation = 0;
    star_sweep::CheckpointState resume_state;
    bool resume_component = false;

    if (resume_mode) {
        if (!checkpoint_manager->has_current()) {
            std::cerr << "Error: checkpoint CURRENT disappeared during "
                         "startup.\n";
            return 1;
        }
        active_generation = checkpoint_manager->current_generation();
        std::string delta_filename;
        resume_state = checkpoint_manager->read_state(
            active_generation, expected_signature, delta_filename);
        checkpoint_manager->replay_chain(expected_signature, visited);
        component_sizes = resume_state.component_sizes;
        discovered_survivors = resume_state.discovered_survivors;
        if (resume_state.star_boundary_size != boundary.size()) {
            std::cerr << "Error: checkpoint Star boundary mismatch.\n";
            return 1;
        }
        if (resume_state.phase == star_sweep::CheckpointPhase::LayerBoundary) {
            current_frontier.clear();
            star_sweep::replay_frontier_delta(
                checkpoint_manager->generation_directory(active_generation) +
                    "/" + delta_filename,
                current_frontier);
            resume_component = true;
        }
        if (partition_mode &&
            (!resume_component || resume_state.active_frontier_size == 0)) {
            std::cerr << "Error: partitioned execution requires an active "
                         "frontier checkpoint.\n";
            return 1;
        }
        std::cout
            << "Checkpoint resume:\n"
            << "  generation: " << active_generation << "\n"
            << "  phase: "
            << (resume_state.phase == star_sweep::CheckpointPhase::LayerBoundary
                    ? "active frontier"
                    : "component complete")
            << "\n"
            << "  layer: " << resume_state.layer << "\n"
            << "  component anchor rank: " << resume_state.component_anchor
            << "\n"
            << "  component size: " << resume_state.component_size << "\n"
            << "  restored frontier size: " << resume_state.active_frontier_size
            << "\n"
            << "  discovered survivors: " << resume_state.discovered_survivors
            << " / " << total_survivors << "\n"
            << "  saved component history: " << component_sizes.size()
            << " entries\n"
            << "  saved direction history: "
            << resume_state.direction_history.size() << " layers\n";
    }
    constexpr std::uint64_t progress_interval = 1'000'000;
    std::uint64_t next_progress =
        resume_mode ? ((discovered_survivors / progress_interval) + 1) *
                          progress_interval
                    : progress_interval;
    bool direction_message_printed = false;

    std::cout << "|S| = " << star.size() << "\n"
              << "|N(S)| = " << boundary.size() << "\n"
              << "Validating " << parameters.g()
              << "-extra cut properties...\n";

    bool partition_finished = false;
    graph.for_each_valid_code([&](const packed_code_t start) {
        if (partition_finished)
            return;
        const std::size_t start_rank = graph.rank_code(start);
        const bool restoring = resume_component;
        if (restoring) {
            if (start_rank != resume_state.component_anchor)
                return;
        } else if (visited.test(start_rank)) {
            return;
        }

        std::uint64_t size = restoring ? resume_state.component_size : 0;
        std::size_t bfs_layer =
            restoring ? static_cast<std::size_t>(resume_state.layer) : 0;
        std::vector<bool> bottom_up_by_layer =
            restoring ? resume_state.direction_history : std::vector<bool>{};
        if (restoring) {
            resume_component = false;
        } else {
            visited.set_atomic(start_rank);
            current_frontier.set_atomic(start_rank);
        }
        while (true) {
            std::uint64_t current_frontier_size = 0;
            for (std::size_t block = 0; block < current_frontier.num_blocks();
                 ++block) {
                if (!current_frontier.block_dirty(block))
                    continue;
                const std::size_t first = block * AtomicBitset::block_size;
                const std::size_t last =
                    std::min(first + AtomicBitset::block_size,
                             current_frontier.num_words());
                for (std::size_t word_index = first; word_index < last;
                     ++word_index)
                    current_frontier_size += __builtin_popcountll(
                        current_frontier.load_word(word_index));
            }

            if (current_frontier_size == 0)
                break;

            const std::size_t layer = ++bfs_layer;

            std::uint64_t next_frontier_size = 0;
            const bool bottom_up =
                current_frontier_size >= graph.valid_count / 20;
            bottom_up_by_layer.push_back(bottom_up);

            if (bottom_up && !direction_message_printed) {
                std::cerr << "[Direction Optimized: Bottom-Up Scan Active]\n"
                          << std::flush;
                direction_message_printed = true;
            }
            if (!bottom_up)
                std::cerr << "Layer " << std::setw(3) << layer << ": frontier "
                          << current_frontier_size << '\n';

            if (bottom_up) {
                const std::size_t total_words = visited.num_words();
                const auto word_range =
                    partition_mode
                        ? partition.range(total_words)
                        : std::pair<std::size_t, std::size_t>{0, total_words};
                const std::size_t scan_words =
                    word_range.second - word_range.first;
                const std::size_t thread_count =
                    static_cast<std::size_t>(omp_get_max_threads());
                const std::size_t report_step =
                    std::max<std::size_t>(1, scan_words / 10);
                const std::size_t local_report_step = std::max<std::size_t>(
                    1024,
                    (scan_words + thread_count * 99) / (thread_count * 100));
                const auto scan_counters =
                    std::make_unique<std::vector<PaddedScanCounter>>(
                        thread_count);
                std::atomic<std::size_t> next_scan_report{report_step};

#pragma omp parallel
                {
                    const int thread_index = omp_get_thread_num();
                    std::size_t local_scanned = 0;
#pragma omp for schedule(static) reduction(+ : next_frontier_size)
                    for (std::size_t word_index = word_range.first;
                         word_index < word_range.second; ++word_index) {
                        const std::uint64_t visited_word =
                            visited.load_word(word_index);
                        if (visited_word !=
                            std::numeric_limits<std::uint64_t>::max()) {
                            std::uint64_t new_word = 0;
                            for (int bit = 0; bit < 64; ++bit) {
                                if ((visited_word >> bit) & 1)
                                    continue;
                                const std::size_t rank = word_index * 64 + bit;
                                if (rank >= graph.valid_count)
                                    break;

                                const packed_code_t code =
                                    graph.decode_rank(rank);
                                bool discovered = false;
                                graph.for_each_neighbor(
                                    code, [&](const packed_code_t neighbor) {
                                        if (discovered)
                                            return;
                                        const std::size_t neighbor_rank =
                                            graph.rank_code(neighbor);
                                        if (current_frontier.test(
                                                neighbor_rank))
                                            discovered = true;
                                    });
                                if (discovered) {
                                    new_word |= std::uint64_t{1} << bit;
                                    ++next_frontier_size;
                                }
                            }
                            if (new_word != 0)
                                next_frontier.set_word_atomic(word_index,
                                                              new_word);
                        }

                        ++local_scanned;
                        if (local_scanned % local_report_step == 0 ||
                            word_index + 1 == word_range.second) {
                            (*scan_counters)[thread_index].value.store(
                                local_scanned, std::memory_order_relaxed);
#pragma omp critical(bfs_scan_progress)
                            {
                                const std::size_t scanned = std::accumulate(
                                    scan_counters->begin(),
                                    scan_counters->end(), std::size_t{0},
                                    [](std::size_t total,
                                       const PaddedScanCounter &counter) {
                                        return total +
                                               counter.value.load(
                                                   std::memory_order_relaxed);
                                    });
                                const std::size_t target =
                                    next_scan_report.load(
                                        std::memory_order_relaxed);
                                if (scanned >= target) {
                                    report_bfs_scan_progress(layer, scanned,
                                                             scan_words);
                                    next_scan_report.store(
                                        ((scanned / report_step) + 1) *
                                            report_step,
                                        std::memory_order_relaxed);
                                }
                            }
                        }
                    }
                    (*scan_counters)[thread_index].value.store(
                        local_scanned, std::memory_order_relaxed);
                }
                if (next_scan_report.load(std::memory_order_relaxed) <=
                    scan_words)
                    report_bfs_scan_progress(layer, scan_words, scan_words);
                std::cerr << '\n';
            } else {
                const std::size_t total_blocks = current_frontier.num_blocks();
                const auto block_range =
                    partition_mode
                        ? partition.range(total_blocks)
                        : std::pair<std::size_t, std::size_t>{0, total_blocks};
                const std::size_t scan_blocks =
                    block_range.second - block_range.first;
                const std::size_t report_step =
                    std::max<std::size_t>(1, scan_blocks / 10);
                std::atomic<std::size_t> scanned_blocks{0};
                std::atomic<std::size_t> last_reported{0};
#pragma omp parallel for schedule(guided) reduction(+ : next_frontier_size)
                for (std::size_t block = block_range.first;
                     block < block_range.second; ++block) {
                    if (current_frontier.block_dirty(block)) {
                        const std::size_t first =
                            block * AtomicBitset::block_size;
                        const std::size_t last =
                            std::min(first + AtomicBitset::block_size,
                                     current_frontier.num_words());
                        for (std::size_t word_index = first; word_index < last;
                             ++word_index) {
                            std::uint64_t word =
                                current_frontier.load_word(word_index);
                            while (word != 0) {
                                const int bit = __builtin_ctzll(word);
                                word &= word - 1;
                                const std::size_t rank = word_index * 64 + bit;
                                const packed_code_t code =
                                    graph.decode_rank(rank);
                                graph.for_each_neighbor(
                                    code, [&](const packed_code_t neighbor) {
                                        const std::size_t neighbor_rank =
                                            graph.rank_code(neighbor);
                                        if (!visited.test(neighbor_rank) &&
                                            next_frontier.set_atomic_check(
                                                neighbor_rank))
                                            ++next_frontier_size;
                                    });
                            }
                        }
                    }
                    const std::size_t scanned =
                        scanned_blocks.fetch_add(1, std::memory_order_relaxed) +
                        1;
                    if (scanned % report_step == 0 || scanned == scan_blocks) {
#pragma omp critical(bfs_topdown_progress)
                        {
                            std::size_t previous =
                                last_reported.load(std::memory_order_relaxed);
                            if (scanned > previous) {
                                last_reported.store(scanned,
                                                    std::memory_order_relaxed);
                                report_bfs_topdown_progress(layer, scanned,
                                                            scan_blocks);
                            }
                        }
                    }
                }
                if (last_reported.load(std::memory_order_relaxed) < scan_blocks)
                    report_bfs_topdown_progress(layer, scan_blocks,
                                                scan_blocks);
                std::cerr << '\n';
            }

            size += current_frontier_size;
            discovered_survivors += current_frontier_size;
            if (discovered_survivors >= next_progress) {
                report_bfs_progress(discovered_survivors, total_survivors,
                                    layer, bottom_up);
                next_progress = (discovered_survivors / progress_interval + 1) *
                                progress_interval;
                std::cerr << '\n';
            }

            if (partition_mode) {
                const std::uint64_t target_generation = active_generation + 1;
                const std::string generation_dir =
                    checkpoint_manager->generation_directory(target_generation);
                std::filesystem::create_directories(generation_dir);
                const auto word_range = partition.range(visited.num_words());
                const auto block_range =
                    partition.range(current_frontier.num_blocks());
                const std::string chunk_name = "frontier.chunk." +
                                               std::to_string(partition.index) +
                                               ".delta";
                const std::string chunk_path =
                    generation_dir + "/" + chunk_name;
                if (bottom_up) {
                    star_sweep::write_frontier_delta(next_frontier, chunk_path,
                                                     block_range.first,
                                                     block_range.second);
                } else {
                    star_sweep::write_frontier_delta(next_frontier, chunk_path);
                }

                star_sweep::PartitionChunkState chunk;
                chunk.signature = expected_signature;
                chunk.base_generation = active_generation;
                chunk.target_generation = target_generation;
                chunk.layer = bfs_layer;
                chunk.partition_index = partition.index;
                chunk.partition_count = partition.count;
                chunk.first_word = word_range.first;
                chunk.last_word = word_range.second;
                chunk.first_block = block_range.first;
                chunk.last_block = block_range.second;
                chunk.local_frontier_size = next_frontier_size;
                chunk.bottom_up = bottom_up;
                star_sweep::write_partition_chunk_metadata(chunk_path + ".meta",
                                                           chunk);
                std::cout << "Partition chunk complete: generation "
                          << target_generation << ", layer " << bfs_layer
                          << ", partition " << partition.index << "/"
                          << partition.count << ", local frontier "
                          << next_frontier_size << "\n";
                partition_finished = true;
                return;
            }

            if (disk_backed) {
                star_sweep::CheckpointState state;
                state.signature = expected_signature;
                state.generation = ++active_generation;
                state.phase = star_sweep::CheckpointPhase::LayerBoundary;
                state.layer = bfs_layer;
                state.component_anchor = start_rank;
                state.component_size = size;
                state.discovered_survivors = discovered_survivors;
                state.star_boundary_size = boundary.size();
                state.active_frontier_size = next_frontier_size;
                state.component_sizes = component_sizes;
                state.direction_history = bottom_up_by_layer;

                const std::string generation_dir =
                    checkpoint_manager->generation_directory(state.generation);
                std::filesystem::create_directories(generation_dir);
                const std::string delta_name = "frontier.delta";
                star_sweep::write_frontier_delta(
                    next_frontier, generation_dir + "/" + delta_name);
                checkpoint_manager->publish(state, delta_name);
                if (interrupt_generation != 0 &&
                    active_generation >= interrupt_generation)
                    std::exit(99);
            }

            if (next_frontier_size == 0)
                break;
            visited.merge_from(next_frontier);
            current_frontier.swap(next_frontier);
            next_frontier.clear();
        }
        std::cerr << "Component complete: " << bfs_layer
                  << " layers (directions:";
        for (const bool layer_bottom_up : bottom_up_by_layer)
            std::cerr << (layer_bottom_up ? " BU" : " TD");
        std::cerr << ")\n";
        component_sizes.push_back(size);

        if (disk_backed) {
            star_sweep::CheckpointState state;
            state.signature = expected_signature;
            state.generation = ++active_generation;
            state.phase = star_sweep::CheckpointPhase::ComponentComplete;
            state.layer = bfs_layer;
            state.component_anchor = start_rank;
            state.component_size = size;
            state.discovered_survivors = discovered_survivors;
            state.star_boundary_size = boundary.size();
            state.active_frontier_size = 0;
            state.component_sizes = component_sizes;
            state.direction_history = bottom_up_by_layer;

            const std::string generation_dir =
                checkpoint_manager->generation_directory(state.generation);
            std::filesystem::create_directories(generation_dir);
            const std::string delta_name = "frontier.delta";
            star_sweep::write_frontier_delta(next_frontier,
                                             generation_dir + "/" + delta_name);
            checkpoint_manager->publish(state, delta_name);
        }
    });

    if (partition_mode)
        return partition_finished ? 0 : 1;

    report_bfs_progress(discovered_survivors, total_survivors);
    std::cerr << '\n';

    std::sort(component_sizes.begin(), component_sizes.end());
    const bool valid = component_sizes.size() >= 2 &&
                       std::all_of(component_sizes.begin(),
                                   component_sizes.end(), [&](const auto size) {
                                       return size > static_cast<std::uint64_t>(
                                                         parameters.g());
                                   });
    std::cout << "component sizes after deletion:\n";
    for (const std::uint64_t size : component_sizes)
        std::cout << "  " << size << '\n';
    std::cout << "valid " << parameters.g()
              << "-extra cut: " << (valid ? "yes" : "no") << '\n';
    if (valid)
        std::cout << "therefore kappa_" << parameters.g() << "(A(" << n << ','
                  << k << ")) <= " << boundary.size() << '\n';
    std::cout << "Hamming baseline: " << parameters.hamming_boundary()
              << "; Star boundary: " << boundary.size() << "; Delta: "
              << (parameters.hamming_boundary() -
                  static_cast<long long>(boundary.size()))
              << '\n'
              << "Embedding gate: d = " << parameters.d() << ", k = " << k
              << ", n-k = " << parameters.m() << " ("
              << (parameters.embedding_gate() ? "open" : "closed") << ")\n";
    if (!valid)
        std::cout << "INVALID EXTRA CUT\n";
    else if (parameters.boundary() < parameters.hamming_boundary() &&
             parameters.embedding_gate())
        std::cout << "HARD COUNTEREXAMPLE: RestrictedLowerBound\n";
    else if (parameters.boundary() < parameters.hamming_boundary())
        std::cout << "SOFT COUNTEREXAMPLE: UniversalLowerBound only\n";
    else
        std::cout << "SATISFIES HAMMING OPTIMALITY\n";
    return 0;
}
