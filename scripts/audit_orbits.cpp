// Burnside regression counts for subset orbits of A(n,k).
// Usage: ./audit_orbits n k max_R

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <vector>

using Count = unsigned long long;

std::uint64_t encode(const std::vector<int> &vertex, int n) {
    return std::accumulate(vertex.begin(), vertex.end(), std::uint64_t{0},
                           [n](const std::uint64_t code, const int symbol) {
                               return code * static_cast<std::uint64_t>(n) +
                                      symbol;
                           });
}

void enumerate_vertices(int n, int k, std::vector<int> &prefix,
                        std::vector<bool> &used,
                        std::vector<std::vector<int>> &vertices) {
    if (static_cast<int>(prefix.size()) == k) {
        vertices.push_back(prefix);
        return;
    }
    for (int symbol = 0; symbol < n; ++symbol) {
        if (used[symbol])
            continue;
        used[symbol] = true;
        prefix.push_back(symbol);
        enumerate_vertices(n, k, prefix, used, vertices);
        prefix.pop_back();
        used[symbol] = false;
    }
}

std::vector<Count> burnside(int n, int k, int maximum) {
    std::vector<std::vector<int>> vertices;
    std::vector<int> prefix;
    std::vector<bool> used(n, false);
    enumerate_vertices(n, k, prefix, used, vertices);

    std::unordered_map<std::uint64_t, int> index;
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i)
        index.emplace(encode(vertices[i], n), i);

    std::vector<int> symbols(n);
    std::iota(symbols.begin(), symbols.end(), 0);
    std::vector<Count> totals(maximum + 1, 0);
    Count group_size = 0;

    do {
        std::vector<int> coordinates(k);
        std::iota(coordinates.begin(), coordinates.end(), 0);
        do {
            ++group_size;
            std::vector<int> image(vertices.size());
            for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
                std::vector<int> mapped(k);
                for (int p = 0; p < k; ++p)
                    mapped[p] = symbols[vertices[i][coordinates[p]]];
                image[i] = index.at(encode(mapped, n));
            }

            std::vector<bool> seen(vertices.size(), false);
            std::vector<Count> polynomial(maximum + 1, 0);
            polynomial[0] = 1;
            for (int start = 0; start < static_cast<int>(vertices.size());
                 ++start) {
                if (seen[start])
                    continue;
                int length = 0;
                int current = start;
                while (!seen[current]) {
                    seen[current] = true;
                    current = image[current];
                    ++length;
                }
                std::vector<Count> updated = polynomial;
                for (int degree = 0; degree + length <= maximum; ++degree)
                    updated[degree + length] += polynomial[degree];
                polynomial.swap(updated);
            }
            for (int r = 0; r <= maximum; ++r)
                totals[r] += polynomial[r];
        } while (std::next_permutation(coordinates.begin(), coordinates.end()));
    } while (std::next_permutation(symbols.begin(), symbols.end()));

    std::transform(
        totals.begin(), totals.end(), totals.begin(),
        [group_size](const Count total) { return total / group_size; });
    return totals;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << " n k max_R\n";
        return 1;
    }
    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    const int maximum = std::stoi(argv[3]);
    if (n < 1 || k < 0 || k > n || maximum < 0 || n > 9 || k > 8) {
        std::cerr << "unsupported audit range\n";
        return 1;
    }
    const auto counts = burnside(n, k, maximum);
    std::cout << "A(" << n << "," << k << ") Burnside subset-orbit counts:\n[";
    for (int r = 0; r <= maximum; ++r)
        std::cout << (r == 0 ? "" : ", ") << counts[r];
    std::cout << "]\n";
}
