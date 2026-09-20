// Rank-indexed experimental validator for larger A(n,k) instances.
// The trusted flat validator remains bin/validate_extra_cut.
// Usage: validate_extra_cut_ranked n k

#include "bfs_utils.hpp"
#include "build_info.hpp"

#include <omp.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

class Bitset {
  public:
    explicit Bitset(std::size_t bits) : words((bits + 63) / 64, 0) {}

    bool test(std::size_t bit) const {
        return (words[bit / 64] >> (bit % 64)) & 1;
    }

    void set(std::size_t bit) {
        words[bit / 64] |= std::uint64_t{1} << (bit % 64);
    }

  private:
    std::vector<std::uint64_t> words;
};

static std::string classification(const FullStarParameters &parameters,
                                  bool valid) {
    if (!valid)
        return "INVALID EXTRA CUT";
    if (parameters.boundary() < parameters.hamming_boundary() &&
        parameters.embedding_gate())
        return "HARD COUNTEREXAMPLE: RestrictedLowerBound";
    if (parameters.boundary() < parameters.hamming_boundary())
        return "SOFT COUNTEREXAMPLE: UniversalLowerBound only";
    return "SATISFIES HAMMING OPTIMALITY";
}

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

    {
        std::size_t perm_count = 1;
        for (int i = 0; i < k; ++i) {
            const std::size_t factor = static_cast<std::size_t>(n - i);
            if (perm_count > std::numeric_limits<std::size_t>::max() / factor) {
                std::cerr << "Error: rank frontier requires fewer than 2^32 "
                             "vertices.\n";
                return 1;
            }
            perm_count *= factor;
        }
        if (perm_count > std::numeric_limits<std::uint32_t>::max()) {
            std::cerr << "Error: rank frontier requires fewer than 2^32 "
                         "vertices.\n";
            return 1;
        }
    }
    const PackedArrangementGraph graph(n, k);
    const FullStarParameters parameters = full_star_parameters(n, k);
    std::cout << "Build: " << build_version << "\n"
              << "Building rank-indexed A(" << n << ',' << k << ")...\n"
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

    Bitset visited(graph.valid_count);
    for (const packed_code_t code : star)
        visited.set(graph.rank_code(code));

    std::vector<packed_code_t> boundary;
    for (const packed_code_t code : star) {
        graph.for_each_neighbor(code, [&](const packed_code_t neighbor) {
            const std::size_t rank = graph.rank_code(neighbor);
            if (!visited.test(rank)) {
                visited.set(rank);
                boundary.push_back(neighbor);
            }
        });
    }
    std::cout << "|S| = " << star.size() << "\n"
              << "|N(S)| = " << boundary.size() << "\n"
              << "Validating " << parameters.g()
              << "-extra cut properties...\n";

    // The Star remains a component after its external boundary is deleted.
    // Its vertices are already marked, so record it before scanning the
    // unmarked complement.
    const std::uint64_t total_survivors =
        graph.valid_count - star.size() - boundary.size();
    std::uint64_t discovered_survivors = 0;
    constexpr std::uint64_t progress_interval = 1'000'000;
    std::vector<std::uint64_t> component_sizes{star.size()};
    graph.for_each_valid_code([&](const packed_code_t start) {
        const std::size_t start_rank = graph.rank_code(start);
        if (visited.test(start_rank))
            return;
        std::uint64_t size = 0;
        std::vector<std::uint32_t> frontier{
            static_cast<std::uint32_t>(start_rank)};
        visited.set(start_rank);
        while (!frontier.empty()) {
            std::vector<std::uint32_t> next;
            for (const std::uint32_t current_rank : frontier) {
                const packed_code_t current = graph.decode_rank(current_rank);
                ++size;
                ++discovered_survivors;
                if (discovered_survivors % progress_interval == 0)
                    report_bfs_progress(discovered_survivors, total_survivors);
                graph.for_each_neighbor(
                    current, [&](const packed_code_t neighbor) {
                        const std::size_t rank = graph.rank_code(neighbor);
                        if (!visited.test(rank)) {
                            visited.set(rank);
                            next.push_back(static_cast<std::uint32_t>(rank));
                        }
                    });
            }
            frontier.swap(next);
        }
        component_sizes.push_back(size);
    });

    report_bfs_progress(discovered_survivors, total_survivors);
    std::cerr << '\n';

    std::sort(component_sizes.begin(), component_sizes.end());
    bool valid = component_sizes.size() >= 2;
    std::cout << "component sizes after deletion:\n";
    for (const std::uint64_t size : component_sizes) {
        std::cout << "  " << size << '\n';
        if (size <= static_cast<std::uint64_t>(parameters.g()))
            valid = false;
    }
    std::cout << "valid " << parameters.g()
              << "-extra cut: " << (valid ? "yes" : "no") << "\n";
    if (valid)
        std::cout << "therefore kappa_" << parameters.g() << "(A(" << n << ','
                  << k << ")) <= " << boundary.size() << "\n";
    std::cout << "Hamming baseline: " << parameters.hamming_boundary()
              << "; Star boundary: " << boundary.size() << "; Delta: "
              << (parameters.hamming_boundary() -
                  static_cast<long long>(boundary.size()))
              << "\n"
              << "Embedding gate: d = " << parameters.d() << ", k = " << k
              << ", n-k = " << parameters.m() << " ("
              << (parameters.embedding_gate() ? "open" : "closed") << ")\n"
              << classification(parameters, valid) << '\n';
    return 0;
}
