// Exact diagnostics for the radius-one full-Star spine.
//
// Usage: ./diagnose_star_clusters n k [max_j]
//
// This diagnostic deliberately does not construct A(n,k). It materializes
// only the O(k(n-k)) vertices in the Star being tested, so large probes do
// not allocate the factorial-sized arrangement graph.

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using Int = std::int64_t;
using Vertex = std::vector<int>;
using Subset = std::vector<Vertex>;

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

Subset star(int n, int k, int active_coordinates) {
    Subset result;
    Vertex center(k);
    for (int coordinate = 0; coordinate < k; ++coordinate)
        center[coordinate] = coordinate;
    result.push_back(center);
    for (int coordinate = 0; coordinate < active_coordinates; ++coordinate) {
        for (int symbol = k; symbol < n; ++symbol) {
            Vertex leaf = center;
            leaf[coordinate] = symbol;
            result.push_back(std::move(leaf));
        }
    }
    return result;
}

using Root = std::pair<int, Vertex>;

std::set<Root> active_roots(const Subset &subset) {
    std::set<Root> roots;
    for (const Vertex &vertex : subset) {
        for (std::size_t coordinate = 0; coordinate < vertex.size();
             ++coordinate) {
            Vertex root = vertex;
            root.erase(root.begin() + static_cast<std::ptrdiff_t>(coordinate));
            roots.emplace(static_cast<int>(coordinate), std::move(root));
        }
    }
    return roots;
}

Int defect(const Subset &subset, int k) {
    const Int active = static_cast<Int>(active_roots(subset).size());
    return static_cast<Int>(subset.size()) * k - active;
}

Int boundary(const Subset &subset, int n, int k) {
    const std::set<Vertex> selected(subset.begin(), subset.end());
    std::set<Vertex> external;
    for (const Vertex &vertex : subset) {
        for (int coordinate = 0; coordinate < k; ++coordinate) {
            for (int symbol = 0; symbol < n; ++symbol) {
                if (symbol == vertex[coordinate])
                    continue;
                if (std::find(vertex.begin(), vertex.end(), symbol) !=
                    vertex.end())
                    continue;
                Vertex neighbor = vertex;
                neighbor[coordinate] = symbol;
                if (!selected.count(neighbor))
                    external.insert(std::move(neighbor));
            }
        }
    }
    return static_cast<Int>(external.size());
}

int agreement(const Vertex &u, const Vertex &v) {
    int result = 0;
    for (std::size_t coordinate = 0; coordinate < u.size(); ++coordinate)
        result += u[coordinate] == v[coordinate];
    return result;
}

struct PairResult {
    Int margin = std::numeric_limits<Int>::min();
    int u = -1;
    int v = -1;
};

PairResult pair_margin(const Subset &subset, int n, int k) {
    const Int m = n - k;
    const int r = static_cast<int>(subset.size());
    const Int q_before = m * r * k - boundary(subset, n, k);
    const Int budget =
        c_constant(r) + m * e_seq(r) - c_constant(r - 2) - m * e_seq(r - 2);
    PairResult result;
    auto consider = [&](int i, int j) {
        Subset reduced;
        for (int index = 0; index < r; ++index)
            if (index != i && index != j)
                reduced.push_back(subset[index]);
        const Int q_after = m * (r - 2) * k - boundary(reduced, n, k);
        const Int margin = budget - (q_before - q_after);
        if (margin > result.margin)
            result = {margin, i, j};
    };

    // The Star automorphism group has four pair orbits: center-leaf,
    // same-branch leaves, and leaves from different branches with either the
    // same or different unused symbols. One representative of each orbit is
    // therefore an exact exhaustive test.
    const int leaves_per_branch = n - k;
    consider(0, 1); // center and a leaf
    if (leaves_per_branch >= 2)
        consider(1, 2); // two leaves in one branch
    if (r > 1 + leaves_per_branch) {
        consider(1, 1 + leaves_per_branch); // different branches, same symbol
        if (leaves_per_branch >= 2)
            consider(1, 2 + leaves_per_branch);
        // different branches, different symbols
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

    const Int m = n - k;
    std::cout << "A(" << n << ',' << k
              << ") full-Star spine (formula-only), m=" << m << '\n';
    std::cout << "j R D X boundary B_slack pair_margin pair\n";
    for (int j = 0; j <= max_j; ++j) {
        const Subset subset = star(n, k, j);
        const int r = static_cast<int>(subset.size());
        const Int d = defect(subset, k);
        const Int b = boundary(subset, n, k);
        const Int u = r * k - d;
        const Int x = u * m - d - b;
        const Int bound = (r * k - e_seq(r)) * m - c_constant(r);
        const PairResult pair =
            r >= 2 ? pair_margin(subset, n, k) : PairResult{};
        std::cout << j << ' ' << r << ' ' << d << ' ' << x << ' ' << b << ' '
                  << b - bound << ' ' << pair.margin << ' ' << pair.u << ','
                  << pair.v;
        if (pair.u >= 0)
            std::cout << " c=" << agreement(subset[pair.u], subset[pair.v]);
        std::cout << '\n';
    }
}
