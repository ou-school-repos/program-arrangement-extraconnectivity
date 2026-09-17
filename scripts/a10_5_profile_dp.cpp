// Exact fiber-profile DP prototype for arrangement-graph isoperimetry.
// This draft deliberately caches exact profiles; symmetry canonicalization is
// a separate layer and must be validated before it is enabled.

#include <algorithm>
#include <atomic>
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <climits>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

using boost::multiprecision::cpp_int;

cpp_int falling_factorial(const int n, const int k) {
    if (k < 0 || k > n)
        return 0;
    cpp_int result = 1;
    for (int i = 0; i < k; ++i)
        result *= n - i;
    return result;
}

cpp_int raw_tree_nodes(const int vertices, const int target,
                       const bool pinned_origin) {
    cpp_int total = 0;
    for (int depth = 1; depth <= target; ++depth) {
        total += pinned_origin ? falling_factorial(vertices - 1, depth - 1)
                               : falling_factorial(vertices, depth);
    }
    return total;
}

struct Instance {
    int n;
    int k;
    std::vector<std::vector<int>> vertices;
    std::unordered_map<int, int> index;
    std::vector<std::vector<std::vector<int>>> lines;
    std::vector<std::vector<int>> root_id;

    int encode(const std::vector<int> &vertex) const {
        return std::accumulate(vertex.begin(), vertex.end(), 0,
                               [this](const int code, const int symbol) {
                                   return n * code + symbol;
                               });
    }

    void enumerate(std::vector<int> &prefix, std::vector<bool> &used) {
        if (static_cast<int>(prefix.size()) == k) {
            index.emplace(encode(prefix), static_cast<int>(vertices.size()));
            vertices.push_back(prefix);
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

    Instance(const int alphabet, const int dimension)
        : n(alphabet), k(dimension) {
        std::vector<int> prefix;
        std::vector<bool> used(n, false);
        enumerate(prefix, used);
        root_id.assign(k, std::vector<int>(vertices.size(), -1));
        lines.resize(k);
        for (int position = 0; position < k; ++position) {
            std::vector<int> root_codes;
            for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
                std::vector<int> root;
                root.reserve(k - 1);
                for (int coordinate = 0; coordinate < k; ++coordinate)
                    if (coordinate != position)
                        root.push_back(vertices[id][coordinate]);
                const int code = encode(root);
                auto found =
                    std::find(root_codes.begin(), root_codes.end(), code);
                int root_id_value = 0;
                if (found == root_codes.end()) {
                    root_id_value = static_cast<int>(root_codes.size());
                    root_codes.push_back(code);
                    lines[position].emplace_back();
                } else {
                    root_id_value =
                        static_cast<int>(found - root_codes.begin());
                }
                root_id[position][id] = root_id_value;
                lines[position][root_id_value].push_back(id);
            }
        }
    }
};

struct Automorphism {
    std::vector<int> coordinates;
    std::vector<int> symbols;

    int apply(const int vertex, const Instance &instance) const {
        const int code = std::accumulate(
            coordinates.begin(), coordinates.begin() + instance.k, 0,
            [this, &instance, vertex](const int value, const int coordinate) {
                return instance.n * value +
                       symbols[instance.vertices[vertex][coordinate]];
            });
        return instance.index.at(code);
    }
};

std::vector<Automorphism> origin_stabilizer(const Instance &instance) {
    std::vector<Automorphism> group;
    std::vector<int> coordinates(instance.k);
    std::iota(coordinates.begin(), coordinates.end(), 0);
    do {
        std::vector<int> free_symbols(instance.n - instance.k);
        std::iota(free_symbols.begin(), free_symbols.end(), instance.k);
        do {
            Automorphism automorphism{coordinates,
                                      std::vector<int>(instance.n)};
            for (int position = 0; position < instance.k; ++position)
                automorphism.symbols[coordinates[position]] = position;
            for (int i = 0; i < instance.n - instance.k; ++i)
                automorphism.symbols[instance.k + i] = free_symbols[i];
            group.push_back(std::move(automorphism));
        } while (
            std::next_permutation(free_symbols.begin(), free_symbols.end()));
    } while (std::next_permutation(coordinates.begin(), coordinates.end()));
    return group;
}

std::vector<int> canonical_key(const std::vector<int> &subset,
                               const Instance &instance,
                               const std::vector<Automorphism> &stabilizer) {
    // Ordered pairs of injective words are classified by their equality
    // pattern.  After using vertex transitivity to fix the first vertex,
    // coordinate and symbol permutations can map any pair with the same
    // number of matching coordinates to one another.  This avoids scanning
    // the full stabilizer for the R=2 stress test.
    if (subset.size() == 2) {
        int matching_coordinates = 0;
        for (int position = 0; position < instance.k; ++position) {
            matching_coordinates +=
                instance.vertices[subset[0]][position] ==
                        instance.vertices[subset[1]][position]
                    ? 1
                    : 0;
        }
        return {-2, matching_coordinates};
    }
    if (subset.size() == 1)
        return {-1};

    std::vector<int> best;
    bool initialized = false;
    for (const int anchor : subset) {
        std::vector<int> shift(instance.n);
        std::vector<bool> anchor_symbol(instance.n, false);
        for (int position = 0; position < instance.k; ++position) {
            shift[instance.vertices[anchor][position]] = position;
            anchor_symbol[instance.vertices[anchor][position]] = true;
        }
        int next_symbol = instance.k;
        for (int symbol = 0; symbol < instance.n; ++symbol) {
            if (!anchor_symbol[symbol])
                shift[symbol] = next_symbol++;
        }

        std::vector<int> shifted;
        shifted.reserve(subset.size());
        for (const int vertex : subset) {
            int code = 0;
            for (int position = 0; position < instance.k; ++position)
                code = instance.n * code +
                       shift[instance.vertices[vertex][position]];
            shifted.push_back(instance.index.at(code));
        }

        for (const auto &automorphism : stabilizer) {
            std::vector<int> mapped;
            mapped.reserve(shifted.size());
            std::transform(shifted.begin(), shifted.end(),
                           std::back_inserter(mapped),
                           [&automorphism, &instance](const int vertex) {
                               return automorphism.apply(vertex, instance);
                           });
            std::sort(mapped.begin(), mapped.end());
            if (!initialized || mapped < best) {
                best = std::move(mapped);
                initialized = true;
            }
        }
    }
    return best;
}

std::uint64_t bit_mask(const int bit) { return std::uint64_t{1} << (bit % 64); }

int word_index(const int bit) { return bit / 64; }

struct ProfileState {
    const Instance &instance;
    std::vector<std::uint64_t> selected;
    std::vector<std::vector<std::uint64_t>> active_roots;
    std::vector<std::vector<int>> root_count;
    std::vector<int> incidence;
    std::vector<std::uint64_t> boundary;
    int selected_count = 0;
    int boundary_size = 0;
    int total_incidences = 0;

    explicit ProfileState(const Instance &graph)
        : instance(graph), selected((graph.vertices.size() + 63) / 64, 0),
          active_roots(graph.k), root_count(graph.k),
          incidence(graph.vertices.size(), 0),
          boundary((graph.vertices.size() + 63) / 64, 0) {
        for (int position = 0; position < graph.k; ++position) {
            root_count[position].assign(graph.lines[position].size(), 0);
            active_roots[position].assign(
                (graph.lines[position].size() + 63) / 64, 0);
        }
    }

    bool selected_at(const int vertex) const {
        return (selected[word_index(vertex)] & bit_mask(vertex)) != 0;
    }

    void set_boundary(const int vertex, const bool value) {
        const bool old = (boundary[word_index(vertex)] & bit_mask(vertex)) != 0;
        if (old == value)
            return;
        if (value)
            boundary[word_index(vertex)] |= bit_mask(vertex);
        else
            boundary[word_index(vertex)] &= ~bit_mask(vertex);
        boundary_size += value ? 1 : -1;
    }

    void add(const int vertex) {
        selected[word_index(vertex)] |= bit_mask(vertex);
        ++selected_count;
        for (int position = 0; position < instance.k; ++position) {
            const int root = instance.root_id[position][vertex];
            if (root_count[position][root]++ != 0)
                continue;
            active_roots[position][word_index(root)] |= bit_mask(root);
            for (const int member : instance.lines[position][root]) {
                ++incidence[member];
                if (!selected_at(member))
                    set_boundary(member, true);
            }
        }
        set_boundary(vertex, false);
        total_incidences = active_root_count() * (instance.n - instance.k + 1) -
                           selected_count * instance.k;
    }

    void remove(const int vertex) {
        selected[word_index(vertex)] &= ~bit_mask(vertex);
        --selected_count;
        for (int position = 0; position < instance.k; ++position) {
            const int root = instance.root_id[position][vertex];
            if (--root_count[position][root] != 0)
                continue;
            active_roots[position][word_index(root)] &= ~bit_mask(root);
            for (const int member : instance.lines[position][root]) {
                --incidence[member];
                if (!selected_at(member))
                    set_boundary(member, incidence[member] > 0);
            }
        }
        set_boundary(vertex, incidence[vertex] > 0);
        total_incidences = active_root_count() * (instance.n - instance.k + 1) -
                           selected_count * instance.k;
    }

    int active_root_count() const {
        return std::accumulate(
            root_count.begin(), root_count.end(), 0,
            [](const int total, const std::vector<int> &roots) {
                return total + static_cast<int>(std::count_if(
                                   roots.begin(), roots.end(),
                                   [](const int count) { return count > 0; }));
            });
    }

    int optimistic_bound(const int target) const {
        const int remaining = target - selected_count;
        if (remaining <= 0)
            return boundary_size;
        const int bleed = boundary_size - remaining;
        int aggregate = 0;
        int coordinate_max = 0;
        for (const auto &roots : root_count) {
            const int active = static_cast<int>(
                std::count_if(roots.begin(), roots.end(),
                              [](const int count) { return count > 0; }));
            const int floor =
                std::max(0, active * (instance.n - instance.k + 1) - target);
            aggregate += floor;
            coordinate_max = std::max(coordinate_max, floor);
        }
        return std::max({bleed, (aggregate + instance.k - 1) / instance.k,
                         coordinate_max, 0});
    }

    std::vector<std::uint64_t> exact_key() const {
        std::vector<std::uint64_t> key = selected;
        for (const auto &roots : active_roots)
            key.insert(key.end(), roots.begin(), roots.end());
        key.insert(key.end(), boundary.begin(), boundary.end());
        return key;
    }
};

struct VectorHash {
    std::size_t operator()(const std::vector<int> &key) const {
        std::size_t hash = 1469598103934665603ULL;
        for (const int value : key) {
            hash ^= static_cast<std::size_t>(value);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
};

struct SearchStats {
    std::uint64_t nodes = 0;
    std::uint64_t bound_prunes = 0;
    std::uint64_t cache_hits = 0;
    double skipped_work = 0.0;
    double reported_skipped_work = 0.0;
};

struct ProgressReporter {
    std::atomic<std::uint64_t> nodes{0};
    std::atomic<double> skipped_work{0.0};
    std::uint64_t interval = 10'000'000;
    long double expected_nodes = 0.0L;
    std::vector<double> subtree_nodes;
    std::chrono::steady_clock::time_point started =
        std::chrono::steady_clock::now();

    void add_skipped(const double amount) {
        double current = skipped_work.load(std::memory_order_relaxed);
        while (!skipped_work.compare_exchange_weak(current, current + amount,
                                                   std::memory_order_relaxed,
                                                   std::memory_order_relaxed)) {
        }
    }

    void set_subtree_nodes(const int vertices, const int target) {
        subtree_nodes.assign(target + 1, 1.0);
        for (int depth = target - 1; depth >= 0; --depth) {
            double paths = 1.0;
            double total = 1.0;
            for (int completion_depth = 1; completion_depth <= target - depth;
                 ++completion_depth) {
                paths *= vertices - depth - completion_depth + 1;
                total += paths;
            }
            subtree_nodes[depth] = total;
        }
    }

    void record(const int depth) {
        const std::uint64_t count = nodes.fetch_add(1) + 1;
        if (interval == 0 || count % interval != 0)
            return;
        const double seconds = std::chrono::duration<double>(
                                   std::chrono::steady_clock::now() - started)
                                   .count();
        const double rate = seconds > 0.0 ? count / seconds : 0.0;
        const long double covered =
            static_cast<long double>(count) +
            skipped_work.load(std::memory_order_relaxed);
        const long double percent =
            expected_nodes > 0.0L ? 100.0L * covered / expected_nodes : 0.0L;
#ifdef _OPENMP
#pragma omp critical(profile_dp_progress)
#endif
        std::cerr << "progress nodes=" << count << " depth=" << depth
                  << " rate=" << rate
                  << "/s percent=" << static_cast<double>(percent)
                  << " covered=" << static_cast<double>(covered) << "\n";
    }
};

void search(const Instance &instance, ProfileState &state,
            std::vector<int> &chosen, const int target, int &best,
            std::unordered_set<std::vector<int>, VectorHash> &seen,
            SearchStats &stats, const std::vector<Automorphism> &stabilizer,
            ProgressReporter *progress) {
    if (state.optimistic_bound(target) >= best) {
        ++stats.bound_prunes;
        if (progress != nullptr &&
            (stats.bound_prunes + stats.cache_hits) % 1024 == 0)
            stats.skipped_work +=
                1024.0 * (progress->subtree_nodes[state.selected_count] - 1.0);
        return;
    }
    const std::vector<int> key = canonical_key(chosen, instance, stabilizer);
    if (!seen.insert(key).second) {
        ++stats.cache_hits;
        if (progress != nullptr &&
            (stats.bound_prunes + stats.cache_hits) % 1024 == 0)
            stats.skipped_work +=
                1024.0 * (progress->subtree_nodes[state.selected_count] - 1.0);
        return;
    }
    if (state.selected_count == target) {
        best = std::min(best, state.boundary_size);
        return;
    }
    for (int vertex = 0; vertex < static_cast<int>(instance.vertices.size());
         ++vertex) {
        if (state.selected_at(vertex))
            continue;
        state.add(vertex);
        chosen.push_back(vertex);
        ++stats.nodes;
        if (progress != nullptr)
            progress->record(state.selected_count);
        if (progress != nullptr && stats.nodes % 100'000 == 0) {
            progress->add_skipped(stats.skipped_work -
                                  stats.reported_skipped_work);
            stats.reported_skipped_work = stats.skipped_work;
        }
        search(instance, state, chosen, target, best, seen, stats, stabilizer,
               progress);
        chosen.pop_back();
        state.remove(vertex);
    }
}

int available_threads() {
#ifdef _OPENMP
    return omp_get_num_procs();
#else
    return 1;
#endif
}

void search_parallel(const Instance &instance, const int target, int &best,
                     std::unordered_set<std::vector<int>, VectorHash> &seen,
                     SearchStats &stats,
                     const std::vector<Automorphism> &stabilizer,
                     const int thread_count, ProgressReporter *progress) {
    // Vertex transitivity lets us pin one vertex without changing the
    // optimum, giving independent second-vertex branches to the workers.
    std::vector<int> origin(instance.k);
    std::iota(origin.begin(), origin.end(), 0);
    const int origin_id = instance.index.at(instance.encode(origin));

    if (target == 1) {
        ProfileState state(instance);
        state.add(origin_id);
        std::vector<int> chosen{origin_id};
        search(instance, state, chosen, target, best, seen, stats, stabilizer,
               progress);
        return;
    }

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) num_threads(thread_count)
#endif
    for (int second_vertex = 0;
         second_vertex < static_cast<int>(instance.vertices.size());
         ++second_vertex) {
        if (second_vertex == origin_id)
            continue;
        ProfileState state(instance);
        std::vector<int> chosen{origin_id, second_vertex};
        state.add(origin_id);
        state.add(second_vertex);
        SearchStats local_stats;
        std::unordered_set<std::vector<int>, VectorHash> local_seen;
        int local_best = INT_MAX;
        local_stats.nodes = 1;
        if (progress != nullptr)
            progress->record(2);
        search(instance, state, chosen, target, local_best, local_seen,
               local_stats, stabilizer, progress);
        if (progress != nullptr)
            progress->add_skipped(local_stats.skipped_work -
                                  local_stats.reported_skipped_work);

#ifdef _OPENMP
#pragma omp critical(profile_dp_merge)
#endif
        {
            best = std::min(best, local_best);
            stats.nodes += local_stats.nodes;
            stats.bound_prunes += local_stats.bound_prunes;
            stats.cache_hits += local_stats.cache_hits;
            seen.insert(local_seen.begin(), local_seen.end());
        }
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0]
                  << " [n k target] [--threads=N] [--progress=N]\n";
        return 0;
    }
    const int n = argc > 1 ? std::stoi(argv[1]) : 4;
    const int k = argc > 2 ? std::stoi(argv[2]) : 2;
    const int target = argc > 3 ? std::stoi(argv[3]) : 5;
    int thread_count = available_threads();
    std::uint64_t progress_interval = 10'000'000;
    for (int argument = 4; argument < argc; ++argument) {
        const std::string option(argv[argument]);
        if (option.rfind("--threads=", 0) == 0)
            thread_count = std::stoi(option.substr(10));
        else if (option.rfind("--progress=", 0) == 0)
            progress_interval = std::stoull(option.substr(11));
        else {
            std::cerr << "unknown option: " << option << '\n';
            return 2;
        }
    }
    if (thread_count < 1) {
        std::cerr << "threads must be positive\n";
        return 2;
    }
    const Instance instance(n, k);
    if (target < 1 || target > static_cast<int>(instance.vertices.size())) {
        std::cerr << "invalid target\n";
        return 2;
    }
    const auto stabilizer = origin_stabilizer(instance);
    ProgressReporter progress;
    progress.interval = progress_interval;
    const cpp_int expected_nodes = raw_tree_nodes(
        static_cast<int>(instance.vertices.size()), target, thread_count > 1);
    progress.expected_nodes = expected_nodes.convert_to<long double>();
    progress.set_subtree_nodes(static_cast<int>(instance.vertices.size()),
                               target);
    std::cout << "raw ordered-transition nodes=" << expected_nodes << " ("
              << (thread_count > 1 ? "origin-pinned" : "full") << ")\n";
    std::unordered_set<std::vector<int>, VectorHash> seen;
    SearchStats stats;
    int best = INT_MAX;
    if (thread_count == 1) {
        ProfileState state(instance);
        std::vector<int> chosen;
        search(instance, state, chosen, target, best, seen, stats, stabilizer,
               &progress);
    } else {
        search_parallel(instance, target, best, seen, stats, stabilizer,
                        thread_count, &progress);
    }
    const long double raw_nodes = expected_nodes.convert_to<long double>();
    const long double coverage = 100.0L * stats.nodes / raw_nodes;
    const long double reduction = 100.0L * (1.0L - stats.nodes / raw_nodes);
    std::cout << '\n' << std::fixed << std::setprecision(12);
    std::cout << "A(" << n << ',' << k << ") R=" << target << " best=" << best
              << " nodes=" << stats.nodes
              << " bound-prunes=" << stats.bound_prunes
              << " exact-cache-hits=" << stats.cache_hits
              << " threads=" << thread_count << " states=" << seen.size()
              << '\n'
              << "raw ordered-transition coverage=  " << coverage << "%\n"
              << "raw ordered-transition reduction=" << reduction << "%\n";
    return 0;
}
