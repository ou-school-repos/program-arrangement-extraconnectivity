// Local structural search for subsets that may improve on the full Star.
//
// This is an empirical obstruction search, not an optimality proof.  It never
// allocates the full arrangement graph: only the requested subset and the
// candidate vertices adjacent to it are held in memory.
//
// Usage: search_hybrids n k [iterations] [restarts]

#include "arrangement_utils.hpp"
#include "build_info.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Vertex = packed_code_t;

bool contains(const std::vector<Vertex> &vertices, const Vertex value) {
    return std::find(vertices.begin(), vertices.end(), value) != vertices.end();
}

bool adjacent(const Vertex left, const Vertex right, const int k) {
    int differences = 0;
    for (int position = 0; position < k; ++position) {
        if (((left >> (6 * position)) & 63) !=
            ((right >> (6 * position)) & 63)) {
            ++differences;
            if (differences > 1)
                return false;
        }
    }
    return differences == 1;
}

std::uint64_t internal_edges(const std::vector<Vertex> &subset, const int k) {
    std::size_t index = 0;
    return std::accumulate(
        subset.begin(), subset.end(), std::uint64_t{0},
        [&](const std::uint64_t total, const Vertex vertex) {
            const std::uint64_t result =
                total +
                std::accumulate(
                    subset.begin() + static_cast<std::ptrdiff_t>(index + 1),
                    subset.end(), std::uint64_t{0},
                    [&](const std::uint64_t inner_total,
                        const Vertex later_vertex) {
                        return inner_total +
                               (adjacent(vertex, later_vertex, k) ? 1U : 0U);
                    });
            ++index;
            return result;
        });
}

std::uint64_t edges_to_subset(const Vertex vertex,
                              const std::vector<Vertex> &subset, const int k,
                              const std::size_t skipped) {
    std::uint64_t result = 0;
    for (std::size_t i = 0; i < subset.size(); ++i)
        if (i != skipped)
            result += adjacent(vertex, subset[i], k) ? 1U : 0U;
    return result;
}

std::vector<Vertex> full_star(const PackedArrangementGraph &graph) {
    std::vector<int> center(graph.k);
    std::iota(center.begin(), center.end(), 0);
    const Vertex center_code = graph.encode(center);
    std::vector<Vertex> result{center_code};
    graph.for_each_neighbor(center_code, [&](const Vertex neighbor) {
        result.push_back(neighbor);
    });
    return result;
}

// Start with a small core, then add the currently most adjacent candidate.
// This makes edge- and multi-core seeds well-defined at exactly |Star|.
std::vector<Vertex> complete_seed(const PackedArrangementGraph &graph,
                                  std::vector<Vertex> core,
                                  const std::size_t target_size) {
    std::vector<Vertex> candidates;
    auto collect_boundary = [&]() {
        for (const Vertex vertex : core) {
            graph.for_each_neighbor(vertex, [&](const Vertex neighbor) {
                if (!contains(core, neighbor) &&
                    !contains(candidates, neighbor))
                    candidates.push_back(neighbor);
            });
        }
    };

    collect_boundary();
    while (core.size() < target_size) {
        if (candidates.empty())
            collect_boundary();
        if (candidates.size() == 0)
            throw std::runtime_error("could not complete hybrid seed");

        std::size_t best = 0;
        std::uint64_t best_gain = 0;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            const std::uint64_t gain =
                edges_to_subset(candidates[i], core, graph.k,
                                std::numeric_limits<std::size_t>::max());
            if (i == 0 || gain > best_gain) {
                best = i;
                best_gain = gain;
            }
        }
        core.push_back(candidates[best]);
        candidates[best] = candidates.back();
        candidates.pop_back();

        if (candidates.empty() && core.size() < target_size)
            collect_boundary();
    }
    return core;
}

std::vector<Vertex> edge_core(const PackedArrangementGraph &graph,
                              const std::size_t target_size) {
    std::vector<Vertex> star = full_star(graph);
    if (star.size() < 2)
        throw std::runtime_error("graph has no edge core");
    std::vector<Vertex> core{star[0], star[1]};
    return complete_seed(graph, std::move(core), target_size);
}

std::vector<Vertex> multi_core(const PackedArrangementGraph &graph,
                               const std::size_t target_size) {
    const std::vector<Vertex> star = full_star(graph);
    std::vector<Vertex> core{star[0]};
    if (star.size() > 2)
        core.push_back(star[1]);
    if (star.size() > 3)
        core.push_back(star[2]);
    return complete_seed(graph, std::move(core), target_size);
}

bool choose_absorb(const PackedArrangementGraph &graph,
                   const std::vector<Vertex> &subset, const Vertex anchor,
                   std::mt19937 &rng, Vertex &result) {
    std::size_t eligible = 0;
    graph.for_each_neighbor(anchor, [&](const Vertex neighbor) {
        if (!contains(subset, neighbor)) {
            ++eligible;
            // Reservoir sampling selects one eligible neighbor uniformly while
            // generating the anchor neighborhood only once.
            std::uniform_int_distribution<std::size_t> pick(1, eligible);
            if (pick(rng) == 1)
                result = neighbor;
        }
    });
    return eligible != 0;
}

std::uint64_t boundary_size(const std::vector<Vertex> &subset, const int degree,
                            const std::uint64_t edges) {
    return static_cast<std::uint64_t>(subset.size()) *
               static_cast<std::uint64_t>(degree) -
           2U * edges;
}

void print_vertex(const Vertex code, const int k) {
    std::cout << "  [";
    for (int position = 0; position < k; ++position) {
        if (position != 0)
            std::cout << ',';
        std::cout << static_cast<unsigned>((code >> (6 * position)) & 63);
    }
    std::cout << "]\n";
}

void print_witness(const std::vector<Vertex> &subset, const int k) {
    std::cout << "Witness subset (" << subset.size() << " vertices):\n";
    for (const Vertex vertex : subset)
        print_vertex(vertex, k);
}

std::vector<Vertex> climb(const PackedArrangementGraph &graph,
                          std::vector<Vertex> subset,
                          const std::uint64_t iterations, std::mt19937 &rng,
                          std::uint64_t &best_edges) {
    std::uint64_t current_edges = internal_edges(subset, graph.k);
    best_edges = current_edges;
    std::uniform_int_distribution<std::size_t> pick(0, subset.size() - 1);
    constexpr std::uint64_t plateau_limit = 32;
    std::uint64_t plateau_steps = 0;

    for (std::uint64_t iteration = 0; iteration < iterations; ++iteration) {
        const std::size_t evict_index = pick(rng);
        const Vertex evict = subset[evict_index];
        const Vertex anchor = subset[pick(rng)];
        Vertex absorb = 0;
        if (!choose_absorb(graph, subset, anchor, rng, absorb))
            continue;

        const std::uint64_t lost =
            edges_to_subset(evict, subset, graph.k, evict_index);
        const std::uint64_t gained =
            edges_to_subset(absorb, subset, graph.k, evict_index);
        const std::uint64_t candidate_edges = current_edges - lost + gained;

        const bool improves = candidate_edges > current_edges;
        const bool crosses_plateau =
            candidate_edges == current_edges && plateau_steps < plateau_limit;
        if (improves || crosses_plateau) {
            subset[evict_index] = absorb;
            current_edges = candidate_edges;
            plateau_steps = improves ? 0 : plateau_steps + 1;
            if (current_edges > best_edges) {
                best_edges = current_edges;
                const std::uint64_t boundary = boundary_size(
                    subset, graph.k * (graph.n - graph.k), best_edges);
                std::cout << "  improvement at iteration " << iteration
                          << ": internal edges " << best_edges << ", boundary "
                          << boundary << '\n';
            }
        }
    }
    return subset;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " n k [iterations] [restarts]\n";
        return 1;
    }

    try {
        const int n = std::stoi(argv[1]);
        const int k = std::stoi(argv[2]);
        const std::uint64_t iterations =
            argc >= 4 ? std::stoull(argv[3]) : 5'000'000U;
        const std::size_t restarts = argc >= 5 ? std::stoull(argv[4]) : 8U;
        if (n <= k || k < 1 || n > 64 || k > 21 || iterations == 0 ||
            restarts == 0)
            throw std::invalid_argument("invalid search parameters");

        const PackedArrangementGraph graph(n, k);
        const std::vector<Vertex> star = full_star(graph);
        const std::size_t target_size = star.size();
        const int degree = k * (n - k);
        const std::uint64_t star_edges = internal_edges(star, k);
        const std::uint64_t star_boundary =
            boundary_size(star, degree, star_edges);

        std::cout << "Build: " << build_version << '\n'
                  << "Local hybrid search for A(" << n << ',' << k << ")\n"
                  << "Target subset size: " << target_size << '\n'
                  << "Degree: " << degree << '\n'
                  << "Full-Star internal edges: " << star_edges << '\n'
                  << "Full-Star boundary: " << star_boundary << '\n'
                  << "Iterations per restart: " << iterations << '\n'
                  << "Restarts: " << restarts << '\n';

        std::mt19937 rng(0x51A7U);
        std::uint64_t global_best_edges = star_edges;
        std::vector<Vertex> global_best = star;
        for (std::size_t restart = 0; restart < restarts; ++restart) {
            std::vector<Vertex> seed = restart % 2 == 0
                                           ? edge_core(graph, target_size)
                                           : multi_core(graph, target_size);
            std::uint64_t best_edges = 0;
            std::cout << "Restart " << (restart + 1) << '/' << restarts
                      << ": initial boundary "
                      << boundary_size(seed, degree, internal_edges(seed, k))
                      << '\n';
            std::vector<Vertex> result =
                climb(graph, std::move(seed), iterations, rng, best_edges);
            if (best_edges > global_best_edges) {
                global_best_edges = best_edges;
                global_best = std::move(result);
                const std::uint64_t boundary =
                    boundary_size(global_best, degree, global_best_edges);
                std::cout << "NEW BEST: internal edges " << global_best_edges
                          << ", boundary " << boundary << '\n';
            }
        }

        const std::uint64_t final_boundary =
            boundary_size(global_best, degree, global_best_edges);
        std::cout << "Best observed internal edges: " << global_best_edges
                  << '\n'
                  << "Best observed boundary: " << final_boundary << '\n';
        if (final_boundary < star_boundary) {
            std::cout << "Potential smaller-boundary structure found; this is "
                         "not a proof of optimality.\n";
            print_witness(global_best, k);
        } else {
            std::cout << "No sampled structure beat the full Star.\n";
        }
    } catch (const std::exception &error) {
        std::cerr << "search_hybrids: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
