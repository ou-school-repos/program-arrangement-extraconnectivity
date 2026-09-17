// Exhaustive diagnostic for the defect and collision inequalities.
//
// Usage:
//   ./diagnose_defect_collision n k R [--limit=N]
//
// Vertex-transitivity lets us pin one selected vertex to the origin.  Thus
// every nonempty subset has an equivalent representative containing origin.
// The tool enumerates those representatives and checks:
//
//   D <= E(R)
//   X + D <= C(R) + (n-k)(E(R)-D).
//   Q(S)-Q(S-v) <= 1+ceil(log2 R)+(n-k-1) popcount(R-1).
//   Q(S)-Q(S-T) <= G(R)-G(R-|T|), for |T|=2.
//   Q(S)-Q(S-v) <= 1+ceil(log2 R)+(n-k-1) popcount(R-1).
//
// This is a finite adversarial search, not a universal proof.

#include "../scripts/arrangement_core.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using Int = std::int64_t;
using arrangement::Instance;

Int ceil_log2_plus_one(Int i) {
    Int x = i + 1;
    Int power = 1;
    Int result = 0;
    while (power < x) {
        power <<= 1;
        ++result;
    }
    return result;
}

Int e_seq(int r) {
    Int result = 0;
    for (int i = 0; i < r; ++i)
        result += __builtin_popcount(static_cast<unsigned>(i));
    return result;
}

Int c_constant(int r) {
    Int result = r == 0 ? 0 : r - 1 - e_seq(r);
    for (int i = 1; i < r; ++i)
        result += ceil_log2_plus_one(i);
    return result;
}

struct Best {
    Int value = std::numeric_limits<Int>::max();
    std::vector<int> subset;

    void update(Int candidate, const std::vector<int> &chosen) {
        if (candidate < value) {
            value = candidate;
            subset = chosen;
        }
    }
};

struct PairBest {
    Int value = std::numeric_limits<Int>::max();
    std::vector<int> subset;
    int first = -1;
    int second = -1;
    bool adjacent_tie = false;
    bool nonadjacent_tie = false;

    void update(Int candidate, const std::vector<int> &chosen, int u, int v,
                bool adjacent) {
        if (candidate < value) {
            value = candidate;
            subset = chosen;
            first = u;
            second = v;
            adjacent_tie = adjacent;
            nonadjacent_tie = !adjacent;
        } else if (candidate == value) {
            adjacent_tie = adjacent_tie || adjacent;
            nonadjacent_tie = nonadjacent_tie || !adjacent;
        }
    }
};

struct Search {
    const Instance &graph;
    int target;
    Int node_limit;
    std::uint64_t leaves = 0;
    bool limited = false;
    bool reported_counterexample = false;
    std::vector<bool> selected;
    std::vector<std::vector<int>> root_count;
    std::vector<int> incidence;
    int boundary_size = 0;
    std::vector<int> chosen;
    Best defect_slack;
    Best collision_slack;
    Best boundary_best;
    Best peeling_margin;
    PairBest pair_peeling_margin;

    Search(const Instance &instance, int size, Int limit)
        : graph(instance), target(size), node_limit(limit),
          selected(instance.vertices.size(), false), root_count(instance.k),
          incidence(instance.vertices.size(), 0), chosen{0} {
        for (int position = 0; position < graph.k; ++position)
            root_count[position].assign(graph.lines[position].size(), 0);
        add(0, false);
    }

    void add(int vertex, bool record = true) {
        if (boundary_size > 0 && !selected[vertex] && incidence[vertex] > 0)
            --boundary_size;
        selected[vertex] = true;
        if (record)
            chosen.push_back(vertex);
        for (int position = 0; position < graph.k; ++position) {
            const int root = graph.root_id[position][vertex];
            if (root_count[position][root]++ != 0)
                continue;
            boundary_size += static_cast<int>(std::count_if(
                graph.lines[position][root].begin(),
                graph.lines[position][root].end(), [&](const int member) {
                    return ++incidence[member] == 1 && !selected[member];
                }));
        }
    }

    void remove(int vertex, bool erase_chosen = true) {
        selected[vertex] = false;
        for (int position = 0; position < graph.k; ++position) {
            const int root = graph.root_id[position][vertex];
            if (--root_count[position][root] != 0)
                continue;
            for (const int member : graph.lines[position][root]) {
                --incidence[member];
                if (member != vertex && incidence[member] == 0 &&
                    !selected[member])
                    --boundary_size;
            }
        }
        if (incidence[vertex] > 0)
            ++boundary_size;
        if (erase_chosen)
            chosen.pop_back();
    }

    void evaluate() {
        ++leaves;
        int active_roots = 0;
        for (const auto &counts : root_count)
            active_roots += static_cast<int>(
                std::count_if(counts.begin(), counts.end(),
                              [](int count) { return count > 0; }));

        const Int r = target;
        const Int m = graph.n - graph.k;
        const Int defect = r * graph.k - active_roots;
        const Int E = e_seq(target);
        const Int C = c_constant(target);

        std::vector<bool> boundary(graph.vertices.size(), false);
        for (int position = 0; position < graph.k; ++position) {
            for (int root = 0;
                 root < static_cast<int>(graph.lines[position].size());
                 ++root) {
                if (root_count[position][root] == 0)
                    continue;
                for (int member : graph.lines[position][root])
                    if (!selected[member])
                        boundary[member] = true;
            }
        }
        const Int external_boundary_size = static_cast<Int>(
            std::count(boundary.begin(), boundary.end(), true));
        const Int collision =
            active_roots * m - defect - external_boundary_size;
        const Int defect_margin = E - defect;
        const Int collision_margin = C + defect_margin * m - collision - defect;

        defect_slack.update(defect_margin, chosen);
        collision_slack.update(collision_margin, chosen);
        boundary_best.update(external_boundary_size, chosen);

        const Int q_before = m * r * graph.k - boundary_size;
        const Int threshold =
            1 + ceil_log2_plus_one(r - 1) +
            (m - 1) * __builtin_popcount(static_cast<unsigned>(r - 1));
        Int best_drop = std::numeric_limits<Int>::max();
        for (const int vertex : chosen) {
            remove(vertex, false);
            const Int q_after = m * (r - 1) * graph.k - boundary_size;
            best_drop = std::min(best_drop, q_before - q_after);
            add(vertex, false);
        }
        peeling_margin.update(threshold - best_drop, chosen);

        if (r >= 3) {
            const Int pair_budget = c_constant(target) + m * e_seq(target) -
                                    c_constant(target - 2) -
                                    m * e_seq(target - 2);
            Int best_pair_margin = std::numeric_limits<Int>::min();
            int best_first = -1;
            int best_second = -1;
            for (std::size_t i = 0; i < chosen.size(); ++i) {
                remove(chosen[i], false);
                for (std::size_t j = i + 1; j < chosen.size(); ++j) {
                    remove(chosen[j], false);
                    const Int q_after = m * (r - 2) * graph.k - boundary_size;
                    const Int drop = q_before - q_after;
                    const Int margin = pair_budget - drop;
                    if (margin > best_pair_margin) {
                        best_pair_margin = margin;
                        best_first = chosen[i];
                        best_second = chosen[j];
                    }
                    add(chosen[j], false);
                }
                add(chosen[i], false);
            }
            int equal_coordinates = 0;
            for (int coordinate = 0; coordinate < graph.k; ++coordinate)
                equal_coordinates += graph.vertices[best_first][coordinate] ==
                                     graph.vertices[best_second][coordinate];
            pair_peeling_margin.update(best_pair_margin, chosen, best_first,
                                       best_second,
                                       equal_coordinates == graph.k - 1);
        }

        if (!reported_counterexample &&
            (defect_margin < 0 || collision_margin < 0 ||
             threshold < best_drop)) {
            reported_counterexample = true;
            std::cout << "COUNTEREXAMPLE leaves=" << leaves
                      << " boundary=" << external_boundary_size
                      << " defect=" << defect << " collision=" << collision
                      << " defect-margin=" << defect_margin
                      << " collision-margin=" << collision_margin
                      << " peeling-margin=" << threshold - best_drop
                      << " subset=";
            for (int vertex : chosen)
                std::cout << ' ' << vertex;
            std::cout << '\n';
        }
    }

    void enumerate(int next_vertex) {
        if (limited)
            return;
        if (static_cast<int>(chosen.size()) == target) {
            evaluate();
            if (node_limit > 0 && static_cast<Int>(leaves) >= node_limit)
                limited = true;
            return;
        }
        const int need = target - static_cast<int>(chosen.size());
        const int last = static_cast<int>(graph.vertices.size()) - need;
        for (int vertex = next_vertex; vertex <= last; ++vertex) {
            add(vertex);
            enumerate(vertex + 1);
            remove(vertex);
            if (limited)
                return;
        }
    }
};

void print_result(const char *name, const Best &best) {
    std::cout << name << '=' << best.value << " witness=";
    for (int vertex : best.subset)
        std::cout << ' ' << vertex;
    std::cout << '\n';
}

void print_pair_result(const PairBest &best) {
    std::cout << "min_pair_peeling_margin=" << best.value << " witness=";
    for (int vertex : best.subset)
        std::cout << ' ' << vertex;
    std::cout << " remove=" << best.first << ',' << best.second
              << " adjacent=" << (best.adjacent_tie ? "yes" : "no")
              << " nonadjacent-tie=" << (best.nonadjacent_tie ? "yes" : "no")
              << '\n';
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 4 || std::string(argv[1]) == "-h" ||
        std::string(argv[1]) == "--help") {
        std::cout << "usage: " << argv[0] << " n k R [--limit=N]\n";
        return argc < 4 ? 2 : 0;
    }

    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    const int target = std::stoi(argv[3]);
    Int limit = 0;
    for (int i = 4; i < argc; ++i) {
        const std::string option(argv[i]);
        if (option.rfind("--limit=", 0) == 0)
            limit = std::stoll(option.substr(8));
        else {
            std::cerr << "unknown option: " << option << '\n';
            return 2;
        }
    }

    const Instance graph(n, k);
    if (n <= 0 || k <= 0 || k > n || target < 1 ||
        target > static_cast<int>(graph.vertices.size())) {
        std::cerr << "invalid n, k, or R\n";
        return 2;
    }

    Search search(graph, target, limit);
    search.enumerate(1); // vertex 0 is pinned by vertex-transitivity.

    const Int total = search.leaves;
    std::cout << "A(" << n << ',' << k << ") R=" << target
              << " origin-pinned-subsets=" << total;
    if (search.limited)
        std::cout << " (LIMIT REACHED)";
    std::cout << '\n';
    print_result("min_defect_margin", search.defect_slack);
    print_result("min_collision_margin", search.collision_slack);
    print_result("min_boundary", search.boundary_best);
    print_result("min_peeling_margin", search.peeling_margin);
    print_pair_result(search.pair_peeling_margin);
    std::cout << "defect_lemma_on_scan="
              << (search.defect_slack.value >= 0 ? "yes" : "no") << '\n';
    std::cout << "collision_lemma_on_scan="
              << (search.collision_slack.value >= 0 ? "yes" : "no") << '\n';
    std::cout << "peeling_lemma_on_scan="
              << (search.peeling_margin.value >= 0 ? "yes" : "no") << '\n';
    std::cout << "pair_peeling_lemma_on_scan="
              << (search.pair_peeling_margin.value >= 0 ? "yes" : "no") << '\n';
    return search.defect_slack.value >= 0 &&
                   search.collision_slack.value >= 0 &&
                   search.pair_peeling_margin.value >= 0
               ? 0
               : 1;
}
