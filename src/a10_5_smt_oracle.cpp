#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
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
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0]
                  << " [timeout-seconds] [partial-state-file]\n"
                  << "       " << argv[0]
                  << " [--timeout=SECONDS] [--partial=FILE]\n";
        return 0;
    }

    int timeout_seconds = 600;
    std::string partial_path;
    for (int argument_index = 1; argument_index < argc; ++argument_index) {
        const std::string argument = argv[argument_index];
        const std::string timeout_prefix = "--timeout=";
        const std::string partial_prefix = "--partial=";
        try {
            if (argument.rfind(timeout_prefix, 0) == 0) {
                timeout_seconds =
                    std::stoi(argument.substr(timeout_prefix.size()));
            } else if (argument.rfind(partial_prefix, 0) == 0) {
                partial_path = argument.substr(partial_prefix.size());
            } else {
                try {
                    timeout_seconds = std::stoi(argument);
                } catch (const std::invalid_argument &) {
                    if (partial_path.empty())
                        partial_path = argument;
                    else
                        throw;
                }
            }
        } catch (const std::exception &) {
            std::cerr << "usage: " << argv[0]
                      << " [timeout-seconds] [partial-state-file]\n"
                      << "       " << argv[0]
                      << " [--timeout=SECONDS] [--partial=FILE]\n";
            return 2;
        }
    }
    if (timeout_seconds < 0) {
        std::cerr << "timeout must be nonnegative\n";
        return 2;
    }
    if (timeout_seconds == 0)
        std::cout << "timeout: unlimited\n";

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
    std::vector<z3::expr> active_lines;
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
            active_lines.push_back(active);
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
    std::vector<Z3_ast> aggregate_args;
    std::vector<int> aggregate_coefficients;
    aggregate_args.reserve(active_lines.size() + boundary.size());
    aggregate_coefficients.reserve(active_lines.size() + boundary.size());
    for (const z3::expr &active : active_lines) {
        aggregate_args.push_back(active);
        aggregate_coefficients.push_back(6);
    }
    for (const z3::expr &external : boundary) {
        aggregate_args.push_back(external);
        aggregate_coefficients.push_back(-5);
    }
    solver.add(z3::expr(
        context,
        Z3_mk_pble(context, static_cast<unsigned>(aggregate_args.size()),
                   aggregate_args.data(), aggregate_coefficients.data(), 130)));
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
    std::vector<bool> selected_flags(vertices.size(), false);
    std::vector<int> witness;
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
        selected_flags[id] = model.eval(selected[id]).is_true();
        if (selected_flags[id])
            witness.push_back(encode(vertices[id]));
    }
    std::vector<bool> external_flags(vertices.size(), false);
    int line_index = 0;
    for (int position = 0; position < kDimension; ++position) {
        std::unordered_map<int, std::vector<int>> lines;
        for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
            int root = 0;
            for (int coordinate = 0; coordinate < kDimension; ++coordinate)
                if (coordinate != position)
                    root = 10 * root + vertices[id][coordinate];
            lines[root].push_back(id);
        }
        for (const auto &[root, members] : lines) {
            (void)root;
            if (!model.eval(active_lines[line_index++]).is_true())
                continue;
            for (const int id : members)
                if (!selected_flags[id])
                    external_flags[id] = true;
        }
    }
    const int actual_boundary = static_cast<int>(
        std::count(external_flags.begin(), external_flags.end(), true));
    if (actual_boundary > kCeiling || witness.size() != kVolume) {
        std::cerr << "independent witness verification failed\n";
        return 2;
    }
    std::cout << "model boundary upper certificate=" << actual_boundary << '\n';
    std::cout << "selected vertices:";
    for (const int code : witness)
        std::cout << ' ' << code;
    std::cout << '\n';
    return 0;
}
