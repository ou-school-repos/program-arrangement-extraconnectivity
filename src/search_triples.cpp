// CP-SAT adversary for the tripartite boundary interface
// J(A,B;C) = d(A)+d(B)+d(C)-d(AuB)-d(AuC)-d(BuC)+d(AuBuC).
//
// Usage: ./search_triples n k c_a c_b c_c
//          [--objective=min_J|max_J] [--tight] [--time-limit=SECONDS]

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

} // namespace

int main(int argc, char **argv) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0]
                  << " n k c_a c_b c_c [--objective=min_J|max_J]"
                  << " [--tight] [--time-limit=SECONDS]\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int ca = std::atoi(argv[3]);
    const int cb = std::atoi(argv[4]);
    const int cc = std::atoi(argv[5]);
    std::string objective = "max_J";
    bool tight = false;
    double time_limit_seconds = 60.0;
    for (int argument = 6; argument < argc; ++argument) {
        const std::string option(argv[argument]);
        if (option.rfind("--objective=", 0) == 0) {
            objective = option.substr(std::string("--objective=").size());
        } else if (option == "--tight") {
            tight = true;
        } else if (option.rfind("--time-limit=", 0) == 0) {
            try {
                time_limit_seconds = std::stod(option.substr(13));
            } catch (const std::exception &) {
                std::cerr << "Invalid time limit: " << option << '\n';
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
    }
    if ((objective != "min_J" && objective != "max_J") || k < 1 || k > n ||
        ca < 1 || cb < 1 || cc < 1) {
        std::cerr << "Require valid positive sizes and objective min_J or max_J.\n";
        return 1;
    }

    const auto vertices = vertices_of(n, k);
    const int count = static_cast<int>(vertices.size());
    if (ca + cb + cc > count) {
        std::cerr << "Fiber sizes exceed the vertex count.\n";
        return 1;
    }
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

    CpModelBuilder cp;
    std::vector<BoolVar> in_a(count), in_b(count), in_c(count);
    LinearExpr sum_a, sum_b, sum_c;
    for (int id = 0; id < count; ++id) {
        in_a[id] = cp.NewBoolVar();
        in_b[id] = cp.NewBoolVar();
        in_c[id] = cp.NewBoolVar();
        cp.AddAtMostOne({in_a[id], in_b[id], in_c[id]});
        sum_a += in_a[id];
        sum_b += in_b[id];
        sum_c += in_c[id];
    }
    cp.AddEquality(sum_a, ca);
    cp.AddEquality(sum_b, cb);
    cp.AddEquality(sum_c, cc);
    // Vertex-transitivity makes this safe because c_a is positive.
    cp.AddEquality(in_a[0], 1);

    const auto unite = [&](const std::vector<BoolVar> &left,
                           const std::vector<BoolVar> &right) {
        std::vector<BoolVar> result(count);
        for (int id = 0; id < count; ++id)
            result[id] = iff_any(cp, {left[id], right[id]});
        return result;
    };
    const auto boundary = [&](const std::vector<BoolVar> &set) {
        std::vector<BoolVar> result(count);
        for (int id = 0; id < count; ++id) {
            std::vector<BoolVar> neighbors;
            for (const int neighbor : adjacency[id])
                neighbors.push_back(set[neighbor]);
            result[id] = iff_all(cp, {Not(set[id]), iff_any(cp, neighbors)});
        }
        return result;
    };

    const auto in_ab = unite(in_a, in_b);
    const auto in_ac = unite(in_a, in_c);
    const auto in_bc = unite(in_b, in_c);
    const auto in_abc = unite(in_ab, in_c);
    const auto ext_a = boundary(in_a);
    const auto ext_b = boundary(in_b);
    const auto ext_c = boundary(in_c);
    const auto ext_ab = boundary(in_ab);
    const auto ext_ac = boundary(in_ac);
    const auto ext_bc = boundary(in_bc);
    const auto ext_abc = boundary(in_abc);

    LinearExpr boundary_a, boundary_b, boundary_c, sum_j;
    for (int id = 0; id < count; ++id) {
        boundary_a += ext_a[id];
        boundary_b += ext_b[id];
        boundary_c += ext_c[id];
        sum_j += ext_a[id] + ext_b[id] + ext_c[id] - ext_ab[id] -
                 ext_ac[id] - ext_bc[id] + ext_abc[id];
    }
    if (tight) {
        cp.AddEquality(boundary_a, rhs(n, k, ca));
        cp.AddEquality(boundary_b, rhs(n, k, cb));
        cp.AddEquality(boundary_c, rhs(n, k, cc));
    }
    if (objective == "max_J")
        cp.Maximize(sum_j);
    else
        cp.Minimize(sum_j);

    SatParameters parameters;
    parameters.set_num_search_workers(8);
    parameters.set_max_time_in_seconds(time_limit_seconds);
    Model model;
    model.Add(NewSatParameters(parameters));
    std::cout << "Building tripartite model for A(" << n << ',' << k << ") "
              << ca << '+' << cb << '+' << cc << " (N=" << count << ")...\n";
    std::cout << "Searching for " << objective << " (limit " << time_limit_seconds
              << " s; symmetry on; tight=" << (tight ? "yes" : "no") << ")...\n";
    const CpSolverResponse response = SolveCpModel(cp.Build(), &model);
    std::cout << "status: " << CpSolverStatus_Name(response.status()) << '\n';
    if (response.status() != CpSolverStatus::OPTIMAL &&
        response.status() != CpSolverStatus::FEASIBLE)
        return 0;

    const auto value = [&](BoolVar variable) {
        return SolutionIntegerValue(response, variable);
    };
    std::cout << "J = " << SolutionIntegerValue(response, sum_j) << '\n';
    const auto print_set = [&](const char *name, const std::vector<BoolVar> &set) {
        std::cout << name << " = {";
        for (int id = 0; id < count; ++id) {
            if (!value(set[id]))
                continue;
            std::cout << ' ';
            for (const int symbol : vertices[id])
                std::cout << symbol;
        }
        std::cout << " }\n";
    };
    print_set("A", in_a);
    print_set("B", in_b);
    print_set("C", in_c);
    std::cout << "Per-vertex signed boundary signature:\n";
    for (int id = 0; id < count; ++id) {
        const std::int64_t contribution = value(ext_a[id]) + value(ext_b[id]) +
            value(ext_c[id]) - value(ext_ab[id]) - value(ext_ac[id]) -
            value(ext_bc[id]) + value(ext_abc[id]);
        if (contribution == 0 && !value(in_a[id]) && !value(in_b[id]) && !value(in_c[id]))
            continue;
        for (const int symbol : vertices[id])
            std::cout << symbol;
        std::cout << " in=" << (value(in_a[id]) ? 'A' : value(in_b[id]) ? 'B' : value(in_c[id]) ? 'C' : '-')
                  << " ext=(" << value(ext_a[id]) << value(ext_b[id]) << value(ext_c[id])
                  << ';' << value(ext_ab[id]) << value(ext_ac[id]) << value(ext_bc[id])
                  << ';' << value(ext_abc[id]) << ") J=" << contribution << '\n';
    }
    if (response.status() != CpSolverStatus::OPTIMAL)
        std::cout << "WARNING: FEASIBLE is an incumbent, not a certified extremum.\n";
}
