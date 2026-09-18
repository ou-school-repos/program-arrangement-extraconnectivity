#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"

using operations_research::sat::BoolVar;
using operations_research::sat::CpModelBuilder;
using operations_research::sat::CpSolverStatus;
using operations_research::sat::CpSolverStatus_Name;
using operations_research::sat::IntVar;
using operations_research::sat::LinearExpr;
using operations_research::sat::Model;
using operations_research::sat::NewFeasibleSolutionObserver;
using operations_research::sat::NewSatParameters;
using operations_research::sat::SatParameters;
using operations_research::sat::SolutionBooleanValue;
using operations_research::sat::SolveCpModel;

using Vertex = std::array<int, 5>;

constexpr int kSymbols = 10;
constexpr int kSize = 5;
constexpr int kVolume = 26;
constexpr int kSliceBoundary = 298;

int encode(const Vertex &vertex) {
    return std::accumulate(
        vertex.begin(), vertex.end(), 0,
        [](const int code, const int symbol) { return 10 * code + symbol; });
}

void enumerate_vertices(int position, Vertex &vertex,
                        std::array<bool, kSymbols> &used,
                        std::vector<Vertex> &vertices) {
    if (position == kSize) {
        vertices.push_back(vertex);
        return;
    }
    for (int symbol = 0; symbol < kSymbols; ++symbol) {
        if (used[symbol])
            continue;
        used[symbol] = true;
        vertex[position] = symbol;
        enumerate_vertices(position + 1, vertex, used, vertices);
        used[symbol] = false;
    }
}

std::vector<int> hamming_slice() {
    std::vector<int> members;
    for (int index = 0; index < kVolume; ++index) {
        Vertex vertex{};
        for (int bit = 0; bit < kSize; ++bit)
            vertex[bit] = (index >> bit) & 1 ? kSize + bit : bit;
        members.push_back(encode(vertex));
    }
    return members;
}

int main(int argc, char **argv) {
    int floor = 0;
    double time_limit = 0.0;
    bool line_cuts = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "cuts") {
            line_cuts = true;
            continue;
        }
        try {
            const double value = std::stod(argument);
            if (i == 1)
                floor = static_cast<int>(value);
            else
                time_limit = value;
        } catch (const std::exception &) {
            std::cerr << "usage: " << argv[0]
                      << " [proven-lower-bound] [time-limit-seconds] [cuts]\n";
            return 2;
        }
    }
    if (floor < 0 || floor > kSliceBoundary) {
        std::cerr << "lower bound must be between 0 and " << kSliceBoundary
                  << "\n";
        return 2;
    }

    std::vector<Vertex> vertices;
    Vertex vertex{};
    std::array<bool, kSymbols> used{};
    enumerate_vertices(0, vertex, used, vertices);

    std::unordered_map<int, int> index;
    index.reserve(vertices.size() * 2);
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id)
        index.emplace(encode(vertices[id]), id);

    std::vector<std::vector<int>> adjacency(vertices.size());
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
        std::array<bool, kSymbols> occupied{};
        for (const int symbol : vertices[id])
            occupied[symbol] = true;
        for (int position = 0; position < kSize; ++position) {
            for (int symbol = 0; symbol < kSymbols; ++symbol) {
                if (occupied[symbol])
                    continue;
                Vertex neighbor = vertices[id];
                neighbor[position] = symbol;
                adjacency[id].push_back(index.at(encode(neighbor)));
            }
        }
    }

    std::cout << "A(10,5): vertices=" << vertices.size() << " directed_edges=";
    const std::size_t edge_count =
        std::accumulate(adjacency.begin(), adjacency.end(), std::size_t{0},
                        [](const std::size_t total, const auto &neighbors) {
                            return total + neighbors.size();
                        });
    std::cout << edge_count << "\n";

    std::vector<char> selected_flag(vertices.size());
    for (const int code : hamming_slice())
        selected_flag[index.at(code)] = true;
    std::vector<int> selected_list;
    selected_list.reserve(kVolume);
    std::vector<int> vertex_ids(vertices.size());
    std::iota(vertex_ids.begin(), vertex_ids.end(), 0);
    std::copy_if(vertex_ids.begin(), vertex_ids.end(),
                 std::back_inserter(selected_list),
                 [&](const int id) { return selected_flag[id] != 0; });

    std::vector<char> boundary_flag(vertices.size(), false);
    for (const int id : selected_list)
        for (const int neighbor : adjacency[id])
            if (!selected_flag[neighbor])
                boundary_flag[neighbor] = true;
    const int boundary_count = static_cast<int>(
        std::count_if(boundary_flag.begin(), boundary_flag.end(),
                      [](const char flag) { return flag != 0; }));
    if (boundary_count != kSliceBoundary) {
        std::cerr << "slice boundary verification failed: " << boundary_count
                  << " != " << kSliceBoundary << "\n";
        return 2;
    }

    std::vector<int> external_count(kSize, 0);
    int incidence_total = 0;
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
        if (!boundary_flag[id])
            continue;
        for (int position = 0; position < kSize; ++position) {
            const bool reached =
                std::any_of(adjacency[id].begin(), adjacency[id].end(),
                            [&](const int neighbor) {
                                return selected_flag[neighbor] &&
                                       vertices[neighbor][position] !=
                                           vertices[id][position];
                            });
            if (reached) {
                ++external_count[position];
                ++incidence_total;
            }
        }
    }
    const int uniqueness = incidence_total - boundary_count;
    if ((incidence_total + kVolume * kSize) % (kSize + 1) != 0) {
        std::cerr << "coordinate-edge identity does not integral-invert\n";
        return 2;
    }
    const int roots = (incidence_total + kVolume * kSize) / (kSize + 1);
    std::cout << "slice |ext|=" << boundary_count
              << " coord_incidences=" << incidence_total
              << " cross=" << uniqueness << " roots=" << roots << " residual=("
              << boundary_count + uniqueness << ")\n";
    for (int position = 0; position < kSize; ++position)
        std::cout << "  coord " << position
                  << ": external=" << external_count[position] << "\n";
    if (floor > 0)
        std::cout << "resuming with proven lower bound >= " << floor << "\n";

    CpModelBuilder model;
    std::vector<BoolVar> selected;
    std::vector<BoolVar> boundary;
    selected.reserve(vertices.size());
    boundary.reserve(vertices.size());
    for (std::size_t id = 0; id < vertices.size(); ++id) {
        selected.push_back(model.NewBoolVar());
        boundary.push_back(model.NewBoolVar());
        model.AddImplication(boundary.back(), selected.back().Not());
    }

    const LinearExpr volume_expr =
        std::accumulate(selected.begin(), selected.end(), LinearExpr{},
                        [](LinearExpr sum, const BoolVar variable) {
                            sum += variable;
                            return sum;
                        });
    const LinearExpr boundary_expr =
        std::accumulate(boundary.begin(), boundary.end(), LinearExpr{},
                        [](LinearExpr sum, const BoolVar variable) {
                            sum += variable;
                            return sum;
                        });
    model.AddEquality(volume_expr, kVolume);
    if (floor > 0)
        model.AddGreaterOrEqual(boundary_expr, floor);

    for (std::size_t id = 0; id < vertices.size(); ++id) {
        const LinearExpr neighbor_sum = std::accumulate(
            adjacency[id].begin(), adjacency[id].end(), LinearExpr{},
            [&](LinearExpr sum, const int neighbor) {
                sum += selected[neighbor];
                return sum;
            });
        model.AddLessOrEqual(boundary[id], neighbor_sum);
    }

    if (!line_cuts) {
        for (std::size_t u = 0; u < adjacency.size(); ++u) {
            for (const int v : adjacency[u])
                model.AddGreaterOrEqual(boundary[v], selected[u] - selected[v]);
        }
    }

    if (line_cuts) {
        std::vector<std::vector<int>> lines;
        lines.reserve(kSize * 5040);
        for (int position = 0; position < kSize; ++position) {
            std::vector<std::array<int, kSize>> line_roots;
            std::array<int, kSize> root{};
            std::array<bool, kSymbols> root_used_symbols{};
            std::function<void(int, int)> build = [&](int cursor, int depth) {
                if (cursor == kSize) {
                    if (depth == kSize - 1)
                        line_roots.push_back(root);
                    return;
                }
                if (cursor == position) {
                    build(cursor + 1, depth);
                    return;
                }
                for (int symbol = 0; symbol < kSymbols; ++symbol) {
                    if (root_used_symbols[symbol])
                        continue;
                    root_used_symbols[symbol] = true;
                    root[cursor] = symbol;
                    build(cursor + 1, depth + 1);
                    root_used_symbols[symbol] = false;
                }
            };
            build(0, 0);
            for (const std::array<int, kSize> &line_root : line_roots) {
                std::vector<int> members;
                members.reserve(kSymbols);
                std::array<bool, kSymbols> root_used{};
                for (int cursor = 0; cursor < kSize; ++cursor)
                    if (cursor != position)
                        root_used[line_root[cursor]] = true;
                for (int symbol = 0; symbol < kSymbols; ++symbol) {
                    if (root_used[symbol])
                        continue;
                    std::array<int, kSize> member = line_root;
                    member[position] = symbol;
                    members.push_back(index.at(encode(member)));
                }
                lines.push_back(std::move(members));
            }
        }
        std::cout << "conditional line cuts: " << lines.size() << " lines\n";

        std::vector<BoolVar> occupied;
        occupied.reserve(lines.size());
        for (std::size_t l = 0; l < lines.size(); ++l)
            occupied.push_back(model.NewBoolVar());
        for (std::size_t l = 0; l < lines.size(); ++l) {
            const std::vector<int> &line = lines[l];
            LinearExpr x_sum;
            for (const int id : line) {
                x_sum += selected[id];
                model.AddImplication(selected[id], occupied[l]);
                model.AddGreaterOrEqual(boundary[id],
                                        occupied[l] - selected[id]);
            }
            model.AddLessOrEqual(occupied[l], x_sum);
        }
    }

    model.Minimize(boundary_expr);

    for (int id = 0; id < static_cast<int>(vertices.size()); ++id)
        model.AddHint(selected[id], selected_flag[id] != 0);

    const Vertex origin = {0, 1, 2, 3, 4};
    model.AddEquality(selected[index.at(encode(origin))], 1);

    Model solver;
    SatParameters parameters;
    parameters.set_num_search_workers(6);
    if (time_limit > 0.0)
        parameters.set_max_time_in_seconds(time_limit);
    parameters.set_log_search_progress(true);
    solver.Add(NewSatParameters(parameters));
    solver.Add(NewFeasibleSolutionObserver(
        [&](const operations_research::sat::CpSolverResponse &response) {
            const char *temporary = "checkpoint_solution.txt.tmp";
            std::ofstream output(temporary);
            if (!output)
                return;
            output << "boundary " << response.objective_value() << '\n';
            for (std::size_t id = 0; id < selected.size(); ++id) {
                if (!SolutionBooleanValue(response, selected[id]))
                    continue;
                const Vertex &saved = vertices[id];
                for (int symbol : saved)
                    output << symbol;
                output << '\n';
            }
            output.close();
            std::rename(temporary, "checkpoint_solution.txt");
        }));
    const auto response = SolveCpModel(model.Build(), &solver);

    std::cout << "status: " << CpSolverStatus_Name(response.status()) << '\n';
    if (response.status() == CpSolverStatus::OPTIMAL ||
        response.status() == CpSolverStatus::FEASIBLE)
        std::cout << "boundary: " << response.objective_value() << '\n';
    return 0;
}
