// Exact connected vertex-boundary profile for small arrangement graphs.
//
// This is deliberately a reference tool, not a large-instance solver.  It
// enumerates every connected set of each requested size containing one fixed
// root.  Vertex transitivity makes that restriction complete for Phi_conn.
// The level-by-level deduplication is slower than a tuned ESU implementation,
// but is easy to audit and is useful as an oracle for future optimizations.
//
// Usage: exact_profile n k max_size [max_states_per_level]

#include "arrangement_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Code = packed_code_t;

struct CodeHash {
    std::size_t operator()(const Code value) const noexcept {
        const std::uint64_t lo = static_cast<std::uint64_t>(value);
        const std::uint64_t hi = static_cast<std::uint64_t>(value >> 64);
        std::uint64_t x =
            lo ^ (hi + 0x9e3779b97f4a7c15ULL + (lo << 6) + (lo >> 2));
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return static_cast<std::size_t>(x);
    }
};

struct SetKey {
    std::vector<std::uint64_t> words;

    bool operator==(const SetKey &other) const { return words == other.words; }
};

struct SetHash {
    std::size_t operator()(const SetKey &key) const noexcept {
        std::uint64_t h = 0x243f6a8885a308d3ULL;
        for (const std::uint64_t word : key.words) {
            h ^= word + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            h ^= h >> 30;
            h *= 0xbf58476d1ce4e5b9ULL;
            h ^= h >> 27;
        }
        return static_cast<std::size_t>(h ^ (h >> 31));
    }
};

struct SetState {
    std::vector<std::uint32_t> members;
    std::vector<std::uint32_t> frontier;
};

SetKey make_key(const std::vector<std::uint32_t> &members,
                const std::size_t word_count) {
    SetKey key{std::vector<std::uint64_t>(word_count, 0)};
    for (const std::uint32_t member : members)
        key.words[member >> 6] |= std::uint64_t{1} << (member & 63U);
    return key;
}

bool contains_sorted(const std::vector<std::uint32_t> &values,
                     const std::uint32_t value) {
    return std::binary_search(values.begin(), values.end(), value);
}

std::vector<std::uint32_t>
compute_frontier(const std::vector<std::uint32_t> &members,
                 const std::vector<std::vector<std::uint32_t>> &adjacency) {
    std::vector<char> inside(adjacency.size(), false);
    for (const std::uint32_t member : members)
        inside[member] = true;
    std::vector<std::uint32_t> frontier;
    for (const std::uint32_t member : members) {
        std::copy_if(
            adjacency[member].begin(), adjacency[member].end(),
            std::back_inserter(frontier),
            [&](const std::uint32_t neighbor) { return !inside[neighbor]; });
    }
    std::sort(frontier.begin(), frontier.end());
    frontier.erase(std::unique(frontier.begin(), frontier.end()),
                   frontier.end());
    return frontier;
}

std::size_t
external_boundary(const std::vector<std::uint32_t> &members,
                  const std::vector<std::vector<std::uint32_t>> &adjacency) {
    std::vector<char> inside(adjacency.size(), false);
    for (const std::uint32_t member : members)
        inside[member] = true;
    std::vector<char> boundary(adjacency.size(), false);
    std::size_t result = 0;
    for (const std::uint32_t member : members) {
        for (const std::uint32_t neighbor : adjacency[member]) {
            if (!inside[neighbor] && !boundary[neighbor]) {
                boundary[neighbor] = true;
                ++result;
            }
        }
    }
    return result;
}

std::string tuple_string(const Code code, const int k) {
    std::string result = "[";
    for (int position = 0; position < k; ++position) {
        if (position != 0)
            result += ',';
        result += std::to_string(
            static_cast<unsigned>((code >> (6 * position)) & 63));
    }
    return result + ']';
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage: exact_profile n k max_size "
                     "[max_states_per_level]\n";
        return 1;
    }

    try {
        const int n = std::stoi(argv[1]);
        const int k = std::stoi(argv[2]);
        const std::size_t max_size = std::stoull(argv[3]);
        const std::size_t state_limit =
            argc == 5 ? std::stoull(argv[4]) : 2'000'000U;
        if (k < 1 || n <= k || n > 12 || k > 8 || max_size < 1 ||
            state_limit == 0)
            throw std::invalid_argument("unsupported exact-profile parameters");

        const PackedArrangementGraph graph(n, k);
        if (graph.valid_count > 50'000)
            throw std::invalid_argument("exact-profile reference guard: graph "
                                        "has more than 50000 vertices");

        std::vector<Code> vertices;
        vertices.reserve(graph.valid_count);
        graph.for_each_valid_code(
            [&](const Code code) { vertices.push_back(code); });
        std::unordered_map<Code, std::uint32_t, CodeHash> index;
        index.reserve(vertices.size() * 2);
        for (std::size_t i = 0; i < vertices.size(); ++i)
            index.emplace(vertices[i], static_cast<std::uint32_t>(i));

        std::vector<std::vector<std::uint32_t>> adjacency(vertices.size());
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            graph.for_each_neighbor(vertices[i], [&](const Code neighbor) {
                adjacency[i].push_back(index.at(neighbor));
            });
        }

        const std::size_t word_count = (vertices.size() + 63U) / 64U;
        SetState root{{0}, compute_frontier({0}, adjacency)};
        std::vector<SetState> level{std::move(root)};

        std::cout << "Exact connected profile for A(" << n << ',' << k << ")\n"
                  << "Vertices: " << vertices.size() << "\n"
                  << "Root-restricted by vertex transitivity: yes\n";

        const std::size_t limit = std::min(max_size, vertices.size());
        for (std::size_t size = 1; size <= limit; ++size) {
            std::size_t best_boundary = std::numeric_limits<std::size_t>::max();
            const SetState *best = nullptr;
            for (const SetState &state : level) {
                const std::size_t boundary =
                    external_boundary(state.members, adjacency);
                if (boundary < best_boundary) {
                    best_boundary = boundary;
                    best = &state;
                }
            }
            std::cout << "s=" << size << " states=" << level.size()
                      << " Phi_conn=" << best_boundary;
            if (best != nullptr) {
                std::cout << " witness=";
                for (std::size_t i = 0; i < best->members.size(); ++i) {
                    if (i != 0)
                        std::cout << ',';
                    std::cout << tuple_string(vertices[best->members[i]], k);
                }
            }
            std::cout << '\n';

            if (size == limit)
                break;
            std::unordered_set<SetKey, SetHash> seen;
            std::vector<SetState> next;
            for (const SetState &state : level) {
                for (const std::uint32_t candidate : state.frontier) {
                    if (contains_sorted(state.members, candidate))
                        continue;
                    std::vector<std::uint32_t> members = state.members;
                    members.insert(std::lower_bound(members.begin(),
                                                    members.end(), candidate),
                                   candidate);
                    SetKey key = make_key(members, word_count);
                    if (!seen.insert(key).second)
                        continue;
                    std::vector<std::uint32_t> frontier =
                        compute_frontier(members, adjacency);
                    next.push_back({std::move(members), std::move(frontier)});
                    if (next.size() > state_limit)
                        throw std::runtime_error("state limit exceeded; "
                                                 "increase the fifth argument");
                }
            }
            level = std::move(next);
            if (level.empty())
                break;
        }
    } catch (const std::exception &error) {
        std::cerr << "exact_profile: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
