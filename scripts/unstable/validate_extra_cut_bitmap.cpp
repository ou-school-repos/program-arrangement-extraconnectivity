// Serial bitmap-frontier experiment for A(n,k).
// The trusted flat and ranked validators remain separate references.
// Usage: validate_extra_cut_bitmap n k

#include "bfs_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " n k\n";
        return 1;
    }
    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    if (n <= k || k < 1 || n > 64 || k > 21) {
        std::cerr << "Error: require 64 >= n > k >= 1 and k <= 21.\n";
        return 1;
    }

    const PackedArrangementGraph graph(n, k);
    const FullStarParameters parameters = full_star_parameters(n, k);

    std::cout << "Building rank-indexed A(" << n << ',' << k << ")...\n"
              << "Total valid vertices: " << graph.valid_count << "\n"
              << "Visited bitset: " << (graph.valid_count + 7) / 8 << " bytes\n"
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

    AtomicBitset visited(graph.valid_count);
    for (const packed_code_t code : star)
        visited.set_atomic(graph.rank_code(code));

    std::vector<std::uint64_t> boundary;
    for (const packed_code_t code : star) {
        graph.for_each_neighbor(code, [&](const packed_code_t neighbor) {
            const std::size_t rank = graph.rank_code(neighbor);
            if (!visited.test(rank)) {
                visited.set_atomic(rank);
                boundary.push_back(neighbor);
            }
        });
    }

    AtomicBitset current_frontier(graph.valid_count);
    AtomicBitset next_frontier(graph.valid_count);
    std::vector<std::uint64_t> component_sizes{star.size()};
    const std::uint64_t total_survivors =
        graph.valid_count - star.size() - boundary.size();
    std::uint64_t discovered_survivors = 0;
    constexpr std::uint64_t progress_interval = 1'000'000;
    std::uint64_t next_progress = progress_interval;

    std::cout << "|S| = " << star.size() << "\n"
              << "|N(S)| = " << boundary.size() << "\n"
              << "Validating " << parameters.g()
              << "-extra cut properties...\n";

    graph.for_each_valid_code([&](const packed_code_t start) {
        const std::size_t start_rank = graph.rank_code(start);
        if (visited.test(start_rank))
            return;

        std::uint64_t size = 0;
        current_frontier.set_atomic(start_rank);
        visited.set_atomic(start_rank);
        while (true) {
            std::uint64_t level_size = 0;
            const std::size_t total_words = current_frontier.num_words();

#pragma omp parallel for schedule(dynamic, 64) reduction(+ : level_size)
            for (std::size_t word_index = 0; word_index < total_words;
                 ++word_index) {
                std::uint64_t word = current_frontier.load_word(word_index);
                while (word != 0) {
                    const int bit = __builtin_ctzll(word);
                    word &= word - 1;
                    const std::size_t rank = word_index * 64 + bit;
                    ++level_size;
                    const packed_code_t code = graph.decode_rank(rank);
                    graph.for_each_neighbor(
                        code, [&](const packed_code_t neighbor) {
                            const std::size_t neighbor_rank =
                                graph.rank_code(neighbor);
                            if (!visited.test(neighbor_rank))
                                next_frontier.set_atomic(neighbor_rank);
                        });
                }
            }

            if (level_size == 0)
                break;
            size += level_size;
            discovered_survivors += level_size;
            if (discovered_survivors >= next_progress) {
                report_bfs_progress(discovered_survivors, total_survivors);
                next_progress = (discovered_survivors / progress_interval + 1) *
                                progress_interval;
            }
            visited.merge_from(next_frontier);
            current_frontier.swap(next_frontier);
            next_frontier.clear();
        }
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
              << "; Star boundary: " << boundary.size() << '\n'
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
