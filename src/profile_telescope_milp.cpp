// Bounded CP-SAT feasibility probe for a coordinate-profile telescope.
//
// Usage:
//   ./profile_telescope_milp n k
//       [--mode=per-set|relaxed-state|exact-menu]
//       [--star-only]
//       [--max-r=R] [--max-sets=N] [--g-cap=N]
//       [--time-limit=SECONDS] [--workers=N]
//       [--dump-positive-menus] [--dump-ledger]
//
// Modes:
//   per-set      One constraint per concrete set (default, original).
//   relaxed-state Union all transitions per profile state, one constraint
//                 per state.  Fast bounded falsification filter: if
//                 infeasible, no profile-only G within --g-cap exists.
//   exact-menu   Group sets by (state, menu).  Two sets with the same
//                 parent state and identical split menu impose identical
//                 disjunctive constraints, so retaining one is exactly
//                 equivalent.  Compression is auditable.
//
// --star-only  Use one radius-one Star of size --max-r, together with its
//              closure under all nontrivial coordinate-fiber splits.  This
//              is a finite Star-closure probe, not an all-subsets search.

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
    int parent_state = 0;
    std::int64_t gap = 0;
    std::vector<int> child_states;
};

// A menu is a sorted list of (gap, sorted child_state multiset) options.
// Two sets with the same (state, menu) impose identical constraints.
struct TransitionKey {
    std::int64_t gap = 0;
    std::vector<int> child_states;

    bool operator<(const TransitionKey &other) const {
        if (gap != other.gap)
            return gap < other.gap;
        return child_states < other.child_states;
    }
};

using Menu = std::vector<TransitionKey>;

// ---- Arithmetic ----------------------------------------------------------

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

// ---- Graph ---------------------------------------------------------------

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
        std::int64_t numerator = n - r + i;
        std::int64_t denominator = i;
        const std::int64_t numerator_gcd = std::gcd(numerator, denominator);
        numerator /= numerator_gcd;
        denominator /= numerator_gcd;
        const std::int64_t result_gcd = std::gcd(result, denominator);
        result /= result_gcd;
        denominator /= result_gcd;
        if (denominator != 1 || result > cap / numerator)
            return cap + 1;
        result *= numerator;
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

// ---- Geometry ------------------------------------------------------------

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

// ---- Star Graph -----------------------------------------------------------

Subset star_graph_center(int k) {
    Subset center(k);
    for (int i = 0; i < k; ++i)
        center[i] = i;
    return center;
}

Subset star_graph(const std::vector<Vertex> &vertices, int k, int size) {
    const Vertex center = star_graph_center(k);
    // Find center index.
    int center_id = -1;
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        if (vertices[i] == center) {
            center_id = i;
            break;
        }
    }
    if (center_id < 0)
        return {};

    // Collect neighbors of center (differ at exactly one position).
    std::vector<int> neighbors;
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        if (i == center_id)
            continue;
        int diff = 0;
        for (int p = 0; p < k; ++p)
            diff += (vertices[i][p] != center[p]) ? 1 : 0;
        if (diff == 1)
            neighbors.push_back(i);
    }
    if (size < 1 || size > 1 + static_cast<int>(neighbors.size()))
        return {};
    Subset result;
    result.push_back(center_id);
    for (int i = 0; i < size - 1; ++i)
        result.push_back(neighbors[i]);
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Subset> star_closure(const std::vector<Vertex> &vertices, int k,
                                 int max_r) {
    Subset center = star_graph(vertices, k, max_r);
    if (center.empty())
        return {};

    std::set<Subset> seen;
    std::vector<Subset> pool;
    std::vector<Subset> queue;
    auto enqueue = [&](const Subset &s) {
        if (seen.count(s))
            return;
        seen.insert(s);
        pool.push_back(s);
        if (static_cast<int>(s.size()) >= 2)
            queue.push_back(s);
    };
    enqueue(center);

    while (!queue.empty()) {
        Subset current = queue.back();
        queue.pop_back();
        for (int position = 0; position < k; ++position) {
            const std::vector<Subset> fibers =
                fibers_of(current, vertices, position);
            if (fibers.size() < 2)
                continue;
            for (const Subset &fiber : fibers) {
                if (static_cast<int>(fiber.size()) <= max_r)
                    enqueue(fiber);
            }
        }
    }
    return pool;
}

// ---- Menu ----------------------------------------------------------------

Menu menu_of(const std::vector<Transition> &choices) {
    Menu result;
    result.reserve(choices.size());
    for (const Transition &t : choices) {
        TransitionKey key;
        key.gap = t.gap;
        key.child_states = t.child_states;
        std::sort(key.child_states.begin(), key.child_states.end());
        result.push_back(std::move(key));
    }
    std::sort(result.begin(), result.end());
    return result;
}

// ---- Printing ------------------------------------------------------------

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

void print_subset(const Subset &subset, const std::vector<Vertex> &vertices) {
    std::cout << "{";
    for (std::size_t i = 0; i < subset.size(); ++i) {
        if (i)
            std::cout << ",";
        const Vertex &vertex = vertices[subset[i]];
        std::cout << "(";
        for (std::size_t j = 0; j < vertex.size(); ++j) {
            if (j)
                std::cout << ",";
            std::cout << vertex[j];
        }
        std::cout << ")";
    }
    std::cout << "}";
}

void dump_split_menu(const Subset &subset, const std::vector<Vertex> &vertices,
                     int n, int k) {
    const int m = n - k;
    const std::int64_t x_parent = cross_collisions_of(subset, vertices, n, k);
    const std::int64_t d_parent = defect_of(subset, vertices, k);
    const std::int64_t p_parent = potential(static_cast<int>(subset.size()), m);
    for (int position = 0; position < k; ++position) {
        const std::vector<Subset> fibers = fibers_of(subset, vertices, position);
        if (fibers.size() < 2)
            continue;
        std::vector<int> sizes;
        std::int64_t x_children = 0;
        std::int64_t d_children = 0;
        std::int64_t p_children = 0;
        for (const Subset &fiber : fibers) {
            sizes.push_back(static_cast<int>(fiber.size()));
            x_children += cross_collisions_of(fiber, vertices, n, k);
            d_children += defect_of(fiber, vertices, k);
            p_children += potential(static_cast<int>(fiber.size()), m);
        }
        std::sort(sizes.begin(), sizes.end());
        const std::int64_t delta_x = x_parent - x_children;
        const std::int64_t delta_d = d_parent - d_children;
        const std::int64_t overhead = delta_x +
            static_cast<std::int64_t>(m + 1) * delta_d;
        const std::int64_t surplus = p_parent - p_children;
        std::cout << "    p=" << position << " sizes=(";
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            if (i)
                std::cout << ",";
            std::cout << sizes[i];
        }
        std::cout << ") dX=" << delta_x << " dD=" << delta_d
                  << " overhead=" << overhead << " surplus=" << surplus
                  << " gap=" << surplus - overhead << '\n';
    }
}

void dump_split_ledger(const Subset &subset, const std::vector<Vertex> &vertices,
                       int n, int k, const std::map<State, int> &state_index,
                       const std::vector<IntVar> &g,
                       const CpSolverResponse &response) {
    const int m = n - k;
    const State parent_state = state_of(subset, vertices, k);
    const int parent_index = state_index.at(parent_state);
    const std::int64_t parent_g = SolutionIntegerValue(response, g[parent_index]);
    const std::int64_t phi_parent = phi_of(subset, vertices, n, k);
    const std::int64_t p_parent = potential(static_cast<int>(subset.size()), m);
    std::cout << "  parent G=" << parent_g << " state=";
    print_state(parent_state);
    std::cout << " V=";
    print_subset(subset, vertices);
    std::cout << '\n';
    for (int position = 0; position < k; ++position) {
        const std::vector<Subset> fibers = fibers_of(subset, vertices, position);
        if (fibers.size() < 2)
            continue;
        std::int64_t phi_children = 0;
        std::int64_t p_children = 0;
        std::int64_t child_g_sum = 0;
        std::vector<int> sizes;
        for (const Subset &fiber : fibers) {
            phi_children += phi_of(fiber, vertices, n, k);
            p_children += potential(static_cast<int>(fiber.size()), m);
            const int child_index = state_index.at(state_of(fiber, vertices, k));
            child_g_sum += SolutionIntegerValue(response, g[child_index]);
            sizes.push_back(static_cast<int>(fiber.size()));
        }
        std::sort(sizes.begin(), sizes.end());
        const std::int64_t gap = p_parent - p_children -
            (phi_parent - phi_children);
        const std::int64_t rhs = gap + child_g_sum;
        std::cout << "    p=" << position << " sizes=(";
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            if (i)
                std::cout << ",";
            std::cout << sizes[i];
        }
        std::cout << ") gap=" << gap << " child_G=" << child_g_sum
                  << " rhs=" << rhs << " slack=" << rhs - parent_g << '\n';
    }
}

// ---- Model builders ------------------------------------------------------

// Per-set: one constraint per concrete set (original behavior).
void build_per_set(const std::vector<std::vector<Transition>> &choices_by_subset,
                   CpModelBuilder *model, std::vector<IntVar> *g) {
    int constrained_sets = 0;
    int choice_count = 0;
    for (std::size_t i = 0; i < choices_by_subset.size(); ++i) {
        const std::vector<Transition> &choices = choices_by_subset[i];
        if (choices.empty())
            continue;
        ++constrained_sets;
        std::vector<BoolVar> choose;
        choose.reserve(choices.size());
        for (std::size_t c = 0; c < choices.size(); ++c)
            choose.push_back(model->NewBoolVar());
        model->AddExactlyOne(choose);
        for (std::size_t c = 0; c < choices.size(); ++c) {
            const Transition &t = choices[c];
            LinearExpr children_sum = std::accumulate(
                t.child_states.begin(), t.child_states.end(), LinearExpr{},
                [&g](LinearExpr acc, int child) { return acc + (*g)[child]; });
            model->AddLessOrEqual((*g)[t.parent_state],
                                  t.gap + children_sum)
                .OnlyEnforceIf(choose[c]);
            ++choice_count;
        }
    }
    std::cout << "  constrained_sets=" << constrained_sets
              << " split_choices=" << choice_count << '\n';
}

// Relaxed-state: union all transitions per profile state, one constraint
// per state.  Fast falsification filter.
void build_relaxed_state(
    const std::vector<std::vector<Transition>> &choices_by_subset,
    CpModelBuilder *model, std::vector<IntVar> *g) {

    // Union transitions by parent state, dedup by (gap, sorted child_states).
    std::map<int, std::set<TransitionKey>> union_by_state;
    for (const auto &choices : choices_by_subset) {
        for (const Transition &t : choices) {
            TransitionKey key;
            key.gap = t.gap;
            key.child_states = t.child_states;
            std::sort(key.child_states.begin(), key.child_states.end());
            union_by_state[t.parent_state].insert(key);
        }
    }

    int constrained_states = 0;
    int choice_count = 0;
    for (auto &[state_idx, key_set] : union_by_state) {
        if (key_set.empty())
            continue;
        ++constrained_states;
        std::vector<BoolVar> choose;
        choose.reserve(key_set.size());
        for (std::size_t c = 0; c < key_set.size(); ++c)
            choose.push_back(model->NewBoolVar());
        model->AddExactlyOne(choose);
        std::size_t c = 0;
        for (const TransitionKey &key : key_set) {
            LinearExpr children_sum = std::accumulate(
                key.child_states.begin(), key.child_states.end(), LinearExpr{},
                [&g](LinearExpr acc, int child) { return acc + (*g)[child]; });
            model->AddLessOrEqual((*g)[state_idx],
                                  key.gap + children_sum)
                .OnlyEnforceIf(choose[c]);
            ++c;
            ++choice_count;
        }
    }
    std::cout << "  constrained_states=" << constrained_states
              << " split_choices=" << choice_count << '\n';
}

// Exact-menu: group by (state, menu), keep one constraint per unique pair.
// Two sets with the same parent state and identical split menu impose
// identical disjunctive constraints, so retaining one is exactly equivalent.
void build_exact_menu(
    const std::vector<std::vector<Transition>> &choices_by_subset,
    const std::vector<int> &parent_states,
    CpModelBuilder *model, std::vector<IntVar> *g) {

    struct MenuEntry {
        int parent_state = 0;
        Menu menu;
        bool operator<(const MenuEntry &other) const {
            if (parent_state != other.parent_state)
                return parent_state < other.parent_state;
            return menu < other.menu;
        }
    };

    // Collect unique (state, menu) pairs.
    std::set<MenuEntry> unique_menus;
    for (std::size_t i = 0; i < choices_by_subset.size(); ++i) {
        if (choices_by_subset[i].empty())
            continue;
        MenuEntry entry;
        entry.parent_state = parent_states[i];
        entry.menu = menu_of(choices_by_subset[i]);
        unique_menus.insert(entry);
    }

    // Map each unique menu to its representative transitions.
    std::map<MenuEntry, std::vector<Transition>> representative;
    for (std::size_t i = 0; i < choices_by_subset.size(); ++i) {
        if (choices_by_subset[i].empty())
            continue;
        MenuEntry entry;
        entry.parent_state = parent_states[i];
        entry.menu = menu_of(choices_by_subset[i]);
        if (representative.find(entry) == representative.end())
            representative.try_emplace(entry, choices_by_subset[i]);
    }

    int constrained_menus = 0;
    int choice_count = 0;
    for (const auto &[entry, choices] : representative) {
        ++constrained_menus;
        std::vector<BoolVar> choose;
        choose.reserve(choices.size());
        for (std::size_t c = 0; c < choices.size(); ++c)
            choose.push_back(model->NewBoolVar());
        model->AddExactlyOne(choose);
        for (std::size_t c = 0; c < choices.size(); ++c) {
            const Transition &t = choices[c];
            LinearExpr children_sum = std::accumulate(
                t.child_states.begin(), t.child_states.end(), LinearExpr{},
                [&g](LinearExpr acc, int child) { return acc + (*g)[child]; });
            model->AddLessOrEqual((*g)[t.parent_state],
                                  t.gap + children_sum)
                .OnlyEnforceIf(choose[c]);
            ++choice_count;
        }
    }
    std::cout << "  constrained_menus=" << constrained_menus
              << " split_choices=" << choice_count << '\n';
}

struct SolveResult {
    CpSolverResponse response;
    std::vector<IntVar> g;
};

SolveResult build_and_solve(
    const std::string &mode, bool minimize_sum_g,
    std::int64_t g_cap, int workers, double time_limit,
    const std::vector<std::vector<Transition>> &choices_by_subset,
    const std::vector<int> &parent_states,
    const std::map<State, int> &state_index,
    int singleton_state,
    const std::vector<std::int64_t> &pin_state_max) {
    CpModelBuilder model;
    const int num_states = static_cast<int>(state_index.size());
    std::vector<IntVar> g;
    g.reserve(num_states);
    for (int i = 0; i < num_states; ++i)
        g.push_back(model.NewIntVar(Domain(0, g_cap)));

    model.AddEquality(g[singleton_state], 0);

    for (int i = 0; i < num_states; ++i) {
        if (i < static_cast<int>(pin_state_max.size()) &&
            pin_state_max[i] >= 0) {
            model.AddLessOrEqual(g[i], pin_state_max[i]);
        }
    }

    if (minimize_sum_g) {
        LinearExpr total_g;
        for (const IntVar &variable : g)
            total_g += variable;
        model.Minimize(total_g);
    }

    if (mode == "per-set")
        build_per_set(choices_by_subset, &model, &g);
    else if (mode == "relaxed-state")
        build_relaxed_state(choices_by_subset, &model, &g);
    else
        build_exact_menu(choices_by_subset, parent_states, &model, &g);

    SatParameters parameters;
    parameters.set_num_search_workers(workers);
    parameters.set_max_time_in_seconds(time_limit);
    Model solver_model;
    solver_model.Add(NewSatParameters(parameters));
    return {SolveCpModel(model.Build(), &solver_model), std::move(g)};
}

}  // namespace

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " n k"
                  << " [--mode=per-set|relaxed-state|exact-menu]"
                  << " [--max-r=R] [--max-sets=N] [--g-cap=N]"
                  << " [--time-limit=SECONDS] [--workers=N]"
                  << " [--print-positive-g] [--minimize-sum-g]"
                  << " [--dump-positive-menus] [--dump-ledger]"
                  << " [--star-only] [--test-forced]\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    int max_r = 4;
    std::int64_t max_sets = 100000;
    std::int64_t g_cap = 100;
    double time_limit = 30.0;
    int workers = 1;
    std::string mode = "per-set";
    bool print_positive_g = false;
    bool minimize_sum_g = false;
    bool star_only = false;
    bool dump_positive_menus = false;
    bool dump_ledger = false;
    bool test_forced = false;
    for (int argument = 3; argument < argc; ++argument) {
        const std::string option(argv[argument]);
        if (option.rfind("--mode=", 0) == 0)
            mode = option.substr(7);
        else if (option.rfind("--max-r=", 0) == 0)
            max_r = std::stoi(option.substr(8));
        else if (option.rfind("--max-sets=", 0) == 0)
            max_sets = std::stoll(option.substr(11));
        else if (option.rfind("--g-cap=", 0) == 0)
            g_cap = std::stoll(option.substr(8));
        else if (option.rfind("--time-limit=", 0) == 0)
            time_limit = std::stod(option.substr(13));
        else if (option.rfind("--workers=", 0) == 0)
            workers = std::stoi(option.substr(10));
        else if (option == "--print-positive-g")
            print_positive_g = true;
        else if (option == "--minimize-sum-g")
            minimize_sum_g = true;
        else if (option == "--star-only")
            star_only = true;
        else if (option == "--dump-positive-menus")
            dump_positive_menus = true;
        else if (option == "--dump-ledger")
            dump_ledger = true;
        else if (option == "--test-forced")
            test_forced = true;
        else {
            std::cerr << "Unknown option: " << option << '\n';
            return 1;
        }
    }
    if (mode != "per-set" && mode != "relaxed-state" && mode != "exact-menu") {
        std::cerr << "Unknown mode: " << mode << '\n';
        return 1;
    }
    if (n < 1 || k < 1 || k > n || max_r < 1 || max_sets < 1 || g_cap < 0 ||
        time_limit <= 0 || workers < 1) {
        std::cerr << "Require 1 <= k <= n, positive caps, and positive solver limits.\n";
        return 1;
    }

    const std::vector<Vertex> vertices = vertices_of(n, k);
    max_r = std::min(max_r, static_cast<int>(vertices.size()));

    std::vector<Subset> subsets;
    if (star_only) {
        subsets = star_closure(vertices, k, max_r);
        if (subsets.empty()) {
            const int capacity = 1 + k * (n - k);
            std::cerr << "--star-only requires 1 <= --max-r <= " << capacity
                      << " for A(" << n << "," << k << ").\n";
            return 1;
        }
        std::cout << "Star closure of A(" << n << "," << k << ") through R="
                  << max_r << ": " << subsets.size() << " sets; mode=" << mode
                  << "; workers=" << workers
                  << ", time limit=" << time_limit << " s.\n";
    } else {
        const std::int64_t estimate = subset_count_bounded(
            static_cast<int>(vertices.size()), max_r, max_sets);
        if (estimate > max_sets) {
            std::cerr << "Refusing to enumerate more than --max-sets="
                      << max_sets
                      << " subsets (raise the cap explicitly).\n";
            return 1;
        }
        std::cout << "Enumerating " << estimate << " subsets of A(" << n << ","
                  << k << ") through R=" << max_r << "; mode=" << mode
                  << "; workers=" << workers
                  << ", time limit=" << time_limit << " s.\n";
        subsets.reserve(static_cast<std::size_t>(estimate));
        enumerate_subsets(static_cast<int>(vertices.size()), max_r, &subsets);
    }

    // Register all states.
    std::map<State, int> state_index;
    std::vector<State> states;
    const auto register_state = [&](const State &state) {
        const auto [iterator, inserted] =
            state_index.emplace(state, static_cast<int>(states.size()));
        if (inserted)
            states.push_back(state);
        return iterator->second;
    };

    // Compute transitions for each subset.
    std::vector<std::vector<Transition>> choices_by_subset;
    std::vector<int> parent_states;
    choices_by_subset.reserve(subsets.size());
    parent_states.reserve(subsets.size());
    const int m = n - k;
    for (const Subset &subset : subsets) {
        std::vector<Transition> choices;
        const int parent_state = register_state(state_of(subset, vertices, k));
        parent_states.push_back(parent_state);
        if (subset.size() >= 2) {
            const std::int64_t phi_parent = phi_of(subset, vertices, n, k);
            const std::int64_t p_parent =
                potential(static_cast<int>(subset.size()), m);
            for (int position = 0; position < k; ++position) {
                const std::vector<Subset> fibers =
                    fibers_of(subset, vertices, position);
                if (fibers.size() < 2)
                    continue;
                std::int64_t phi_children = 0;
                std::int64_t p_children = 0;
                std::vector<int> child_states;
                for (const Subset &fiber : fibers) {
                    phi_children += phi_of(fiber, vertices, n, k);
                    p_children += potential(static_cast<int>(fiber.size()), m);
                    child_states.push_back(
                        register_state(state_of(fiber, vertices, k)));
                }
                choices.push_back(
                    {parent_state,
                     p_parent - p_children - (phi_parent - phi_children),
                     std::move(child_states)});
            }
        }
        choices_by_subset.push_back(std::move(choices));
    }

    // Count distinct menus for stats.
    std::set<Menu> distinct_menus;
    for (const auto &choices : choices_by_subset) {
        if (!choices.empty())
            distinct_menus.insert(menu_of(choices));
    }

    // Stats.
    const int concrete_sets = static_cast<int>(std::count_if(
        choices_by_subset.begin(), choices_by_subset.end(),
        [](const auto &choices) { return !choices.empty(); }));

    std::cout << "Stats:\n"
              << "  vertices=" << vertices.size() << '\n'
              << "  subsets_enumerated=" << subsets.size() << '\n'
              << "  concrete_sets=" << concrete_sets << '\n'
              << "  profile_states=" << states.size() << '\n'
              << "  distinct_menus=" << distinct_menus.size() << '\n';

    const Subset singleton{0};
    const int singleton_state = register_state(state_of(singleton, vertices, k));

    // Build CP-SAT model and solve.
    SolveResult baseline = build_and_solve(
        mode, minimize_sum_g, g_cap, workers, time_limit,
        choices_by_subset, parent_states, state_index, singleton_state, {});

    const CpSolverResponse &response = baseline.response;
    std::vector<IntVar> &g = baseline.g;

    std::cout << "status: " << CpSolverStatus_Name(response.status()) << '\n';
    if (response.status() != CpSolverStatus::OPTIMAL &&
        response.status() != CpSolverStatus::FEASIBLE)
        return 0;

    const std::int64_t maximum_g = std::accumulate(
        g.begin(), g.end(), std::int64_t{0},
        [&response](std::int64_t acc, const IntVar &var) {
            return std::max(acc, SolutionIntegerValue(response, var));
        });
    std::cout << "G range: [0, " << maximum_g << "] (cap " << g_cap << ")\n";
    if (minimize_sum_g)
        std::cout << "sum G: " << response.objective_value() << '\n';
    std::cout << "Largest profile states:\n";
    std::vector<int> order(states.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int left, int right) {
        return SolutionIntegerValue(response, g[left]) >
               SolutionIntegerValue(response, g[right]);
    });
    for (int rank = 0; rank < std::min(5, static_cast<int>(order.size())); ++rank) {
        const int index = order[rank];
        std::cout << "  G=" << SolutionIntegerValue(response, g[index])
                  << " state=";
        print_state(states[index]);
        std::cout << '\n';
    }
    if (print_positive_g) {
        int positive_count = 0;
        for (const int index : order) {
            const std::int64_t value = SolutionIntegerValue(response, g[index]);
            if (value == 0)
                break;
            ++positive_count;
        }
        std::cout << "Positive-G states: " << positive_count << '\n';
        for (const int index : order) {
            const std::int64_t value = SolutionIntegerValue(response, g[index]);
            if (value == 0)
                break;
            std::cout << "  G=" << value << " state=";
            print_state(states[index]);
            std::cout << '\n';
        }
    }
    if (dump_positive_menus) {
        std::map<std::pair<int, Menu>, std::size_t> representatives;
        for (std::size_t i = 0; i < choices_by_subset.size(); ++i) {
            if (choices_by_subset[i].empty())
                continue;
            const int parent = parent_states[i];
            if (SolutionIntegerValue(response, g[parent]) == 0)
                continue;
            representatives.try_emplace(
                std::make_pair(parent, menu_of(choices_by_subset[i])), i);
        }
        std::cout << "Positive-G menu representatives: "
                  << representatives.size() << '\n';
        for (const auto &[key, index] : representatives) {
            const int parent = key.first;
            std::cout << "  G=" << SolutionIntegerValue(response, g[parent])
                      << " state=";
            print_state(states[parent]);
            std::cout << " V=";
            print_subset(subsets[index], vertices);
            std::cout << '\n';
            dump_split_menu(subsets[index], vertices, n, k);
        }
    }
    if (dump_ledger) {
        std::map<std::pair<int, Menu>, std::size_t> representatives;
        for (std::size_t i = 0; i < choices_by_subset.size(); ++i) {
            if (!choices_by_subset[i].empty()) {
                representatives.try_emplace(
                    std::make_pair(parent_states[i], menu_of(choices_by_subset[i])), i);
            }
        }
        std::cout << "Ledger menu representatives: " << representatives.size() << '\n';
        for (const auto &[key, index] : representatives) {
            static_cast<void>(key);
            dump_split_ledger(subsets[index], vertices, n, k, state_index, g,
                              response);
        }
    }

    // --test-forced: for each positive-G state, pin to 0 and re-solve
    // (feasibility only, no minimize-sum-g). Reports forced vs movable.
    if (test_forced) {
        auto is_infeasible = [](const CpSolverResponse &r) {
            return r.status() == CpSolverStatus::INFEASIBLE;
        };
        auto is_feasible = [](const CpSolverResponse &r) {
            return r.status() == CpSolverStatus::OPTIMAL ||
                   r.status() == CpSolverStatus::FEASIBLE;
        };
        std::cout << "\nForced-value analysis (feasibility, no objective):\n";
        for (const int state_idx : order) {
            const std::int64_t base_value =
                SolutionIntegerValue(response, g[state_idx]);
            if (base_value == 0)
                break;
            std::cout << "  state=";
            print_state(states[state_idx]);
            std::cout << " baseline_G=" << base_value << '\n';

            // Try pinning to 0.
            std::vector<std::int64_t> pin(states.size(), -1);
            pin[state_idx] = 0;
            SolveResult pinned = build_and_solve(
                mode, false, g_cap, workers, time_limit,
                choices_by_subset, parent_states, state_index, singleton_state,
                pin);
            if (is_infeasible(pinned.response)) {
                std::cout << "    pin G=0: INFEASIBLE\n";
                // Binary search for minimum feasible G.
                bool inconclusive = false;
                std::int64_t lo = 1, hi = base_value;
                while (lo < hi) {
                    const std::int64_t mid = (lo + hi) / 2;
                    std::vector<std::int64_t> pin_mid(states.size(), -1);
                    pin_mid[state_idx] = mid;
                    SolveResult trial = build_and_solve(
                        mode, false, g_cap, workers, time_limit,
                        choices_by_subset, parent_states, state_index,
                        singleton_state, pin_mid);
                    if (is_feasible(trial.response))
                        hi = mid;
                    else if (is_infeasible(trial.response))
                        lo = mid + 1;
                    else {
                        inconclusive = true;
                        break;
                    }
                }
                if (inconclusive)
                    std::cout << "    UNKNOWN (binary search inconclusive)\n";
                else
                    std::cout << "    forced_G >= " << lo
                              << " (baseline was " << base_value << ")\n";
            } else if (is_feasible(pinned.response)) {
                std::cout << "    pin G=0: FEASIBLE (movable)\n";
                // Re-solve with minimize_sum_g to find where credit relocates.
                SolveResult optimized = build_and_solve(
                    mode, true, g_cap, workers, time_limit,
                    choices_by_subset, parent_states, state_index, singleton_state,
                    pin);
                if (is_feasible(optimized.response)) {
                    std::vector<int> opt_order(states.size());
                    std::iota(opt_order.begin(), opt_order.end(), 0);
                    std::sort(opt_order.begin(), opt_order.end(),
                        [&](int l, int r) {
                            return SolutionIntegerValue(optimized.response, optimized.g[l]) >
                                   SolutionIntegerValue(optimized.response, optimized.g[r]);
                        });
                    const char *label = optimized.response.status() == CpSolverStatus::OPTIMAL
                        ? "relocated assignment (min sum G="
                        : "relocated assignment (non-optimal, sum G=";
                    bool any_positive = false;
                    for (const int idx : opt_order) {
                        const std::int64_t val =
                            SolutionIntegerValue(optimized.response, optimized.g[idx]);
                        if (val == 0)
                            break;
                        if (!any_positive) {
                            std::cout << "    relocated assignment (min sum G="
                                      << optimized.response.objective_value() << "):\n";
                            any_positive = true;
                        }
                        std::cout << "      G=" << val << " state=";
                        print_state(states[idx]);
                        std::cout << '\n';
                    }
                }
            } else {
                std::cout << "    pin G=0: UNKNOWN (solver timed out)\n";
            }
        }
    }
    return 0;
}
