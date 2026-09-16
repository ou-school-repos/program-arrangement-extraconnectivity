// Exact CP-SAT adversary for two fibers in A(n,k).
//
// An OPTIMAL result is a certificate for this finite encoded cell only; it is
// not an algebraic proof for all parameters. The ghost definition matches
// compute_T in src/check_amortized_slack.cpp.
//
// Usage: ./search_ghosts n k c_a c_b [--objective=ghosts|loss|amortized]
//        [--time-limit=SECONDS] [--no-symmetry-break]

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/sat_parameters.pb.h"

namespace {

using operations_research::sat::BoolVar;
using operations_research::sat::CpModelBuilder;
using operations_research::sat::CpSolverResponse;
using operations_research::sat::CpSolverStatus;
using operations_research::sat::CpSolverStatus_Name;
using operations_research::sat::LinearExpr;
using operations_research::sat::Model;
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
    std::vector<BoolVar> negatives;
    negatives.reserve(literals.size());
    for (BoolVar literal : literals)
        negatives.push_back(Not(literal));
    model.AddBoolAnd(negatives).OnlyEnforceIf(Not(result));
    return result;
}

BoolVar iff_all(CpModelBuilder &model, const std::vector<BoolVar> &literals) {
    BoolVar result = model.NewBoolVar();
    model.AddBoolAnd(literals).OnlyEnforceIf(result);
    std::vector<BoolVar> negated;
    negated.reserve(literals.size());
    for (const auto literal : literals)
        negated.push_back(literal.Not());
    model.AddBoolOr(negated).OnlyEnforceIf(Not(result));
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0]
                  << " n k c_a c_b [--objective=ghosts|loss|amortized]"
                  << " [--time-limit=SECONDS] [--no-symmetry-break]\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int ca = std::atoi(argv[3]);
    const int cb = std::atoi(argv[4]);
    std::string objective = "ghosts";
    double time_limit_seconds = 60.0;
    bool symmetry_break = true;
    for (int argument = 5; argument < argc; ++argument) {
        const std::string option(argv[argument]);
        constexpr const char *kObjective = "--objective=";
        constexpr const char *kTimeLimit = "--time-limit=";
        if (option.rfind(kObjective, 0) == 0) {
            objective =
                option.substr(std::char_traits<char>::length(kObjective));
        } else if (option.rfind(kTimeLimit, 0) == 0) {
            try {
                time_limit_seconds = std::stod(
                    option.substr(std::char_traits<char>::length(kTimeLimit)));
            } catch (const std::exception &) {
                std::cerr << "Invalid time limit: " << option << '\n';
                return 1;
            }
        } else if (option == "--no-symmetry-break") {
            symmetry_break = false;
        } else {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
    }
    if (objective != "ghosts" && objective != "loss" &&
        objective != "amortized") {
        std::cerr << "Objective must be ghosts, loss, or amortized.\n";
        return 1;
    }
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

    std::cout << "Building CP-SAT model for A(" << n << ',' << k
              << ") c_a=" << ca << " c_b=" << cb << " (N=" << count
              << "; targets " << rhs(n, k, ca) << ", " << rhs(n, k, cb)
              << ")...\n";
    CpModelBuilder cp;
    std::vector<BoolVar> in_a(count), in_b(count), ext_a(count), ext_b(count),
        ghosts(count);
    LinearExpr sum_a, sum_b, sum_ext_a, sum_ext_b, sum_ghosts;
    for (int id = 0; id < count; ++id) {
        in_a[id] = cp.NewBoolVar();
        in_b[id] = cp.NewBoolVar();
        cp.AddAtMostOne({in_a[id], in_b[id]});
        sum_a += in_a[id];
        sum_b += in_b[id];
    }
    cp.AddEquality(sum_a, ca);
    cp.AddEquality(sum_b, cb);
    // A(n,k) is vertex-transitive.  Since c_a > 0, an automorphism can map
    // any chosen F_a vertex to vertices[0], so this loses no orbit.
    if (symmetry_break)
        cp.AddEquality(in_a[0], 1);

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
    // The loss and ghost experiments condition on individually tight fibers.
    // The amortized objective deliberately drops that condition: its objective
    // is algebraically rhs(R)-|d(F_a union F_b)|, i.e. the original global
    // Proposition 5.3 adversary for this finite cell.
    if (objective != "amortized") {
        cp.AddEquality(sum_ext_a, rhs(n, k, ca));
        cp.AddEquality(sum_ext_b, rhs(n, k, cb));
    }

    std::vector<BoolVar> shared(count), b_ba(count), b_ab(count);
    LinearExpr sum_shared, sum_b_ba, sum_b_ab;
    for (int target = 0; target < count; ++target) {
        shared[target] = iff_all(cp, {ext_a[target], ext_b[target]});
        b_ba[target] = iff_all(cp, {ext_a[target], in_b[target]});
        b_ab[target] = iff_all(cp, {ext_b[target], in_a[target]});
        sum_shared += shared[target];
        sum_b_ba += b_ba[target];
        sum_b_ab += b_ab[target];
        std::vector<BoolVar> explainers;
        for (const int u : adjacency[target]) {
            for (const int v : adjacency[target]) {
                if (u == v ||
                    std::find(adjacency[u].begin(), adjacency[u].end(), v) ==
                        adjacency[u].end())
                    continue;
                explainers.push_back(iff_all(cp, {in_a[u], in_b[v]}));
            }
        }
        const BoolVar distance_one = iff_any(cp, explainers);
        ghosts[target] = iff_all(cp, {shared[target], Not(distance_one)});
        sum_ghosts += ghosts[target];
    }
    const std::int64_t recombination_slack =
        rhs(n, k, ca) + rhs(n, k, cb) - rhs(n, k, ca + cb);
    LinearExpr total_loss = sum_shared + sum_b_ba + sum_b_ab;
    if (objective == "ghosts") {
        cp.Maximize(sum_ghosts);
    } else if (objective == "loss") {
        cp.Maximize(total_loss);
    } else {
        // total_loss - Delta - S(F_a) - S(F_b)
        cp.Maximize(total_loss - recombination_slack -
                    (sum_ext_a - rhs(n, k, ca)) - (sum_ext_b - rhs(n, k, cb)));
    }

    Model model;
    SatParameters parameters;
    parameters.set_num_search_workers(8);
    parameters.set_max_time_in_seconds(time_limit_seconds);
    model.Add(NewSatParameters(parameters));
    std::cout << "Searching for objective " << objective << " (limit "
              << time_limit_seconds << " s; symmetry "
              << (symmetry_break ? "on" : "off") << ")...\n";
    const CpSolverResponse response = SolveCpModel(cp.Build(), &model);
    std::cout << "status: " << CpSolverStatus_Name(response.status()) << '\n';
    if (response.status() != CpSolverStatus::OPTIMAL &&
        response.status() != CpSolverStatus::FEASIBLE)
        return 0;
    std::cout << "objective value = " << response.objective_value() << '\n';
    const auto value_of = [&](const LinearExpr &expression) {
        return SolutionIntegerValue(response, expression);
    };
    const std::int64_t i = value_of(sum_shared);
    const std::int64_t b_ba_value = value_of(sum_b_ba);
    const std::int64_t b_ab_value = value_of(sum_b_ab);
    const std::int64_t t = value_of(sum_ghosts);
    const std::int64_t slack_a = value_of(sum_ext_a) - rhs(n, k, ca);
    const std::int64_t slack_b = value_of(sum_ext_b) - rhs(n, k, cb);
    std::cout << "I = " << i << ", B_ba = " << b_ba_value
              << ", B_ab = " << b_ab_value << ", T = " << t << '\n';
    std::cout << "loss = " << (i + b_ba_value + b_ab_value)
              << ", Delta = " << recombination_slack << ", S(F_a) = " << slack_a
              << ", S(F_b) = " << slack_b << '\n';
    const auto print_set = [&](const char *name,
                               const std::vector<BoolVar> &set) {
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
        std::cout
            << "WARNING: FEASIBLE is an incumbent, not a certified maximum.\n";
}
