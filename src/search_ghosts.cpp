// Exact CP-SAT search for the maximum number of distance-2 shared-boundary
// targets ("ghosts") between two disjoint tight fibers in A(n,k).
//
// An OPTIMAL result is a certificate for this finite encoded cell only; it is
// not an algebraic proof for all parameters. The ghost definition matches
// compute_T in src/check_amortized_slack.cpp.
//
// Usage: ./search_ghosts n k c_a c_b

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"

namespace {

using operations_research::Model;
using operations_research::sat::BoolVar;
using operations_research::sat::CpModelBuilder;
using operations_research::sat::CpSolverResponse;
using operations_research::sat::CpSolverStatus;
using operations_research::sat::LinearExpr;
using operations_research::sat::NewSatParameters;
using operations_research::sat::Not;
using operations_research::sat::SatParameters;
using operations_research::sat::SolutionIntegerValue;
using operations_research::sat::SolveCpModel;

std::int64_t e_seq(int size) {
    std::int64_t total = 0;
    for (int value = 0; value < size; ++value)
        total += __builtin_popcount(static_cast<unsigned>(value));
    return total;
}

// This is the repository's C(R), not sum_{i<R}(i-popcount(i)).
std::int64_t c_constant(int size) {
    if (size == 0)
        return 0;
    std::int64_t total = size - 1 - e_seq(size);
    for (int value = 1; value < size; ++value)
        total += 32 - __builtin_clz(static_cast<unsigned>(value));
    return total;
}

std::int64_t rhs(int n, int k, int size) {
    return (static_cast<std::int64_t>(size) * k - e_seq(size)) * (n - k) -
           c_constant(size);
}

std::vector<std::vector<int>> vertices_of(int n, int k) {
    std::vector<std::vector<int>> vertices;
    std::vector<int> used(n, 0), current;
    std::function<void()> visit = [&] {
        if (static_cast<int>(current.size()) == k) {
            vertices.push_back(current);
            return;
        }
        for (int symbol = 0; symbol < n; ++symbol) {
            if (used[symbol])
                continue;
            used[symbol] = 1;
            current.push_back(symbol);
            visit();
            current.pop_back();
            used[symbol] = 0;
        }
    };
    visit();
    return vertices;
}

BoolVar iff_any(CpModelBuilder &model, const std::vector<BoolVar> &literals) {
    BoolVar result = model.NewBoolVar();
    if (literals.empty()) {
        model.AddEquality(result, 0);
        return result;
    }
    model.AddBoolOr(literals).OnlyEnforceIf(result);
    std::vector<operations_research::sat::Literal> negatives;
    negatives.reserve(literals.size());
    for (BoolVar literal : literals)
        negatives.push_back(Not(literal));
    model.AddBoolAnd(negatives).OnlyEnforceIf(Not(result));
    return result;
}

BoolVar iff_all(CpModelBuilder &model,
                const std::vector<operations_research::sat::Literal> &literals) {
    BoolVar result = model.NewBoolVar();
    model.AddBoolAnd(literals).OnlyEnforceIf(result);
    std::vector<operations_research::sat::Literal> negated;
    negated.reserve(literals.size());
    for (const auto literal : literals)
        negated.push_back(literal.Negated());
    model.AddBoolOr(negated).OnlyEnforceIf(Not(result));
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " n k c_a c_b\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int ca = std::atoi(argv[3]);
    const int cb = std::atoi(argv[4]);
    if (k < 1 || k > n || ca < 1 || cb < 1) {
        std::cerr << "Require 1 <= k <= n and positive fiber sizes.\n";
        return 1;
    }

    const auto vertices = vertices_of(n, k);
    const int count = static_cast<int>(vertices.size());
    std::map<std::vector<int>, int> index;
    for (int id = 0; id < count; ++id)
        index[vertices[id]] = id;
    std::vector<std::vector<int>> adjacency(count);
    for (int id = 0; id < count; ++id) {
        std::vector<int> used(n, 0);
        for (const int symbol : vertices[id])
            used[symbol] = 1;
        for (int position = 0; position < k; ++position) {
            for (int symbol = 0; symbol < n; ++symbol) {
                if (used[symbol])
                    continue;
                auto neighbor = vertices[id];
                neighbor[position] = symbol;
                adjacency[id].push_back(index.at(neighbor));
            }
        }
    }

    std::cout << "Building CP-SAT model for A(" << n << ',' << k << ") c_a="
              << ca << " c_b=" << cb << " (N=" << count << "; targets "
              << rhs(n, k, ca) << ", " << rhs(n, k, cb) << ")...\n";
    CpModelBuilder cp;
    std::vector<BoolVar> in_a(count), in_b(count), ext_a(count), ext_b(count),
        ghosts(count);
    LinearExpr sum_a, sum_b, sum_ext_a, sum_ext_b, sum_ghosts;
    for (int id = 0; id < count; ++id) {
        in_a[id] = cp.NewBoolVar();
        in_b[id] = cp.NewBoolVar();
        cp.AddAtMostOne(in_a[id], in_b[id]);
        sum_a += in_a[id];
        sum_b += in_b[id];
    }
    cp.AddEquality(sum_a, ca);
    cp.AddEquality(sum_b, cb);

    for (int id = 0; id < count; ++id) {
        std::vector<BoolVar> adjacent_a, adjacent_b;
        for (const int neighbor : adjacency[id]) {
            adjacent_a.push_back(in_a[neighbor]);
            adjacent_b.push_back(in_b[neighbor]);
        }
        const BoolVar has_a = iff_any(cp, adjacent_a);
        const BoolVar has_b = iff_any(cp, adjacent_b);
        ext_a[id] = iff_all(cp, {has_a, Not(in_a[id])});
        ext_b[id] = iff_all(cp, {has_b, Not(in_b[id])});
        sum_ext_a += ext_a[id];
        sum_ext_b += ext_b[id];
    }
    cp.AddEquality(sum_ext_a, rhs(n, k, ca));
    cp.AddEquality(sum_ext_b, rhs(n, k, cb));

    for (int target = 0; target < count; ++target) {
        std::vector<BoolVar> explainers;
        for (const int u : adjacency[target]) {
            for (const int v : adjacency[target]) {
                if (u == v || std::find(adjacency[u].begin(), adjacency[u].end(), v) == adjacency[u].end())
                    continue;
                explainers.push_back(iff_all(cp, {in_a[u], in_b[v]}));
            }
        }
        const BoolVar distance_one = iff_any(cp, explainers);
        ghosts[target] = iff_all(cp, {ext_a[target], ext_b[target], Not(distance_one)});
        sum_ghosts += ghosts[target];
    }
    cp.Maximize(sum_ghosts);

    Model model;
    SatParameters parameters;
    parameters.set_num_search_workers(8);
    model.Add(NewSatParameters(parameters));
    std::cout << "Searching for maximum ghost count T...\n";
    const CpSolverResponse response = SolveCpModel(cp.Build(), &model);
    std::cout << "status: " << CpSolverStatus_Name(response.status()) << '\n';
    if (response.status() != CpSolverStatus::OPTIMAL && response.status() != CpSolverStatus::FEASIBLE)
        return 0;
    std::cout << "T = " << response.objective_value() << '\n';
    const auto print_set = [&](const char *name, const std::vector<BoolVar> &set) {
        std::cout << name << " = {";
        for (int id = 0; id < count; ++id) {
            if (!SolutionIntegerValue(response, set[id]))
                continue;
            std::cout << ' ';
            for (const int symbol : vertices[id])
                std::cout << symbol;
        }
        std::cout << " }\n";
    };
    print_set("F_a", in_a);
    print_set("F_b", in_b);
    print_set("ghosts", ghosts);
    if (response.status() != CpSolverStatus::OPTIMAL)
        std::cout << "WARNING: FEASIBLE is an incumbent, not a certified maximum.\n";
}
