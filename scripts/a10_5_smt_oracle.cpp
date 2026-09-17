#include <array>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include <z3++.h>

using Vertex = std::array<int, 5>;

constexpr int kAlphabet = 10;
constexpr int kDimension = 5;
constexpr int kVolume = 26;
constexpr int kCeiling = 297;

int encode(const Vertex &vertex) {
    return std::accumulate(
        vertex.begin(), vertex.end(), 0,
        [](const int code, const int symbol) { return 10 * code + symbol; });
}

void enumerate_vertices(int position, Vertex &vertex,
                        std::array<bool, kAlphabet> &used,
                        std::vector<Vertex> &vertices) {
    if (position == kDimension) {
        vertices.push_back(vertex);
        return;
    }
    for (int symbol = 0; symbol < kAlphabet; ++symbol) {
        if (used[symbol])
            continue;
        used[symbol] = true;
        vertex[position] = symbol;
        enumerate_vertices(position + 1, vertex, used, vertices);
        used[symbol] = false;
    }
}

int main(int argc, char **argv) {
    const int timeout_seconds = argc > 1 ? std::stoi(argv[1]) : 600;
    const std::string partial_path = argc > 2 ? argv[2] : "";

    std::vector<Vertex> vertices;
    Vertex vertex{};
    std::array<bool, kAlphabet> used{};
    enumerate_vertices(0, vertex, used, vertices);

    std::unordered_map<int, int> index;
    index.reserve(vertices.size() * 2);
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id)
        index.emplace(encode(vertices[id]), id);

    z3::context context;
    z3::solver solver(context);
    z3::params parameters(context);
    parameters.set("timeout", static_cast<unsigned>(timeout_seconds * 1000));
    solver.set(parameters);

    std::vector<z3::expr> selected;
    std::vector<z3::expr> boundary;
    selected.reserve(vertices.size());
    boundary.reserve(vertices.size());
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
        selected.push_back(
            context.bool_const(("x_" + std::to_string(id)).c_str()));
        boundary.push_back(
            context.bool_const(("y_" + std::to_string(id)).c_str()));
        solver.add(implies(boundary.back(), !selected.back()));
    }

    z3::expr volume = context.int_val(0);
    z3::expr boundary_size = context.int_val(0);
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
        volume = volume +
                 z3::ite(selected[id], context.int_val(1), context.int_val(0));
        boundary_size =
            boundary_size +
            z3::ite(boundary[id], context.int_val(1), context.int_val(0));
    }
    solver.add(volume == kVolume);
    solver.add(boundary_size <= kCeiling);

    int line_count = 0;
    for (int position = 0; position < kDimension; ++position) {
        std::unordered_map<int, std::vector<int>> lines;
        for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
            int root = 0;
            for (int coordinate = 0; coordinate < kDimension; ++coordinate) {
                if (coordinate != position)
                    root = 10 * root + vertices[id][coordinate];
            }
            lines[root].push_back(id);
        }
        for (const auto &[root, members] : lines) {
            (void)root;
            const z3::expr active = context.bool_const(
                ("a_" + std::to_string(line_count++)).c_str());
            z3::expr occupied = context.bool_val(false);
            z3::expr selected_on_line = context.int_val(0);
            for (const int id : members) {
                occupied = occupied || selected[id];
                selected_on_line =
                    selected_on_line + z3::ite(selected[id], context.int_val(1),
                                               context.int_val(0));
                solver.add(implies(selected[id], active));
                solver.add(implies(active && !selected[id], boundary[id]));
            }
            solver.add(implies(active, occupied));
            solver.add(z3::ite(active, context.int_val(1),
                               context.int_val(0)) <= selected_on_line);
        }
    }
    std::cout << "A(10,5): vertices=" << vertices.size()
              << " coordinate_lines=" << line_count << '\n';

    const Vertex origin = {0, 1, 2, 3, 4};
    solver.add(selected[index.at(encode(origin))]);

    if (!partial_path.empty()) {
        std::ifstream input(partial_path);
        if (!input) {
            std::cerr << "cannot open partial state: " << partial_path << '\n';
            return 2;
        }
        int code = 0;
        while (input >> code) {
            const auto found = index.find(code);
            if (found == index.end()) {
                std::cerr << "invalid arrangement vertex: " << code << '\n';
                return 2;
            }
            solver.add(selected[found->second]);
        }
        std::cout << "partial state pinned from " << partial_path << '\n';
    }

    const z3::check_result result = solver.check();
    std::cout << "status: " << result << '\n';
    if (result != z3::sat)
        return result == z3::unknown ? 1 : 0;

    const z3::model model = solver.get_model();
    int actual_boundary = 0;
    std::vector<int> witness;
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
        if (model.eval(selected[id]).is_true())
            witness.push_back(encode(vertices[id]));
        if (model.eval(boundary[id]).is_true())
            ++actual_boundary;
    }
    std::cout << "model boundary upper certificate=" << actual_boundary << '\n';
    std::cout << "selected vertices:";
    for (const int code : witness)
        std::cout << ' ' << code;
    std::cout << '\n';
    return 0;
}
