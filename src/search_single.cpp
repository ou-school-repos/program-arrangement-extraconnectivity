// Exact CP-SAT adversary for the one-fiber coupled defect/collision ledger.
//
// Usage: ./search_single n k R
//          [--objective=slack|collision-excess|collisions]
//          [--defect-deficit=Q] [--time-limit=SECONDS] [--no-symmetry-break]
//
// collision-excess maximizes X - (m + 1)(E(R) - D).  Thus an OPTIMAL
// positive value is a finite counterexample to the proposed coarse bound
// X <= (m + 1)(E(R) - D).  The exact compensated slack is printed separately.

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
    model.AddBoolOr(literals).OnlyEnforceIf(result);
    std::vector<BoolVar> negatives;
    negatives.reserve(literals.size());
    for (const BoolVar literal : literals)
        negatives.push_back(Not(literal));
    model.AddBoolAnd(negatives).OnlyEnforceIf(Not(result));
    return result;
}

BoolVar iff_all(CpModelBuilder &model, const std::vector<BoolVar> &literals) {
    BoolVar result = model.NewBoolVar();
    model.AddBoolAnd(literals).OnlyEnforceIf(result);
    std::vector<BoolVar> negatives;
    negatives.reserve(literals.size());
    for (const BoolVar literal : literals)
        negatives.push_back(Not(literal));
    model.AddBoolOr(negatives).OnlyEnforceIf(Not(result));
    return result;
}
}  // namespace

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " n k R "
                  << "[--objective=slack|collision-excess|collisions] "
                  << "[--time-limit=SECONDS] [--no-symmetry-break]\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int size = std::atoi(argv[3]);
    std::string objective = "collision-excess";
    double time_limit = 60.0;
    bool symmetry_break = true;
    int exact_defect_deficit = -1;
    for (int argument = 4; argument < argc; ++argument) {
        const std::string option(argv[argument]);
        if (option.rfind("--objective=", 0) == 0)
            objective = option.substr(12);
        else if (option.rfind("--defect-deficit=", 0) == 0)
            exact_defect_deficit = std::stoi(option.substr(17));
        else if (option.rfind("--time-limit=", 0) == 0)
            time_limit = std::stod(option.substr(13));
        else if (option == "--no-symmetry-break")
            symmetry_break = false;
        else {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
    }
    if (k < 1 || k > n || size < 1 ||
        (objective != "slack" && objective != "collision-excess" &&
         objective != "collisions")) {
        std::cerr << "Require 1 <= k <= n, positive R, and a valid objective.\n";
        return 1;
    }

    const auto vertices = vertices_of(n, k);
    const int count = static_cast<int>(vertices.size());
    if (size > count) {
        std::cerr << "R exceeds |V(A(n,k))|.\n";
        return 1;
    }
    std::map<std::vector<int>, int> index;
    for (int id = 0; id < count; ++id)
        index[vertices[id]] = id;
    std::vector<std::vector<int>> adjacency(count);
    for (int id = 0; id < count; ++id) {
        std::vector<int> used(n, 0);
        for (int symbol : vertices[id]) used[symbol] = 1;
        for (int position = 0; position < k; ++position) {
            for (int symbol = 0; symbol < n; ++symbol) {
                if (used[symbol]) continue;
                auto neighbor = vertices[id];
                neighbor[position] = symbol;
                adjacency[id].push_back(index.at(neighbor));
            }
        }
    }

    CpModelBuilder cp;
    std::vector<BoolVar> in_set(count), ext(count);
    LinearExpr sum_members, boundary;
    for (int id = 0; id < count; ++id) {
        in_set[id] = cp.NewBoolVar();
        sum_members += in_set[id];
    }
    cp.AddEquality(sum_members, size);
    if (symmetry_break) cp.AddEquality(in_set[0], 1);

    for (int id = 0; id < count; ++id) {
        std::vector<BoolVar> adjacent;
        for (int neighbor : adjacency[id]) adjacent.push_back(in_set[neighbor]);
        const BoolVar has_neighbor = iff_any(cp, adjacent);
        ext[id] = iff_all(cp, {has_neighbor, Not(in_set[id])});
        boundary += ext[id];
    }

    // D is the root defect: R*k minus the number of occupied coordinate roots.
    LinearExpr occupied_roots;
    for (int position = 0; position < k; ++position) {
        std::map<std::vector<int>, std::vector<int>> root_members;
        for (int id = 0; id < count; ++id) {
            auto root = vertices[id];
            root.erase(root.begin() + position);
            root_members[root].push_back(id);
        }
        for (const auto &[root, members] : root_members) {
            (void)root;
            std::vector<BoolVar> literals;
            for (int id : members) literals.push_back(in_set[id]);
            occupied_roots += iff_any(cp, literals);
        }
    }

    const std::int64_t m = n - k;
    const std::int64_t e = e_seq(size);
    const std::int64_t c = c_constant(size);
    const std::int64_t total_slots = static_cast<std::int64_t>(size) * k;
    // X = total coordinate boundary - ordinary external boundary.
    const LinearExpr defect = total_slots - occupied_roots;
    const LinearExpr collisions = (m + 1) * occupied_roots - total_slots - boundary;
    const LinearExpr defect_deficit = e - defect;
    const LinearExpr collision_excess = collisions - (m + 1) * defect_deficit;
    const LinearExpr slack = boundary - rhs(n, k, size);

    if (exact_defect_deficit >= 0)
        cp.AddEquality(defect_deficit, exact_defect_deficit);

    if (objective == "slack") cp.Minimize(slack);
    else if (objective == "collisions") cp.Maximize(collisions);
    else cp.Maximize(collision_excess);

    SatParameters parameters;
    parameters.set_num_search_workers(8);
    parameters.set_max_time_in_seconds(time_limit);
    Model model;
    model.Add(NewSatParameters(parameters));
    std::cout << "Building single-fiber model for A(" << n << ',' << k
              << ") R=" << size << " (N=" << count << "; m=" << m
              << "; rhs=" << rhs(n, k, size) << ")...\n";
    std::cout << "Searching for objective " << objective << " (limit "
              << time_limit << " s; symmetry " << (symmetry_break ? "on" : "off")
              << "; defect deficit "
              << (exact_defect_deficit >= 0 ? std::to_string(exact_defect_deficit)
                                            : "unconstrained")
              << ")...\n";
    const CpSolverResponse response = SolveCpModel(cp.Build(), &model);
    std::cout << "status: " << CpSolverStatus_Name(response.status()) << '\n';
    if (response.status() != CpSolverStatus::OPTIMAL &&
        response.status() != CpSolverStatus::FEASIBLE)
        return 0;
    const auto value = [&](const LinearExpr &expr) {
        return SolutionIntegerValue(response, expr);
    };
    const auto d = value(defect);
    const auto x = value(collisions);
    const auto delta_d = value(defect_deficit);
    const auto b = value(boundary);
    std::cout << "objective value = " << response.objective_value() << '\n';
    std::cout << "|dF| = " << b << ", S(F) = " << value(slack) << '\n';
    std::cout << "D(F) = " << d << ", E(R)-D(F) = " << delta_d
              << ", X(F) = " << x << '\n';
    std::cout << "X - (m+1)(E-D) = " << value(collision_excess)
              << ", compensated ledger = "
              << (c - e + (m + 1) * delta_d - x) << '\n';
    std::cout << "F = {";
    for (int id = 0; id < count; ++id) {
        if (!SolutionIntegerValue(response, in_set[id])) continue;
        std::cout << ' ';
        for (int symbol : vertices[id]) std::cout << symbol;
    }
    std::cout << " }\n";
    if (response.status() != CpSolverStatus::OPTIMAL)
        std::cout << "WARNING: FEASIBLE is an incumbent, not a certified extremum.\n";
}
