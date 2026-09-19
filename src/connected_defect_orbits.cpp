// Orbit-reduced diagnostic for high-defect connected subsets.
// Usage: connected_defect_orbits n k R target
#include "arrangement_core.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <utility>
#include <vector>

using namespace arrangement;

int main(int argc, char **argv) {
    if (argc != 5) {
        std::fprintf(stderr, "usage: %s n k R target\n", argv[0]);
        return 2;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int R = std::atoi(argv[3]);
    const int target = std::atoi(argv[4]);
    if (n <= k || k < 1 || R < 1 || target < 0)
        return 2;
    Instance I(n, k);
    const auto stabilizer = origin_stabilizer(I);
    std::vector<std::vector<int>> adjacency(I.vertices.size());
    for (int p = 0; p < k; ++p)
        for (const auto &line : I.lines[p])
            for (int a : line)
                for (int b : line)
                    if (a != b)
                        adjacency[a].push_back(b);
    auto defect = [&](const std::vector<int> &set) {
        std::set<std::pair<int, int>> roots;
        for (int vertex : set)
            for (int p = 0; p < k; ++p)
                roots.emplace(p, I.root_id[p][vertex]);
        return static_cast<int>(set.size()) * k -
               static_cast<int>(roots.size());
    };
    std::vector<int> origin(k);
    for (int i = 0; i < k; ++i)
        origin[i] = i;
    std::set<std::vector<int>> layer{{I.index.at(I.encode(origin))}};
    std::set<int> spectrum;
    spectrum.insert(defect(origin));
    for (int r = 2; r <= R; ++r) {
        const int threshold = target - k * (R - r);
        std::set<std::vector<int>> next;
        for (const auto &set : layer) {
            std::set<int> candidates;
            for (int vertex : set)
                for (int neighbor : adjacency[vertex])
                    candidates.insert(neighbor);
            for (int vertex : set)
                candidates.erase(vertex);
            for (int vertex : candidates) {
                auto expanded = set;
                expanded.push_back(vertex);
                std::sort(expanded.begin(), expanded.end());
                const int value = defect(expanded);
                if (r == R) {
                    spectrum.insert(value);
                } else if (value >= threshold) {
                    next.insert(canonical_key(expanded, I, stabilizer));
                }
            }
        }
        std::printf("r=%d kept-orbits=%zu threshold=%d\n", r,
                    r == R ? 0U : next.size(), threshold);
        std::fflush(stdout);
        if (r < R)
            layer.swap(next);
    }
    std::printf("A(%d,%d) R=%d connected defects near target %d:", n, k, R,
                target);
    for (int value : spectrum)
        if (value >= target - 2)
            std::printf(" %d", value);
    std::printf("\n");
}
