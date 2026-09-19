#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
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

int encode(const Vertex &vertex) {
    return std::accumulate(
        vertex.begin(), vertex.end(), 0,
        [](const int code, const int symbol) { return 10 * code + symbol; });
}

void enumerate_vertices(int position, Vertex &vertex,
                        std::array<bool, 10> &used,
                        std::vector<Vertex> &vertices) {
    if (position == 5) {
        vertices.push_back(vertex);
        return;
    }
    for (int symbol = 0; symbol < 10; ++symbol) {
        if (used[symbol])
            continue;
        used[symbol] = true;
        vertex[position] = symbol;
        enumerate_vertices(position + 1, vertex, used, vertices);
        used[symbol] = false;
    }
}

int main(int argc, char **argv) {
    constexpr int n = 10;
    constexpr int k = 5;
    constexpr int volume = 26;
    constexpr int cutoff = 297;

    int start_lb = 0;
    double time_limit_seconds = 3600.0;
    if (argc > 1) {
        try {
            start_lb = std::stoi(argv[1]);
        } catch (const std::exception &) {
            std::cerr << "usage: " << argv[0] << " [proven-lower-bound]\n";
            return 2;
        }
        if (start_lb < 0 || start_lb > cutoff) {
            std::cerr << "lower bound must be between 0 and " << cutoff << "\n";
            return 2;
        }
    }
    if (argc > 2) {
        try {
            time_limit_seconds = std::stod(argv[2]);
        } catch (const std::exception &) {
            std::cerr << "usage: " << argv[0]
                      << " [proven-lower-bound] [time-limit-seconds]\n";
            return 2;
        }
        if (time_limit_seconds <= 0) {
            std::cerr << "time limit must be positive\n";
            return 2;
        }
    }

    std::vector<Vertex> vertices;
    Vertex vertex{};
    std::array<bool, 10> used{};
    enumerate_vertices(0, vertex, used, vertices);

    std::unordered_map<int, int> index;
    index.reserve(vertices.size() * 2);
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id)
        index.emplace(encode(vertices[id]), id);

    std::vector<std::vector<int>> adjacency(vertices.size());
    for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
        std::array<bool, n> occupied{};
        for (const int symbol : vertices[id])
            occupied[symbol] = true;
        for (int position = 0; position < k; ++position) {
            for (int symbol = 0; symbol < n; ++symbol) {
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
    if (start_lb > 0)
        std::cout << "resuming with proven lower bound >= " << start_lb << "\n";

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
    model.AddEquality(volume_expr, volume);
    model.AddLessOrEqual(boundary_expr, cutoff);
    if (start_lb > 0)
        model.AddGreaterOrEqual(boundary_expr, start_lb);
    model.Minimize(boundary_expr);

    // One pinned vertex is symmetry-safe because A(10,5) is vertex-transitive.
    const Vertex origin = {0, 1, 2, 3, 4};
    model.AddEquality(selected[index.at(encode(origin))], 1);

    for (std::size_t u = 0; u < adjacency.size(); ++u) {
        for (const int v : adjacency[u])
            model.AddGreaterOrEqual(boundary[v], selected[u] - selected[v]);
    }

    Model solver;
    SatParameters parameters;
    parameters.set_num_search_workers(6);
    parameters.set_max_time_in_seconds(time_limit_seconds);
    parameters.set_log_search_progress(true);
    solver.Add(NewSatParameters(parameters));
    auto boundary_size =
        [&](const operations_research::sat::CpSolverResponse &response) {
            std::vector<bool> selected_flags(vertices.size(), false);
            for (std::size_t id = 0; id < selected.size(); ++id)
                selected_flags[id] =
                    SolutionBooleanValue(response, selected[id]);
            std::vector<bool> external(vertices.size(), false);
            for (std::size_t u = 0; u < adjacency.size(); ++u) {
                if (!selected_flags[u])
                    continue;
                for (const int v : adjacency[u])
                    if (!selected_flags[v])
                        external[v] = true;
            }
            return static_cast<int>(
                std::count(external.begin(), external.end(), true));
        };
    solver.Add(NewFeasibleSolutionObserver(
        [&](const operations_research::sat::CpSolverResponse &response) {
            const char *temporary = "checkpoint_solution.txt.tmp";
            std::ofstream output(temporary);
            if (!output)
                return;
            output << "boundary " << boundary_size(response) << '\n';
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
        std::cout << "boundary: " << boundary_size(response) << '\n';
    return 0;
}
