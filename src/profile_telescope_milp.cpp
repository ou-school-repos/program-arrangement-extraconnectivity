// Bounded CP-SAT feasibility probe for a coordinate-profile telescope.
//
// Usage:
//   ./profile_telescope_milp n k
//       [--max-r=R] [--max-sets=N] [--g-cap=N]
//       [--time-limit=SECONDS] [--workers=N]
//
// For every enumerated set V, the model chooses one nontrivial coordinate
// split p and requires
//
//   G(state(V)) <= gap(V,p) + sum_s G(state(F_s)),
//
// with integer G >= 0.  A state is the sorted multiset of the sorted fiber
// size profiles over all coordinates.  This is a finite feasibility probe,
// not a proof outside the enumerated cardinality range.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/sat_parameters.pb.h"

namespace {

using operations_research::Domain;
using operations_research::sat::BoolVar;
using operations_research::sat::CpModelBuilder;
using operations_research::sat::CpSolverResponse;
using operations_research::sat::CpSolverStatus;
using operations_research::sat::CpSolverStatus_Name;
using operations_research::sat::IntVar;
using operations_research::sat::LinearExpr;
using operations_research::sat::Model;
using operations_research::sat::NewSatParameters;
using operations_research::sat::SatParameters;
using operations_research::sat::SolutionIntegerValue;
using operations_research::sat::SolveCpModel;

using Vertex = std::vector<int>;
using Subset = std::vector<int>;
using Profile = std::vector<int>;
using State = std::vector<Profile>;

struct Transition {
    int parent_state;
    std::int64_t gap;
    std::vector<int> child_states;
};

std::int64_t e_seq(int size) {
    std::int64_t total = 0;
    for (int value = 0; value < size; ++value)
        total += __builtin_popcount(static_cast<unsigned>(value));
    return total;
}

std::int64_t sbl(int size) {
    std::int64_t total = 0;
    for (int value = 1; value < size; ++value)
        total += 32 - __builtin_clz(static_cast<unsigned>(value));
    return total;
}

std::int64_t c_constant(int size) {
    if (size == 0)
        return 0;
    return size - 1 + sbl(size) - e_seq(size);
}

std::int64_t potential(int size, int overhang) {
    return c_constant(size) + static_cast<std::int64_t>(overhang) * e_seq(size);
}

std::vector<Vertex> vertices_of(int n, int k) {
    std::vector<Vertex> vertices;
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

std::int64_t choose_bounded(int n, int r, std::int64_t cap) {
    if (r < 0 || r > n)
        return 0;
    r = std::min(r, n - r);
    std::int64_t result = 1;
    for (int i = 1; i <= r; ++i) {
        if (result > cap / (n - r + i))
            return cap + 1;
        result *= n - r + i;
        result /= i;
    }
    return result;
}

std::int64_t subset_count_bounded(int vertex_count, int max_r, std::int64_t cap) {
    std::int64_t total = 0;
    for (int r = 1; r <= max_r; ++r) {
        const std::int64_t count = choose_bounded(vertex_count, r, cap - total);
        if (count > cap - total)
            return cap + 1;
        total += count;
    }
    return total;
}

void enumerate_subsets(int vertex_count, int max_r, std::vector<Subset> *out) {
    Subset current;
    std::function<void(int, int)> choose = [&](int next, int remaining) {
        if (remaining == 0) {
            out->push_back(current);
            return;
        }
        for (int index = next; index <= vertex_count - remaining; ++index) {
            current.push_back(index);
            choose(index + 1, remaining - 1);
            current.pop_back();
        }
    };
    for (int size = 1; size <= max_r; ++size)
        choose(0, size);
}

State state_of(const Subset &subset, const std::vector<Vertex> &vertices, int k) {
    State profiles;
    for (int position = 0; position < k; ++position) {
        std::map<int, int> fiber_sizes;
        for (const int id : subset)
            ++fiber_sizes[vertices[id][position]];
        Profile profile;
        for (const auto &[symbol, size] : fiber_sizes) {
            static_cast<void>(symbol);
            profile.push_back(size);
        }
        std::sort(profile.begin(), profile.end());
        profiles.push_back(std::move(profile));
    }
    std::sort(profiles.begin(), profiles.end());
    return profiles;
}

std::int64_t defect_of(const Subset &subset, const std::vector<Vertex> &vertices,
                       int k) {
    int roots = 0;
    for (int position = 0; position < k; ++position) {
        std::set<Vertex> seen;
        for (const int id : subset) {
            Vertex root = vertices[id];
            root.erase(root.begin() + position);
            seen.insert(std::move(root));
        }
        roots += static_cast<int>(seen.size());
    }
    return static_cast<std::int64_t>(subset.size()) * k - roots;
}

std::int64_t cross_collisions_of(const Subset &subset,
                                 const std::vector<Vertex> &vertices, int n, int k) {
    std::set<Vertex> members, external;
    for (const int id : subset)
        members.insert(vertices[id]);

    std::int64_t directional_total = 0;
    for (int position = 0; position < k; ++position) {
        std::set<Vertex> directional;
        for (const int id : subset) {
            const Vertex &vertex = vertices[id];
            std::set<int> used(vertex.begin(), vertex.end());
            for (int symbol = 0; symbol < n; ++symbol) {
                if (used.count(symbol))
                    continue;
                Vertex neighbor = vertex;
                neighbor[position] = symbol;
                if (!members.count(neighbor))
                    directional.insert(neighbor);
            }
        }
        directional_total += static_cast<std::int64_t>(directional.size());
        external.insert(directional.begin(), directional.end());
    }
    return directional_total - static_cast<std::int64_t>(external.size());
}

std::int64_t phi_of(const Subset &subset, const std::vector<Vertex> &vertices, int n,
                    int k) {
    const int m = n - k;
    return cross_collisions_of(subset, vertices, n, k) +
           static_cast<std::int64_t>(m + 1) * defect_of(subset, vertices, k);
}

std::vector<Subset> fibers_of(const Subset &subset, const std::vector<Vertex> &vertices,
                               int position) {
    std::map<int, Subset> fibers;
    for (const int id : subset)
        fibers[vertices[id][position]].push_back(id);
    std::vector<Subset> result;
    for (auto &[symbol, fiber] : fibers) {
        static_cast<void>(symbol);
        result.push_back(std::move(fiber));
    }
    return result;
}

void print_state(const State &state) {
    std::cout << "{";
    for (std::size_t i = 0; i < state.size(); ++i) {
        if (i)
            std::cout << ",";
        std::cout << "(";
        for (std::size_t j = 0; j < state[i].size(); ++j) {
            if (j)
                std::cout << ",";
            std::cout << state[i][j];
        }
        std::cout << ")";
    }
    std::cout << "}";
}

}  // namespace

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " n k [--max-r=R] [--max-sets=N] "
                  << "[--g-cap=N] [--time-limit=SECONDS] [--workers=N]\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    int max_r = 4;
    std::int64_t max_sets = 100000;
    std::int64_t g_cap = 100;
    double time_limit = 30.0;
    int workers = 1;
    for (int argument = 3; argument < argc; ++argument) {
        const std::string option(argv[argument]);
        if (option.rfind("--max-r=", 0) == 0)
            max_r = std::stoi(option.substr(8));
        else if (option.rfind("--max-sets=", 0) == 0)
            max_sets = std::stoll(option.substr(11));
        else if (option.rfind("--g-cap=", 0) == 0)
            g_cap = std::stoll(option.substr(8));
        else if (option.rfind("--time-limit=", 0) == 0)
            time_limit = std::stod(option.substr(13));
        else if (option.rfind("--workers=", 0) == 0)
            workers = std::stoi(option.substr(10));
        else {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
    }
    if (n < 1 || k < 1 || k > n || max_r < 1 || max_sets < 1 || g_cap < 0 ||
        time_limit <= 0 || workers < 1) {
        std::cerr << "Require 1 <= k <= n, positive caps, and positive solver limits.\n";
        return 1;
    }

    const std::vector<Vertex> vertices = vertices_of(n, k);
    max_r = std::min(max_r, static_cast<int>(vertices.size()));
    const std::int64_t estimate = subset_count_bounded(
        static_cast<int>(vertices.size()), max_r, max_sets);
    if (estimate > max_sets) {
        std::cerr << "Refusing to enumerate more than --max-sets=" << max_sets
                  << " subsets (raise the cap explicitly).\n";
        return 1;
    }

    std::cout << "Enumerating " << estimate << " subsets of A(" << n << "," << k
              << ") through R=" << max_r << "; CP-SAT workers=" << workers
              << ", time limit=" << time_limit << " s.\n";
    std::vector<Subset> subsets;
    subsets.reserve(static_cast<std::size_t>(estimate));
    enumerate_subsets(static_cast<int>(vertices.size()), max_r, &subsets);

    std::map<State, int> state_index;
    std::vector<State> states;
    const auto register_state = [&](const State &state) {
        const auto [iterator, inserted] =
            state_index.emplace(state, static_cast<int>(states.size()));
        if (inserted)
            states.push_back(state);
        return iterator->second;
    };

    std::vector<std::vector<Transition>> choices_by_subset;
    choices_by_subset.reserve(subsets.size());
    const int m = n - k;
    for (const Subset &subset : subsets) {
        std::vector<Transition> choices;
        const int parent_state = register_state(state_of(subset, vertices, k));
        if (subset.size() >= 2) {
            const std::int64_t phi_parent = phi_of(subset, vertices, n, k);
            const std::int64_t p_parent = potential(static_cast<int>(subset.size()), m);
            for (int position = 0; position < k; ++position) {
                const std::vector<Subset> fibers = fibers_of(subset, vertices, position);
                if (fibers.size() < 2)
                    continue;
                std::int64_t phi_children = 0;
                std::int64_t p_children = 0;
                std::vector<int> child_states;
                for (const Subset &fiber : fibers) {
                    phi_children += phi_of(fiber, vertices, n, k);
                    p_children += potential(static_cast<int>(fiber.size()), m);
                    child_states.push_back(register_state(state_of(fiber, vertices, k)));
                }
                choices.push_back({parent_state, p_parent - p_children -
                                                    (phi_parent - phi_children),
                                   std::move(child_states)});
            }
        }
        choices_by_subset.push_back(std::move(choices));
    }

    CpModelBuilder model;
    std::vector<IntVar> g;
    g.reserve(states.size());
    for (std::size_t index = 0; index < states.size(); ++index)
        g.push_back(model.NewIntVar(Domain(0, g_cap)));

    int constrained_sets = 0;
    int choice_count = 0;
    for (std::size_t subset_index = 0; subset_index < subsets.size(); ++subset_index) {
        const std::vector<Transition> &choices = choices_by_subset[subset_index];
        if (choices.empty())
            continue;
        ++constrained_sets;
        std::vector<BoolVar> choose;
        choose.reserve(choices.size());
        for (std::size_t choice = 0; choice < choices.size(); ++choice)
            choose.push_back(model.NewBoolVar());
        model.AddExactlyOne(choose);
        for (std::size_t choice = 0; choice < choices.size(); ++choice) {
            const Transition &transition = choices[choice];
            LinearExpr children_sum;
            for (const int child : transition.child_states)
                children_sum += g[child];
            model.AddLessOrEqual(g[transition.parent_state],
                                  transition.gap + children_sum)
                .OnlyEnforceIf(choose[choice]);
            ++choice_count;
        }
    }

    const Subset singleton{0};
    const int singleton_state = register_state(state_of(singleton, vertices, k));
    model.AddEquality(g[singleton_state], 0);

    SatParameters parameters;
    parameters.set_num_search_workers(workers);
    parameters.set_max_time_in_seconds(time_limit);
    Model solver_model;
    solver_model.Add(NewSatParameters(parameters));
    const CpSolverResponse response = SolveCpModel(model.Build(), &solver_model);

    std::cout << "Model: " << states.size() << " states, " << constrained_sets
              << " parent sets, " << choice_count << " split choices.\n";
    std::cout << "status: " << CpSolverStatus_Name(response.status()) << '\n';
    if (response.status() != CpSolverStatus::OPTIMAL &&
        response.status() != CpSolverStatus::FEASIBLE)
        return 0;

    std::int64_t maximum_g = 0;
    for (const IntVar variable : g)
        maximum_g = std::max(maximum_g, SolutionIntegerValue(response, variable));
    std::cout << "G range: [0, " << maximum_g << "] (configured cap " << g_cap << ")\n";
    std::cout << "Largest profile states:\n";
    std::vector<int> order(states.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int left, int right) {
        return SolutionIntegerValue(response, g[left]) >
               SolutionIntegerValue(response, g[right]);
    });
    for (int rank = 0; rank < std::min(5, static_cast<int>(order.size())); ++rank) {
        const int index = order[rank];
        std::cout << "  G=" << SolutionIntegerValue(response, g[index]) << " state=";
        print_state(states[index]);
        std::cout << '\n';
    }
    return 0;
}
