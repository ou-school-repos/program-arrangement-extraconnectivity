// Serial bitmap-frontier experiment for A(n,k).
// The trusted flat and ranked validators remain separate references.
// Usage: validate_extra_cut_bitmap n k

#include "bfs_utils.hpp"

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_max_threads() { return 1; }
inline int omp_get_thread_num() { return 0; }
#endif

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

int main(int argc, char **argv) {
    if (argc != 3 && argc != 5) {
        std::cerr << "Usage: " << argv[0] << " n k [--disk-backed prefix]\n";
        return 1;
    }
    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    const bool disk_backed =
        argc == 5 && std::string(argv[3]) == "--disk-backed";
    if (argc == 5 && !disk_backed) {
        std::cerr << "Usage: " << argv[0] << " n k [--disk-backed prefix]\n";
        return 1;
    }
    const std::string disk_prefix = disk_backed ? argv[4] : std::string{};
    if (n <= k || k < 1 || n > 64 || k > 21) {
        std::cerr << "Error: require 64 >= n > k >= 1 and k <= 21.\n";
        return 1;
    }

    const PackedArrangementGraph graph(n, k);
    const FullStarParameters parameters = full_star_parameters(n, k);

    std::cout << "Build: " << build_version << "\n"
              << "Building rank-indexed A(" << n << ',' << k << ")...\n"
              << "Total valid vertices: " << graph.valid_count << "\n"
              << "Visited bitset: " << (graph.valid_count + 7) / 8 << " bytes\n"
              << "Storage: " << (disk_backed ? "disk-backed (mmap)" : "RAM")
              << "\n"
              << "R = " << parameters.volume() << "\n"
              << "Candidate g = " << parameters.g() << "\n";

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
        return disk_prefix + "." + name + ".bitmap";
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
    constexpr std::uint64_t progress_interval = 1'000'000;
    std::uint64_t next_progress = progress_interval;
    bool direction_message_printed = false;

    std::cout << "|S| = " << star.size() << "\n"
              << "|N(S)| = " << boundary.size() << "\n"
              << "Validating " << parameters.g()
              << "-extra cut properties...\n";

    graph.for_each_valid_code([&](const packed_code_t start) {
        const std::size_t start_rank = graph.rank_code(start);
        if (visited.test(start_rank))
            return;

        std::uint64_t size = 0;
        std::size_t bfs_layer = 0;
        std::vector<bool> bottom_up_by_layer;
        visited.set_atomic(start_rank);
        current_frontier.set_atomic(start_rank);
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
                std::cerr << "\r\033[KTop-down expand: [top-down layer "
                          << std::setw(3) << layer << "] frontier "
                          << current_frontier_size << '\n';

            if (bottom_up) {
                const std::size_t total_words = visited.num_words();
                const std::size_t thread_count =
                    static_cast<std::size_t>(omp_get_max_threads());
                const std::size_t report_step =
                    std::max<std::size_t>(1, total_words / 100);
                const std::size_t local_report_step = std::max<std::size_t>(
                    1024,
                    (total_words + thread_count * 99) / (thread_count * 100));
                const auto scan_counters =
                    std::make_unique<std::vector<PaddedScanCounter>>(
                        thread_count);
                std::atomic<std::size_t> next_scan_report{report_step};

#pragma omp parallel
                {
                    const int thread_index = omp_get_thread_num();
                    std::size_t local_scanned = 0;
#pragma omp for schedule(static) reduction(+ : next_frontier_size)
                    for (std::size_t word_index = 0; word_index < total_words;
                         ++word_index) {
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
                            word_index + 1 == total_words) {
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
                                    report_bfs_scan_progress(
                                        layer, scanned, total_words,
                                        discovered_survivors, total_survivors);
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
                report_bfs_scan_progress(layer, total_words, total_words,
                                         discovered_survivors, total_survivors);
                std::cerr << '\n';
            } else {
                const std::size_t total_blocks = current_frontier.num_blocks();
#pragma omp parallel for schedule(guided) reduction(+ : next_frontier_size)
                for (std::size_t block = 0; block < total_blocks; ++block) {
                    if (!current_frontier.block_dirty(block))
                        continue;
                    const std::size_t first = block * AtomicBitset::block_size;
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
                            const packed_code_t code = graph.decode_rank(rank);
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
            }

            size += current_frontier_size;
            discovered_survivors += current_frontier_size;
            if (discovered_survivors >= next_progress) {
                report_bfs_progress(discovered_survivors, total_survivors,
                                    layer, bottom_up);
                next_progress = (discovered_survivors / progress_interval + 1) *
                                progress_interval;
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
    });

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
