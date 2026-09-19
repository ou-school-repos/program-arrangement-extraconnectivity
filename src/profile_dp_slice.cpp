// Exact slice-transfer prototype for A(n,k).
// Usage: ./profile_dp_slice [n] [k] [max_R]

#include "arrangement_core.hpp"

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using arrangement::Automorphism;
using arrangement::Instance;

namespace {

std::vector<Automorphism> layer_stabilizer(const Instance &instance,
                                           const int processed_slices) {
    constexpr std::size_t max_stabilizer_size = 100'000;
    std::size_t stabilizer_size = 1;
    for (const int degree : {instance.k - 1, instance.n - processed_slices}) {
        for (int factor = 2; factor <= degree; ++factor) {
            const auto value = static_cast<std::size_t>(factor);
            if (stabilizer_size > max_stabilizer_size / value)
                return {};
            stabilizer_size *= value;
        }
    }

    std::vector<Automorphism> group;
    std::vector<int> coordinates(instance.k);
    std::iota(coordinates.begin(), coordinates.end(), 0);
    do {
        if (coordinates[0] != 0)
            continue;
        std::vector<int> unprocessed(instance.n - processed_slices);
        std::iota(unprocessed.begin(), unprocessed.end(), processed_slices);
        do {
            Automorphism automorphism;
            automorphism.coordinates = coordinates;
            automorphism.symbols.resize(instance.n);
            for (int i = 0; i < processed_slices; ++i)
                automorphism.symbols[i] = i;
            for (int i = 0; i < instance.n - processed_slices; ++i)
                automorphism.symbols[processed_slices + i] = unprocessed[i];
            group.push_back(std::move(automorphism));
        } while (std::next_permutation(unprocessed.begin(), unprocessed.end()));
    } while (std::next_permutation(coordinates.begin(), coordinates.end()));
    return group;
}

std::vector<int>
canonical_residual(const std::vector<int> &subset, const Instance &instance,
                   const std::vector<Automorphism> &stabilizer) {
    if (subset.empty() || stabilizer.empty())
        return subset;

    std::vector<int> best;
    bool initialized = false;
    for (const auto &automorphism : stabilizer) {
        std::vector<int> mapped;
        mapped.reserve(subset.size());
        std::transform(subset.begin(), subset.end(), std::back_inserter(mapped),
                       [&automorphism, &instance](const int vertex) {
                           return automorphism.apply(vertex, instance);
                       });
        std::sort(mapped.begin(), mapped.end());
        if (!initialized || mapped < best) {
            best = std::move(mapped);
            initialized = true;
        }
    }
    return best;
}

int boundary_size(const std::vector<int> &subset, const Instance &instance) {
    std::vector<unsigned char> selected(instance.vertices.size(), false);
    std::vector<unsigned char> boundary(instance.vertices.size(), false);
    for (const int vertex : subset)
        selected[vertex] = true;
    for (const int vertex : subset) {
        for (int position = 0; position < instance.k; ++position) {
            const int root = instance.root_id[position][vertex];
            for (const int neighbor : instance.lines[position][root]) {
                if (!selected[neighbor])
                    boundary[neighbor] = true;
            }
        }
    }
    return static_cast<int>(std::count(boundary.begin(), boundary.end(), true));
}

} // namespace

int main(int argc, char **argv) {
    if (argc > 4) {
        std::cerr << "usage: " << argv[0] << " [n] [k] [max_R]\n";
        return 2;
    }
    const int n = argc > 1 ? std::stoi(argv[1]) : 5;
    const int k = argc > 2 ? std::stoi(argv[2]) : 3;
    const int max_volume = argc > 3 ? std::stoi(argv[3]) : 6;
    if (n < 1 || k < 1 || k > n || max_volume < 0) {
        std::cerr << "require n >= 1, 1 <= k <= n, max_R >= 0\n";
        return 2;
    }
    constexpr std::size_t max_vertices = 500'000;
    std::size_t vertex_count = 1;
    for (int i = 0; i < k; ++i) {
        const auto factor = static_cast<std::size_t>(n - i);
        if (vertex_count > max_vertices / factor) {
            std::cerr << "graph exceeds " << max_vertices << " vertices\n";
            return 2;
        }
        vertex_count *= factor;
    }

    const Instance instance(n, k);
    if (static_cast<std::size_t>(max_volume) > instance.vertices.size()) {
        std::cerr << "max_R exceeds vertex count " << instance.vertices.size()
                  << '\n';
        return 2;
    }
    std::cout << "Transfer-DP slice audit for A(" << n << ',' << k
              << ") max_R=" << max_volume << "\n"
              << "vertices=" << instance.vertices.size() << "\n\n";

    std::vector<std::set<std::vector<int>>> layers(max_volume + 1);
    layers[0].insert(std::vector<int>{});

    for (int slice = 0; slice < n; ++slice) {
        std::vector<int> slice_vertices;
        for (int vertex = 0;
             vertex < static_cast<int>(instance.vertices.size()); ++vertex) {
            if (instance.vertices[vertex][0] == slice)
                slice_vertices.push_back(vertex);
        }

        constexpr std::size_t max_slice_subsets = 500'000;
        std::vector<std::vector<int>> slice_subsets(1);
        for (const int vertex : slice_vertices) {
            const std::size_t current_size = slice_subsets.size();
            for (std::size_t i = 0; i < current_size; ++i) {
                if (slice_subsets[i].size() >=
                    static_cast<std::size_t>(max_volume))
                    continue;
                auto expanded = slice_subsets[i];
                expanded.push_back(vertex);
                slice_subsets.push_back(std::move(expanded));
                if (slice_subsets.size() > max_slice_subsets) {
                    std::cerr << "slice " << slice << " exceeds the subset "
                              << "limit of " << max_slice_subsets << '\n';
                    return 1;
                }
            }
        }

        const auto stabilizer = layer_stabilizer(instance, slice + 1);
        if (stabilizer.empty())
            std::cerr << "slice " << slice
                      << ": stabilizer exceeds 100000; using identity only\n";
        std::vector<std::set<std::vector<int>>> next(max_volume + 1);
        constexpr std::size_t max_total_states = 1'000'000;
        std::size_t total_states = 0;
        for (int volume = 0; volume <= max_volume; ++volume) {
            for (const auto &prefix : layers[volume]) {
                for (const auto &suffix : slice_subsets) {
                    if (volume + static_cast<int>(suffix.size()) > max_volume)
                        continue;
                    auto merged = prefix;
                    merged.insert(merged.end(), suffix.begin(), suffix.end());
                    std::sort(merged.begin(), merged.end());
                    const auto [unused, inserted] =
                        next[volume + static_cast<int>(suffix.size())].insert(
                            canonical_residual(merged, instance, stabilizer));
                    (void)unused;
                    if (inserted && ++total_states > max_total_states) {
                        std::cerr << "slice DP exceeds the state limit of "
                                  << max_total_states << '\n';
                        return 1;
                    }
                }
            }
        }
        layers = std::move(next);

        std::size_t total = 0;
        std::cout << "after-slice=" << slice + 1
                  << " residual-stabilizer=" << stabilizer.size()
                  << " states-by-volume=";
        for (const auto &layer : layers) {
            std::cout << ' ' << layer.size();
            total += layer.size();
        }
        std::cout << " total=" << total << "\n";
    }

    std::cout << "\nprofile:\n";
    for (int volume = 0; volume <= max_volume; ++volume) {
        const int best = std::accumulate(
            layers[volume].begin(), layers[volume].end(),
            static_cast<int>(instance.vertices.size()) + 1,
            [&instance](const int current, const std::vector<int> &subset) {
                return std::min(current, boundary_size(subset, instance));
            });
        std::cout << "R=" << volume
                  << " best=" << (layers[volume].empty() ? -1 : best) << '\n';
    }
    return 0;
}
