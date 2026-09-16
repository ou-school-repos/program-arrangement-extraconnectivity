// Exact CP-SAT adversary for the one-fiber coupled defect/collision ledger.
//
// Usage: ./search_single n k R
//          [--objective=slack|collision-excess|collisions]
//          [--defect-deficit=Q] [--time-limit=SECONDS] [--no-symmetry-break]
//
// collision-excess maximizes the stronger residual X - (m + 1)(E(R) - D).
// A positive value refutes that stronger experimental bound.  The exact
// proposition-equivalent collision bound additionally permits C(R)-E(R),
// which is printed as the target margin below.

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
using operations_research::sat::IntVar;
using operations_research::Domain;
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
                  << "[--objective=slack|collision-excess|collisions|local-residual] "
                  << "[--time-limit=SECONDS] [--no-symmetry-break]\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int size = std::atoi(argv[3]);
    std::string objective = "collision-excess";
    double time_limit = 0.0;
    bool has_time_limit = false;
    bool symmetry_break = true;
    int exact_defect_deficit = -1;
    for (int argument = 4; argument < argc; ++argument) {
        const std::string option(argv[argument]);
        if (option.rfind("--objective=", 0) == 0)
            objective = option.substr(12);
        else if (option.rfind("--defect-deficit=", 0) == 0)
            exact_defect_deficit = std::stoi(option.substr(17));
        else if (option.rfind("--time-limit=", 0) == 0)
            time_limit = std::stod(option.substr(13)), has_time_limit = true;
        else if (option == "--no-symmetry-break")
            symmetry_break = false;
        else {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
    }
    if (k < 1 || k > n || size < 1 ||
        (objective != "slack" && objective != "collision-excess" &&
         objective != "collisions" && objective != "local-residual")) {
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
    std::vector<BoolVar> coord0_projections;
    std::vector<std::vector<int>> coord0_compatible_symbols;
    for (int position = 0; position < k; ++position) {
        std::map<std::vector<int>, std::vector<int>> root_members;
        for (int id = 0; id < count; ++id) {
            auto root = vertices[id];
            root.erase(root.begin() + position);
            root_members[root].push_back(id);
        }
        for (const auto &[root, members] : root_members) {
            std::vector<BoolVar> literals;
            for (int id : members) literals.push_back(in_set[id]);
            const BoolVar occupied = iff_any(cp, literals);
            occupied_roots += occupied;
            if (position == 0) {
                coord0_projections.push_back(occupied);
                std::vector<int> compatible;
                for (int symbol = 0; symbol < n; ++symbol) {
                    if (std::find(root.begin(), root.end(), symbol) == root.end())
                        compatible.push_back(symbol);
                }
                coord0_compatible_symbols.push_back(std::move(compatible));
            }
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

    // Coordinate-0 local residual: required immune slots minus empty-slice
    // immune slots, namely mR - dC - m*dE - V_0.
    std::vector<std::int64_t> e_values(size + 1), c_values(size + 1);
    for (int slice_size = 0; slice_size <= size; ++slice_size) {
        e_values[slice_size] = e_seq(slice_size);
        c_values[slice_size] = c_constant(slice_size);
    }
    const std::int64_t max_e = *std::max_element(e_values.begin(), e_values.end());
    const std::int64_t max_c = *std::max_element(c_values.begin(), c_values.end());
    std::vector<IntVar> slice_sizes(n), e_by_slice(n), c_by_slice(n);
    std::vector<BoolVar> is_empty(n);
    LinearExpr sum_e0, sum_c0, void0;
    for (int symbol = 0; symbol < n; ++symbol) {
        slice_sizes[symbol] = cp.NewIntVar(Domain(0, size));
        is_empty[symbol] = cp.NewBoolVar();
        e_by_slice[symbol] = cp.NewIntVar(Domain(0, max_e));
        c_by_slice[symbol] = cp.NewIntVar(Domain(0, max_c));
        LinearExpr members_in_slice;
        for (int id = 0; id < count; ++id) {
            if (vertices[id][0] == symbol) members_in_slice += in_set[id];
        }
        cp.AddEquality(slice_sizes[symbol], members_in_slice);
        cp.AddEquality(slice_sizes[symbol], 0).OnlyEnforceIf(is_empty[symbol]);
        cp.AddGreaterThan(slice_sizes[symbol], 0).OnlyEnforceIf(Not(is_empty[symbol]));
        cp.AddElement(slice_sizes[symbol], e_values, e_by_slice[symbol]);
        cp.AddElement(slice_sizes[symbol], c_values, c_by_slice[symbol]);
        sum_e0 += e_by_slice[symbol];
        sum_c0 += c_by_slice[symbol];
    }
    for (std::size_t p = 0; p < coord0_projections.size(); ++p) {
        for (const int symbol : coord0_compatible_symbols[p]) {
            const BoolVar void_incidence = cp.NewBoolVar();
            cp.AddBoolAnd({coord0_projections[p], is_empty[symbol]})
                .OnlyEnforceIf(void_incidence);
            cp.AddBoolOr({Not(coord0_projections[p]), Not(is_empty[symbol])})
                .OnlyEnforceIf(Not(void_incidence));
            void0 += void_incidence;
        }
    }
    const LinearExpr local_residual =
        sum_c0 + m * sum_e0 - void0 + (m * size - c - m * e);

    if (exact_defect_deficit >= 0)
        cp.AddEquality(defect_deficit, exact_defect_deficit);

    if (objective == "slack") cp.Minimize(slack);
    else if (objective == "collisions") cp.Maximize(collisions);
    else if (objective == "local-residual") cp.Maximize(local_residual);
    else cp.Maximize(collision_excess);

    SatParameters parameters;
    parameters.set_num_search_workers(8);
    if (has_time_limit)
        parameters.set_max_time_in_seconds(time_limit);
    Model model;
    model.Add(NewSatParameters(parameters));
    std::cout << "Building single-fiber model for A(" << n << ',' << k
              << ") R=" << size << " (N=" << count << "; m=" << m
              << "; rhs=" << rhs(n, k, size) << ")...\n";
    std::cout << "Searching for objective " << objective << " (limit "
              << (has_time_limit ? std::to_string(time_limit) + " s" : "none")
              << "; symmetry " << (symmetry_break ? "on" : "off")
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
    std::cout << "C(R)-E(R) = " << (c - e)
              << ", target excess = " << (value(collision_excess) - (c - e))
              << '\n';
    if (objective == "local-residual")
        std::cout << "coordinate-0 local residual = " << value(local_residual) << '\n';

    std::vector<int> members;
    for (int id = 0; id < count; ++id) {
        if (SolutionIntegerValue(response, in_set[id])) members.push_back(id);
    }
    std::cout << "Empty-slice immune-slot audit:\n";
    for (int position = 0; position < k; ++position) {
        std::vector<int> slice_sizes(n, 0);
        std::map<std::vector<int>, int> projections;
        for (const int id : members) {
            ++slice_sizes[vertices[id][position]];
            auto projection = vertices[id];
            projection.erase(projection.begin() + position);
            projections.emplace(std::move(projection), 0);
        }
        int support = 0;
        std::int64_t sum_e = 0, sum_c = 0, void_slots = 0;
        for (int symbol = 0; symbol < n; ++symbol) {
            if (slice_sizes[symbol] != 0) {
                ++support;
            } else {
                for (const auto &[projection, unused] : projections) {
                    (void)unused;
                    if (std::find(projection.begin(), projection.end(), symbol) ==
                        projection.end()) {
                        ++void_slots;
                    }
                }
            }
            sum_e += e_seq(slice_sizes[symbol]);
            sum_c += c_constant(slice_sizes[symbol]);
        }
        const std::int64_t delta_e = e - sum_e;
        const std::int64_t delta_c = c - sum_c;
        const std::int64_t required_gap =
            m * size - delta_c - m * delta_e;
        const std::int64_t void_lower =
            static_cast<std::int64_t>(projections.size()) *
            std::max<std::int64_t>(0, m + 1 - support);
        const std::int64_t nonempty_residual =
            std::max<std::int64_t>(0, required_gap - void_slots);
        const char *void_status = required_gap <= 0
                                      ? "vacuous"
                                      : (void_slots >= required_gap ? "covers" : "no");
        std::cout << "  coordinate " << position
                  << ": support=" << support
                  << ", |pi|=" << projections.size()
                  << ", void=" << void_slots
                  << ", lower=" << void_lower
                  << ", dC=" << delta_c
                  << ", dE=" << delta_e
                  << ", required=" << required_gap
                  << ", void=" << void_status
                  << ", nonempty-residual=" << nonempty_residual << '\n';
    }
    std::cout << "F = {";
    for (const int id : members) {
        std::cout << ' ';
        for (int symbol : vertices[id]) std::cout << symbol;
    }
    std::cout << " }\n";
    if (response.status() != CpSolverStatus::OPTIMAL)
        std::cout << "WARNING: FEASIBLE is an incumbent, not a certified extremum.\n";
}
