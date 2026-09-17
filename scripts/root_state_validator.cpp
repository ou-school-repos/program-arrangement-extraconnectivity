// Exact incremental root/fiber state validator for small arrangement graphs.
// Usage: ./root_state_validator [n] [k] [max-subset-size]

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

struct Metrics {
    int boundary;
    int incidences;
    int collisions;
    int active_roots;
};

struct IntegerRange {
    int first;
    int last;
};

IntegerRange parse_range(const std::string &text) {
    const std::size_t separator = text.find('-');
    if (separator == std::string::npos) {
        const int value = std::stoi(text);
        return {value, value};
    }
    const int first = std::stoi(text.substr(0, separator));
    const int last = std::stoi(text.substr(separator + 1));
    if (first > last)
        throw std::invalid_argument("descending range");
    return {first, last};
}

boost::multiprecision::cpp_int vertex_count(int n, int k) {
    boost::multiprecision::cpp_int count = 1;
    for (int offset = 0; offset < k; ++offset)
        count *= n - offset;
    return count;
}

boost::multiprecision::cpp_int total_nodes(int n, int k, int target_size) {
    const boost::multiprecision::cpp_int vertices = vertex_count(n, k);
    boost::multiprecision::cpp_int choose = 1;
    boost::multiprecision::cpp_int total = 0;
    for (int depth = 1; depth <= target_size; ++depth) {
        choose *= vertices - depth + 1;
        choose /= depth;
        total += choose;
    }
    return total;
}

int precompute_ranges(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "usage: " << argv[0]
                  << " --pre n[-n] k[-k] target[-target]\n";
        return 2;
    }
    try {
        const IntegerRange n_range = parse_range(argv[2]);
        const IntegerRange k_range = parse_range(argv[3]);
        const IntegerRange target_range = parse_range(argv[4]);
        for (int n = n_range.first; n <= n_range.last; ++n) {
            for (int k = k_range.first; k <= k_range.last; ++k) {
                for (int target = target_range.first;
                     target <= target_range.last; ++target) {
                    if (n < 1 || k < 1 || k > n || target < 1) {
                        std::cout << "A(" << n << ',' << k << ") R=" << target
                                  << ": invalid\n";
                        continue;
                    }
                    const auto vertices = vertex_count(n, k);
                    if (vertices < target) {
                        std::cout << "A(" << n << ',' << k << ") R=" << target
                                  << ": invalid (target exceeds " << vertices
                                  << " vertices)\n";
                        continue;
                    }
                    const auto leaves = total_nodes(n, k, target) -
                                        total_nodes(n, k, target - 1);
                    std::cout
                        << "A(" << n << ',' << k << ") R=" << target
                        << ": vertices=" << vertices << " leaves=" << leaves
                        << " total-nodes=" << total_nodes(n, k, target) << '\n';
                }
            }
        }
    } catch (const std::exception &) {
        std::cerr << "usage: " << argv[0]
                  << " --pre n[-n] k[-k] target[-target]\n";
        return 2;
    }
    return 0;
}

struct Instance {
    int n;
    int k;
    std::vector<std::vector<int>> vertices;
    std::vector<std::vector<std::vector<int>>> lines;
    std::vector<std::vector<int>> root_id;
    std::unordered_map<int, int> index;
    std::vector<int> flat_index;

    int encode(const std::vector<int> &vertex) const {
        return std::accumulate(vertex.begin(), vertex.end(), 0,
                               [&](const int code, const int symbol) {
                                   return n * code + symbol;
                               });
    }

    int lookup(int code) const {
        if (code >= 0 && code < static_cast<int>(flat_index.size())) {
            const int vertex_id = flat_index[code];
            if (vertex_id >= 0)
                return vertex_id;
        }
        return index.at(code);
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
        long long index_capacity = 1;
        for (int coordinate = 0; coordinate < k; ++coordinate)
            index_capacity *= n;
        if (index_capacity <= 1'000'000) {
            flat_index.assign(static_cast<std::size_t>(index_capacity), -1);
            for (const auto &[code, vertex_id] : index)
                flat_index[code] = vertex_id;
        }
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

    int optimistic_bound(int target_size) const {
        const int remaining = target_size - selected_count;
        if (remaining <= 0)
            return boundary_size;

        const int bleed_bound = boundary_size - remaining;
        int incidence_bound_total = 0;
        int incidence_bound_max = 0;
        for (const auto &roots : root_count) {
            const int active = static_cast<int>(
                std::count_if(roots.begin(), roots.end(),
                              [](const int count) { return count > 0; }));
            const int coordinate_bound = std::max(
                0, active * (instance.n - instance.k + 1) - target_size);
            incidence_bound_total += coordinate_bound;
            incidence_bound_max =
                std::max(incidence_bound_max, coordinate_bound);
        }
        const int aggregate_bound =
            (incidence_bound_total + instance.k - 1) / instance.k;
        return std::max({bleed_bound, aggregate_bound, incidence_bound_max, 0});
    }
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

struct SearchProgress {
    static constexpr std::uint64_t kReportInterval = 10'000'000;
    std::chrono::steady_clock::time_point started =
        std::chrono::steady_clock::now();
    std::uint64_t next_report = kReportInterval;

    void report(std::uint64_t nodes, int depth, int best) {
        if (nodes < next_report)
            return;
        const double elapsed = std::chrono::duration<double>(
                                   std::chrono::steady_clock::now() - started)
                                   .count();
        const double rate = elapsed > 0.0 ? nodes / elapsed : 0.0;
        std::cout << "  progress nodes=" << nodes << " depth=" << depth
                  << " best=" << best << " rate=" << rate << "/s\n"
                  << std::flush;
        next_report += kReportInterval;
    }
};

void raw_dfs(const Instance &instance, State &state, std::vector<int> &subset,
             int target_size, int &best_boundary, std::uint64_t &nodes_visited,
             bool use_bound, SearchProgress &progress) {
    if (state.selected_count == target_size) {
        best_boundary = std::min(best_boundary, state.boundary_size);
        return;
    }
    if (use_bound && state.optimistic_bound(target_size) >= best_boundary)
        return;

    const int start = subset.empty() ? 0 : subset.back() + 1;
    for (int vertex = start;
         vertex < static_cast<int>(instance.vertices.size()); ++vertex) {
        state.add(vertex);
        subset.push_back(vertex);
        ++nodes_visited;
        progress.report(nodes_visited, state.selected_count, best_boundary);
        raw_dfs(instance, state, subset, target_size, best_boundary,
                nodes_visited, use_bound, progress);
        subset.pop_back();
        state.remove(vertex);
    }
}

struct Automorphism {
    std::array<std::uint8_t, 10> coordinates{};
    std::array<std::uint8_t, 10> symbols{};

    int apply(int vertex_id, const Instance &instance) const {
        std::array<int, 10> positions{};
        std::iota(positions.begin(), positions.begin() + instance.k, 0);
        const int code = std::accumulate(
            positions.begin(), positions.begin() + instance.k, 0,
            [this, vertex_id, &instance](const int value, const int position) {
                return instance.n * value +
                       symbols[instance
                                   .vertices[vertex_id][coordinates[position]]];
            });
        return instance.lookup(code);
    }
};

std::vector<Automorphism> origin_stabilizer(const Instance &instance) {
    std::vector<Automorphism> permutations;
    std::vector<int> coordinate_permutation(instance.k);
    std::iota(coordinate_permutation.begin(), coordinate_permutation.end(), 0);

    do {
        std::vector<int> free_symbols(instance.n - instance.k);
        std::iota(free_symbols.begin(), free_symbols.end(), instance.k);
        do {
            Automorphism automorphism;
            for (int position = 0; position < instance.k; ++position)
                automorphism.symbols[coordinate_permutation[position]] =
                    position;
            for (int index = 0; index < instance.n - instance.k; ++index)
                automorphism.symbols[instance.k + index] = free_symbols[index];
            for (int position = 0; position < instance.k; ++position)
                automorphism.coordinates[position] =
                    coordinate_permutation[position];
            permutations.push_back(automorphism);
        } while (
            std::next_permutation(free_symbols.begin(), free_symbols.end()));
    } while (std::next_permutation(coordinate_permutation.begin(),
                                   coordinate_permutation.end()));
    return permutations;
}

struct OrbitProgress {
    SearchProgress progress;
    std::uint64_t skipped = 0;
};

void orbit_dfs(const Instance &instance, State &state, std::vector<int> &subset,
               int target_size, int &best_boundary,
               std::uint64_t &nodes_visited,
               const std::vector<Automorphism> &stabilizer,
               OrbitProgress &progress) {
    if (state.selected_count == target_size) {
        best_boundary = std::min(best_boundary, state.boundary_size);
        return;
    }
    if (state.optimistic_bound(target_size) >= best_boundary)
        return;

    const int start = subset.empty() ? 0 : subset.back() + 1;
    for (int vertex = start;
         vertex < static_cast<int>(instance.vertices.size()); ++vertex) {
        bool orbit_representative = true;
        orbit_representative = !std::any_of(
            stabilizer.begin(), stabilizer.end(),
            [&instance, vertex](const Automorphism &automorphism) {
                return automorphism.apply(vertex, instance) < vertex;
            });
        if (!orbit_representative) {
            ++progress.skipped;
            continue;
        }

        std::vector<Automorphism> next_stabilizer;
        std::copy_if(stabilizer.begin(), stabilizer.end(),
                     std::back_inserter(next_stabilizer),
                     [&instance, vertex](const Automorphism &automorphism) {
                         return automorphism.apply(vertex, instance) == vertex;
                     });

        state.add(vertex);
        subset.push_back(vertex);
        ++nodes_visited;
        progress.progress.report(nodes_visited, state.selected_count,
                                 best_boundary);
        orbit_dfs(instance, state, subset, target_size, best_boundary,
                  nodes_visited, next_stabilizer, progress);
        subset.pop_back();
        state.remove(vertex);
    }
}

void compare_orbit_pruning(const Instance &instance, int target_size) {
    std::cout << "--- Orbit test A(" << instance.n << ',' << instance.k
              << ") R=" << target_size << " ---\n";
    std::vector<int> origin(instance.k);
    std::iota(origin.begin(), origin.end(), 0);
    const int origin_id = instance.index.at(instance.encode(origin));
    const auto stabilizer = origin_stabilizer(instance);

    State state(instance);
    state.add(origin_id);
    std::vector<int> subset{origin_id};
    int best_boundary = INT_MAX;
    std::uint64_t nodes = 0;
    OrbitProgress progress;
    orbit_dfs(instance, state, subset, target_size, best_boundary, nodes,
              stabilizer, progress);
    std::cout << "Orbit-pruned: best=" << best_boundary << " nodes=" << nodes
              << " orbit-skipped=" << progress.skipped
              << " stabilizer=" << stabilizer.size() << '\n';
}

void compare_dfs_pruning(const Instance &instance, int target_size) {
    std::cout << "--- Testing A(" << instance.n << ',' << instance.k
              << ") R=" << target_size << " ---\n";
    int best_unpruned = INT_MAX;
    std::uint64_t nodes_unpruned = 0;
    State unpruned_state(instance);
    std::vector<int> unpruned_subset;
    SearchProgress unpruned_progress;
    raw_dfs(instance, unpruned_state, unpruned_subset, target_size,
            best_unpruned, nodes_unpruned, false, unpruned_progress);

    int best_pruned = INT_MAX;
    std::uint64_t nodes_pruned = 0;
    State pruned_state(instance);
    std::vector<int> pruned_subset;
    SearchProgress pruned_progress;
    raw_dfs(instance, pruned_state, pruned_subset, target_size, best_pruned,
            nodes_pruned, true, pruned_progress);

    assert(best_unpruned == best_pruned);
    const double reduction =
        100.0 * (1.0 - static_cast<double>(nodes_pruned) /
                           static_cast<double>(
                               std::max<std::uint64_t>(1, nodes_unpruned)));
    std::cout << "Unpruned: best=" << best_unpruned
              << " nodes=" << nodes_unpruned << '\n'
              << "Pruned:   best=" << best_pruned << " nodes=" << nodes_pruned
              << '\n'
              << "Node reduction: " << reduction << "%\n";
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
    if (argc > 1 && std::string(argv[1]) == "--pre")
        return precompute_ranges(argc, argv);
    if (argc == 5 && std::string(argv[1]) == "--orbit") {
        try {
            const int n = std::stoi(argv[2]);
            const int k = std::stoi(argv[3]);
            const int target_size = std::stoi(argv[4]);
            const Instance instance(n, k);
            if (n < 1 || k < 1 || k > n || target_size < 1 ||
                target_size > static_cast<int>(instance.vertices.size()))
                throw std::invalid_argument("invalid instance");
            compare_orbit_pruning(instance, target_size);
            return 0;
        } catch (const std::exception &) {
            // Fall through to usage.
        }
    }
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0] << " [n] [k] [target-size]\n"
                  << "       " << argv[0] << " --orbit [n] [k] [target-size]\n"
                  << "       " << argv[0]
                  << " (runs the built-in A(4,2) and A(5,3) tests)\n";
        return 0;
    }
    if (argc == 4) {
        try {
            const int n = std::stoi(argv[1]);
            const int k = std::stoi(argv[2]);
            const int target_size = std::stoi(argv[3]);
            if (n < 1 || k < 1 || k > n || target_size < 1 ||
                target_size > static_cast<int>(Instance(n, k).vertices.size()))
                throw std::invalid_argument("invalid instance");
            compare_dfs_pruning(Instance(n, k), target_size);
            return 0;
        } catch (const std::exception &) {
            // Fall through to usage.
        }
    }
    if (argc > 1) {
        std::cerr << "usage: " << argv[0] << " [n] [k] [target-size]\n"
                  << "       " << argv[0] << " --orbit [n] [k] [target-size]\n"
                  << "       " << argv[0]
                  << " (runs the built-in A(4,2) and A(5,3) tests)\n";
        return 2;
    }
    exhaustive_small_test();
    random_test(Instance(5, 3));
    canonicalization_test();
    const Instance comparison_instance(5, 3);
    compare_dfs_pruning(comparison_instance, 4);
    compare_dfs_pruning(comparison_instance, 5);
    compare_dfs_pruning(comparison_instance, 6);
    return 0;
}
