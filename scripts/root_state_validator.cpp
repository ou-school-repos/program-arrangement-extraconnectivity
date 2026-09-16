// Exact incremental root/fiber state validator for small arrangement graphs.
// Usage: ./root_state_validator [n] [k] [max-subset-size]

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

struct Metrics {
    int boundary;
    int incidences;
    int collisions;
    int active_roots;
};

struct Instance {
    int n;
    int k;
    std::vector<std::vector<int>> vertices;
    std::vector<std::vector<std::vector<int>>> lines;
    std::vector<std::vector<int>> root_id;
    std::unordered_map<int, int> index;

    int encode(const std::vector<int> &vertex) const {
        return std::accumulate(vertex.begin(), vertex.end(), 0,
                               [&](const int code, const int symbol) {
                                   return n * code + symbol;
                               });
    }

    void enumerate(std::vector<int> &prefix, std::vector<bool> &used) {
        if (static_cast<int>(prefix.size()) == k) {
            const int id = static_cast<int>(vertices.size());
            vertices.push_back(prefix);
            index.emplace(encode(prefix), id);
            return;
        }
        for (int symbol = 0; symbol < n; ++symbol) {
            if (used[symbol])
                continue;
            used[symbol] = true;
            prefix.push_back(symbol);
            enumerate(prefix, used);
            prefix.pop_back();
            used[symbol] = false;
        }
    }

    explicit Instance(int alphabet_size, int dimension)
        : n(alphabet_size), k(dimension) {
        std::vector<int> prefix;
        std::vector<bool> used(n, false);
        enumerate(prefix, used);
        root_id.assign(k, std::vector<int>(vertices.size(), -1));
        lines.resize(k);
        for (int position = 0; position < k; ++position) {
            std::unordered_map<int, int> roots;
            for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
                std::vector<int> root;
                for (int cursor = 0; cursor < k; ++cursor)
                    if (cursor != position)
                        root.push_back(vertices[id][cursor]);
                const int code = encode(root);
                auto [entry, inserted] =
                    roots.emplace(code, static_cast<int>(roots.size()));
                if (inserted)
                    lines[position].push_back({});
                root_id[position][id] = entry->second;
                lines[position][entry->second].push_back(id);
            }
        }
    }
};

std::vector<int> canonicalize(const Instance &instance,
                              const std::vector<int> &subset) {
    std::vector<int> best_subset = subset;
    std::sort(best_subset.begin(), best_subset.end());
    std::vector<int> coordinate_permutation(instance.k);
    std::iota(coordinate_permutation.begin(), coordinate_permutation.end(), 0);

    do {
        std::vector<int> free_symbols(instance.n - instance.k);
        std::iota(free_symbols.begin(), free_symbols.end(), instance.k);
        do {
            std::vector<int> symbol_permutation(instance.n);
            for (int position = 0; position < instance.k; ++position)
                symbol_permutation[coordinate_permutation[position]] = position;
            for (int index = 0; index < instance.n - instance.k; ++index)
                symbol_permutation[instance.k + index] = free_symbols[index];

            std::vector<int> mapped;
            mapped.reserve(subset.size());
            for (int vertex_id : subset) {
                const std::vector<int> &vertex = instance.vertices[vertex_id];
                std::vector<int> image(instance.k);
                for (int position = 0; position < instance.k; ++position)
                    image[position] = symbol_permutation
                        [vertex[coordinate_permutation[position]]];
                mapped.push_back(instance.index.at(instance.encode(image)));
            }
            std::sort(mapped.begin(), mapped.end());
            if (mapped < best_subset)
                best_subset = std::move(mapped);
        } while (
            std::next_permutation(free_symbols.begin(), free_symbols.end()));
    } while (std::next_permutation(coordinate_permutation.begin(),
                                   coordinate_permutation.end()));
    return best_subset;
}

struct State {
    const Instance &instance;
    std::vector<bool> selected;
    std::vector<std::vector<int>> root_count;
    std::vector<int> incidence;
    std::vector<bool> external;
    int selected_count = 0;
    int boundary_size = 0;
    int total_incidences = 0;

    explicit State(const Instance &graph)
        : instance(graph), selected(graph.vertices.size(), false),
          root_count(graph.k), incidence(graph.vertices.size(), 0),
          external(graph.vertices.size(), false) {
        for (int position = 0; position < graph.k; ++position)
            root_count[position].assign(graph.lines[position].size(), 0);
    }

    void refresh(int vertex) {
        const bool wanted = !selected[vertex] && incidence[vertex] > 0;
        if (wanted == external[vertex])
            return;
        external[vertex] = wanted;
        boundary_size += wanted ? 1 : -1;
    }

    void add(int vertex) {
        assert(!selected[vertex]);
        selected[vertex] = true;
        ++selected_count;
        for (int position = 0; position < instance.k; ++position) {
            const int root = instance.root_id[position][vertex];
            if (root_count[position][root]++ != 0)
                continue;
            ++total_incidences;
            for (int member : instance.lines[position][root]) {
                ++incidence[member];
                refresh(member);
            }
        }
        // The selected vertex itself is no longer external.
        refresh(vertex);
        total_incidences = active_roots() * (instance.n - instance.k + 1) -
                           selected_count * instance.k;
    }

    void remove(int vertex) {
        assert(selected[vertex]);
        selected[vertex] = false;
        --selected_count;
        for (int position = 0; position < instance.k; ++position) {
            const int root = instance.root_id[position][vertex];
            if (--root_count[position][root] != 0)
                continue;
            for (int member : instance.lines[position][root]) {
                --incidence[member];
                refresh(member);
            }
        }
        refresh(vertex);
        total_incidences = active_roots() * (instance.n - instance.k + 1) -
                           selected_count * instance.k;
    }

    int active_roots() const {
        int total = 0;
        for (const auto &roots : root_count) {
            total += static_cast<int>(
                std::count_if(roots.begin(), roots.end(),
                              [](const int count) { return count > 0; }));
        }
        return total;
    }

    int collisions() const { return total_incidences - boundary_size; }
};

Metrics from_scratch(const Instance &instance, const State &state) {
    std::vector<bool> external(instance.vertices.size(), false);
    int active_roots = 0;
    int incidences = 0;
    for (int position = 0; position < instance.k; ++position) {
        for (std::size_t root = 0; root < instance.lines[position].size();
             ++root) {
            const int selected_on_line = static_cast<int>(std::count_if(
                instance.lines[position][root].begin(),
                instance.lines[position][root].end(),
                [&](const int vertex) { return state.selected[vertex]; }));
            if (selected_on_line == 0)
                continue;
            ++active_roots;
            incidences +=
                static_cast<int>(instance.lines[position][root].size()) -
                selected_on_line;
            for (int vertex : instance.lines[position][root])
                if (!state.selected[vertex])
                    external[vertex] = true;
        }
    }
    const int boundary =
        static_cast<int>(std::count(external.begin(), external.end(), true));
    return {boundary, incidences, incidences - boundary, active_roots};
}

void check(const Instance &instance, const State &state) {
    const Metrics expected = from_scratch(instance, state);
    assert(expected.boundary == state.boundary_size);
    assert(expected.incidences == state.total_incidences);
    assert(expected.collisions == state.collisions());
    assert(expected.active_roots == state.active_roots());
}

void exhaustive_small_test() {
    const Instance instance(4, 2);
    State state(instance);
    std::vector<int> chosen;
    std::function<void(int)> visit = [&](int next) {
        check(instance, state);
        if (static_cast<int>(chosen.size()) == 5)
            return;
        for (int vertex = next;
             vertex < static_cast<int>(instance.vertices.size()); ++vertex) {
            state.add(vertex);
            chosen.push_back(vertex);
            visit(vertex + 1);
            chosen.pop_back();
            state.remove(vertex);
        }
    };
    visit(0);
    std::cout << "A(4,2): exhaustive incremental checks passed\n";
}

void random_test(const Instance &instance) {
    State state(instance);
    std::mt19937 rng(20260916);
    for (int trial = 0; trial < 1000; ++trial) {
        std::vector<int> chosen;
        for (int step = 0; step < 40; ++step) {
            const bool can_add = chosen.size() < instance.vertices.size();
            const bool can_remove = !chosen.empty();
            const bool add_vertex = can_add && (!can_remove || (rng() & 1));
            if (add_vertex) {
                std::vector<bool> present(instance.vertices.size(), false);
                for (int vertex : chosen)
                    present[vertex] = true;
                std::vector<int> candidates;
                candidates.reserve(present.size());
                std::vector<int> vertex_ids(present.size());
                std::iota(vertex_ids.begin(), vertex_ids.end(), 0);
                std::copy_if(
                    vertex_ids.begin(), vertex_ids.end(),
                    std::back_inserter(candidates),
                    [&](const int vertex) { return !present[vertex]; });
                const int vertex = candidates[rng() % candidates.size()];
                chosen.push_back(vertex);
                state.add(vertex);
            } else {
                const int position = rng() % chosen.size();
                const int vertex = chosen[position];
                chosen.erase(chosen.begin() + position);
                state.remove(vertex);
            }
            check(instance, state);
        }
        while (!chosen.empty()) {
            state.remove(chosen.back());
            chosen.pop_back();
            check(instance, state);
        }
    }
    std::cout << "A(" << instance.n << ',' << instance.k
              << "): randomized add/remove checks passed\n";
}

State state_from_subset(const Instance &instance,
                        const std::vector<int> &subset) {
    State state(instance);
    for (int vertex : subset)
        state.add(vertex);
    return state;
}

void canonicalization_test() {
    const Instance instance(5, 3);
    const int origin = instance.index.at(instance.encode({0, 1, 2}));
    std::mt19937 rng(20260916);
    std::vector<std::vector<int>> representatives;
    for (int trial = 0; trial < 100; ++trial) {
        std::vector<int> subset{origin};
        std::vector<bool> present(instance.vertices.size(), false);
        present[origin] = true;
        const int size = 1 + (rng() % 5);
        while (static_cast<int>(subset.size()) < size) {
            const int vertex = rng() % instance.vertices.size();
            if (present[vertex])
                continue;
            present[vertex] = true;
            subset.push_back(vertex);
        }
        std::sort(subset.begin(), subset.end());
        const std::vector<int> representative = canonicalize(instance, subset);
        const State original_state = state_from_subset(instance, subset);
        const State representative_state =
            state_from_subset(instance, representative);
        check(instance, original_state);
        check(instance, representative_state);
        assert(original_state.boundary_size ==
               representative_state.boundary_size);
        assert(original_state.total_incidences ==
               representative_state.total_incidences);
        assert(original_state.collisions() ==
               representative_state.collisions());
        assert(original_state.active_roots() ==
               representative_state.active_roots());
        representatives.push_back(representative);
    }
    std::sort(representatives.begin(), representatives.end());
    representatives.erase(
        std::unique(representatives.begin(), representatives.end()),
        representatives.end());
    std::cout << "A(5,3): canonicalization checks passed; "
              << representatives.size() << " representatives in 100 samples\n";
}

int main(int argc, char **argv) {
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0]
                  << "  (runs the built-in A(4,2) and A(5,3) tests)\n";
        return 0;
    }
    if (argc > 1) {
        std::cerr << "usage: " << argv[0]
                  << "  (runs the built-in A(4,2) and A(5,3) tests)\n";
        return 2;
    }
    exhaustive_small_test();
    random_test(Instance(5, 3));
    canonicalization_test();
    return 0;
}
