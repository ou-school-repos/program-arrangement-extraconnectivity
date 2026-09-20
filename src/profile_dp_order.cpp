// Greedy fiber ordering for arrangement-graph frontier measurement.
// Usage: ./profile_dp_order [n] [k]

#include "arrangement_core.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

using arrangement::Instance;

int main(int argc, char **argv) {
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0] << " [n] [k]\n";
        return 0;
    }
    if (argc > 3) {
        std::cerr << "usage: " << argv[0] << " [n] [k]\n";
        return 2;
    }
    const int n = argc > 1 ? std::stoi(argv[1]) : 5;
    const int k = argc > 2 ? std::stoi(argv[2]) : 3;
    const Instance instance(n, k);

    const int fiber_count = std::accumulate(
        instance.lines.begin(), instance.lines.end(), 0,
        [](const int total, const std::vector<std::vector<int>> &lines) {
            return total + static_cast<int>(lines.size());
        });

    std::vector<std::vector<int>> fiber_vertices(fiber_count);
    std::vector<std::vector<int>> vertex_fibers(instance.vertices.size());
    int offset = 0;
    for (int coordinate = 0; coordinate < k; ++coordinate) {
        for (std::size_t root = 0; root < instance.lines[coordinate].size();
             ++root) {
            const int fiber = offset + static_cast<int>(root);
            for (const int vertex : instance.lines[coordinate][root]) {
                fiber_vertices[fiber].push_back(vertex);
                vertex_fibers[vertex].push_back(fiber);
            }
        }
        offset += static_cast<int>(instance.lines[coordinate].size());
    }

    std::vector<bool> processed(fiber_count, false);
    std::vector<int> processed_incidence(instance.vertices.size(), 0);
    std::vector<int> order;
    order.reserve(fiber_count);
    int frontier = 0;
    int maximum_frontier = 0;

    std::cout << "A(" << n << ',' << k << ") fiber ordering\n"
              << "vertices=" << instance.vertices.size()
              << " fibers=" << fiber_count << "\n";

    for (int step = 0; step < fiber_count; ++step) {
        int best_fiber = -1;
        int best_width = static_cast<int>(instance.vertices.size()) + 1;
        int best_completed = -1;

        for (int fiber = 0; fiber < fiber_count; ++fiber) {
            if (processed[fiber] || (step == 0 && fiber != 0))
                continue;

            int width = frontier;
            int completed = 0;
            for (const int vertex : fiber_vertices[fiber]) {
                if (processed_incidence[vertex] == 0)
                    ++width;
                if (processed_incidence[vertex] == k - 1) {
                    --width;
                    ++completed;
                }
            }
            if (width < best_width ||
                (width == best_width && completed > best_completed) ||
                (width == best_width && completed == best_completed &&
                 fiber < best_fiber)) {
                best_fiber = fiber;
                best_width = width;
                best_completed = completed;
            }
        }

        if (best_fiber < 0) {
            std::cerr << "unable to choose fiber at step " << step << '\n';
            return 2;
        }
        processed[best_fiber] = true;
        order.push_back(best_fiber);
        for (const int vertex : fiber_vertices[best_fiber]) {
            if (processed_incidence[vertex] == 0)
                ++frontier;
            ++processed_incidence[vertex];
            if (processed_incidence[vertex] == k)
                --frontier;
        }
        maximum_frontier = std::max(maximum_frontier, frontier);

        std::cout << "step=" << step + 1 << '/' << fiber_count
                  << " fiber=" << best_fiber << " frontier=" << frontier
                  << " max=" << maximum_frontier << '\n';
    }

    std::cout << "max-frontier-width=" << maximum_frontier << '\n';
    return 0;
}
