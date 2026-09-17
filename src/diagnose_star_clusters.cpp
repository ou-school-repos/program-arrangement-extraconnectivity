// Exact diagnostics for the radius-one full-Star spine.
//
// Usage: ./diagnose_star_clusters n k [max_j]
//
// S_j consists of the center (0,1,...,k-1) and every one-coordinate leaf
// in the first j coordinate directions.  Thus |S_j| = 1 + j(n-k).

#include "../scripts/arrangement_core.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using Int = std::int64_t;
using arrangement::Instance;

Int e_seq(int r) {
    Int result = 0;
    for (int i = 0; i < r; ++i)
        result += __builtin_popcount(static_cast<unsigned>(i));
    return result;
}

Int ceil_log2_plus_one(int i) {
    Int power = 1;
    Int result = 0;
    while (power < i + 1) {
        power <<= 1;
        ++result;
    }
    return result;
}

Int c_constant(int r) {
    Int result = r == 0 ? 0 : r - 1 - e_seq(r);
    for (int i = 1; i < r; ++i)
        result += ceil_log2_plus_one(i);
    return result;
}

std::vector<int> star(const Instance &graph, int active_coordinates) {
    std::vector<int> result;
    std::vector<int> center(graph.k);
    for (int coordinate = 0; coordinate < graph.k; ++coordinate)
        center[coordinate] = coordinate;
    result.push_back(graph.index.at(graph.encode(center)));
    for (int coordinate = 0; coordinate < active_coordinates; ++coordinate) {
        for (int symbol = graph.k; symbol < graph.n; ++symbol) {
            auto leaf = center;
            leaf[coordinate] = symbol;
            result.push_back(graph.index.at(graph.encode(leaf)));
        }
    }
    return result;
}

Int boundary(const Instance &graph, const std::vector<int> &subset) {
    std::vector<bool> selected(graph.vertices.size(), false);
    for (int vertex : subset)
        selected[vertex] = true;
    std::vector<bool> external(graph.vertices.size(), false);
    for (int position = 0; position < graph.k; ++position) {
        std::vector<bool> active(graph.lines[position].size(), false);
        for (int vertex : subset)
            active[graph.root_id[position][vertex]] = true;
        for (int root = 0; root < static_cast<int>(active.size()); ++root) {
            if (!active[root])
                continue;
            for (int member : graph.lines[position][root])
                if (!selected[member])
                    external[member] = true;
        }
    }
    return std::count(external.begin(), external.end(), true);
}

Int defect(const Instance &graph, const std::vector<int> &subset) {
    int active_roots = 0;
    for (int position = 0; position < graph.k; ++position) {
        std::vector<bool> active(graph.lines[position].size(), false);
        for (int vertex : subset)
            active[graph.root_id[position][vertex]] = true;
        active_roots += std::count(active.begin(), active.end(), true);
    }
    return static_cast<Int>(subset.size()) * graph.k - active_roots;
}

int agreement(const Instance &graph, int u, int v) {
    int result = 0;
    for (int coordinate = 0; coordinate < graph.k; ++coordinate)
        result +=
            graph.vertices[u][coordinate] == graph.vertices[v][coordinate];
    return result;
}

struct PairResult {
    Int margin = std::numeric_limits<Int>::min();
    int u = -1;
    int v = -1;
};

PairResult pair_margin(const Instance &graph, const std::vector<int> &subset) {
    const Int m = graph.n - graph.k;
    const int r = static_cast<int>(subset.size());
    const Int q_before = m * r * graph.k - boundary(graph, subset);
    const Int budget =
        c_constant(r) + m * e_seq(r) - c_constant(r - 2) - m * e_seq(r - 2);
    PairResult result;
    for (std::size_t i = 0; i < subset.size(); ++i) {
        for (std::size_t j = i + 1; j < subset.size(); ++j) {
            std::vector<int> reduced;
            for (std::size_t index = 0; index < subset.size(); ++index)
                if (index != i && index != j)
                    reduced.push_back(subset[index]);
            const Int q_after =
                m * (r - 2) * graph.k - boundary(graph, reduced);
            const Int margin = budget - (q_before - q_after);
            if (margin > result.margin)
                result = {margin, subset[i], subset[j]};
        }
    }
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3 || std::string(argv[1]) == "-h" ||
        std::string(argv[1]) == "--help") {
        std::cout << "usage: " << argv[0] << " n k [max_j]\n";
        return argc < 3 ? 2 : 0;
    }
    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    const int max_j = argc > 3 ? std::stoi(argv[3]) : k;
    if (n <= k || k < 1 || max_j < 0 || max_j > k) {
        std::cerr << "require n > k >= 1 and 0 <= max_j <= k\n";
        return 2;
    }

    const Instance graph(n, k);
    const Int m = n - k;
    std::cout << "A(" << n << ',' << k << ") full-Star spine, m=" << m << '\n';
    std::cout << "j R D X boundary B_slack pair_margin pair\n";
    for (int j = 0; j <= max_j; ++j) {
        const auto subset = star(graph, j);
        const int r = static_cast<int>(subset.size());
        const Int d = defect(graph, subset);
        const Int b = boundary(graph, subset);
        const Int u = r * k - d;
        const Int x = u * m - d - b;
        const Int bound = (r * k - e_seq(r)) * m - c_constant(r);
        const PairResult pair =
            r >= 2 ? pair_margin(graph, subset) : PairResult{};
        std::cout << j << ' ' << r << ' ' << d << ' ' << x << ' ' << b << ' '
                  << b - bound << ' ' << pair.margin << ' ' << pair.u << ','
                  << pair.v;
        if (pair.u >= 0)
            std::cout << " c=" << agreement(graph, pair.u, pair.v);
        std::cout << '\n';
    }
}
