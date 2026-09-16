#include <array>
#include <iostream>
#include <unordered_set>
#include <vector>

using Vertex = std::array<int, 5>;

struct VertexHash {
    std::size_t operator()(const Vertex &vertex) const {
        std::size_t hash = 0;
        for (const int symbol : vertex)
            hash = hash * 11 + static_cast<std::size_t>(symbol);
        return hash;
    }
};

using VertexSet = std::unordered_set<Vertex, VertexHash>;

int calculate_boundary(const std::vector<Vertex> &vertices, int n, int k) {
    const VertexSet members(vertices.begin(), vertices.end());
    VertexSet boundary;
    for (const Vertex &vertex : vertices) {
        bool used[10] = {};
        for (const int symbol : vertex)
            used[symbol] = true;
        for (int position = 0; position < k; ++position) {
            for (int symbol = 0; symbol < n; ++symbol) {
                if (used[symbol])
                    continue;
                Vertex neighbor = vertex;
                neighbor[position] = symbol;
                if (members.find(neighbor) == members.end())
                    boundary.insert(neighbor);
            }
        }
    }
    return static_cast<int>(boundary.size());
}

int main() {
    constexpr int n = 10;
    constexpr int k = 5;
    std::cout << "--- Stage 1: A(10,5), R=26 ---\n";

    const Vertex center = {0, 1, 2, 3, 4};
    std::vector<Vertex> star{center};
    for (int position = 0; position < k; ++position) {
        for (int symbol = k; symbol < n; ++symbol) {
            Vertex leaf = center;
            leaf[position] = symbol;
            star.push_back(leaf);
        }
    }

    std::vector<Vertex> binary_ball;
    const int pairs[k][2] = {{0, 5}, {1, 6}, {2, 7}, {3, 8}, {4, 9}};
    for (int a = 0; a < 2; ++a)
        for (int b = 0; b < 2; ++b)
            for (int c = 0; c < 2; ++c)
                for (int d = 0; d < 2; ++d)
                    for (int e = 0; e < 2; ++e)
                        if (binary_ball.size() < 26)
                            binary_ball.push_back(
                                {pairs[0][a], pairs[1][b], pairs[2][c],
                                 pairs[3][d], pairs[4][e]});

    std::cout << "Full Star boundary: "
              << calculate_boundary(star, n, k) << "\n";
    std::cout << "Binary 5D slice boundary: "
              << calculate_boundary(binary_ball, n, k) << "\n";
    return 0;
}
