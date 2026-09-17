// Exact fiber-profile DP prototype for arrangement-graph isoperimetry.
// This draft deliberately caches exact profiles; symmetry canonicalization is
// a separate layer and must be validated before it is enabled.

#include <algorithm>
#include <climits>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct Instance {
    int n;
    int k;
    std::vector<std::vector<int>> vertices;
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
    std::size_t operator()(const std::vector<std::uint64_t> &key) const {
        std::size_t hash = 1469598103934665603ULL;
        for (const std::uint64_t word : key) {
            hash ^= static_cast<std::size_t>(word);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
};

struct SearchStats {
    std::uint64_t nodes = 0;
    std::uint64_t bound_prunes = 0;
    std::uint64_t cache_hits = 0;
};

void search(const Instance &instance, ProfileState &state,
            std::vector<int> &chosen, const int target, int &best,
            std::unordered_set<std::vector<std::uint64_t>, VectorHash> &seen,
            SearchStats &stats) {
    if (state.optimistic_bound(target) >= best) {
        ++stats.bound_prunes;
        return;
    }
    const std::vector<std::uint64_t> key = state.exact_key();
    if (!seen.insert(key).second) {
        ++stats.cache_hits;
        return;
    }
    if (state.selected_count == target) {
        best = std::min(best, state.boundary_size);
        return;
    }
    const int start = chosen.empty() ? 0 : chosen.back() + 1;
    for (int vertex = start;
         vertex < static_cast<int>(instance.vertices.size()); ++vertex) {
        state.add(vertex);
        chosen.push_back(vertex);
        ++stats.nodes;
        search(instance, state, chosen, target, best, seen, stats);
        chosen.pop_back();
        state.remove(vertex);
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0] << " [n k target]\n";
        return 0;
    }
    const int n = argc > 1 ? std::stoi(argv[1]) : 4;
    const int k = argc > 2 ? std::stoi(argv[2]) : 2;
    const int target = argc > 3 ? std::stoi(argv[3]) : 5;
    const Instance instance(n, k);
    if (target < 1 || target > static_cast<int>(instance.vertices.size())) {
        std::cerr << "invalid target\n";
        return 2;
    }
    ProfileState state(instance);
    std::vector<int> chosen;
    std::unordered_set<std::vector<std::uint64_t>, VectorHash> seen;
    SearchStats stats;
    int best = INT_MAX;
    search(instance, state, chosen, target, best, seen, stats);
    std::cout << "A(" << n << ',' << k << ") R=" << target << " best=" << best
              << " nodes=" << stats.nodes
              << " bound-prunes=" << stats.bound_prunes
              << " exact-cache-hits=" << stats.cache_hits
              << " states=" << seen.size() << '\n';
    return 0;
}
