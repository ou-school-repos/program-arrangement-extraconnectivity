#pragma once

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace arrangement {

struct Instance {
    int n = 0;
    int k = 0;
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
            std::unordered_map<int, int> roots;
            for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
                std::vector<int> root;
                root.reserve(k - 1);
                for (int coordinate = 0; coordinate < k; ++coordinate)
                    if (coordinate != position)
                        root.push_back(vertices[id][coordinate]);
                const int code = encode(root);
                auto [entry, inserted] =
                    roots.emplace(code, static_cast<int>(roots.size()));
                if (inserted)
                    lines[position].emplace_back();
                root_id[position][id] = entry->second;
                lines[position][entry->second].push_back(id);
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

inline std::vector<Automorphism> origin_stabilizer(const Instance &instance) {
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

inline std::vector<int>
canonical_key(const std::vector<int> &subset, const Instance &instance,
              const std::vector<Automorphism> &stabilizer) {
    if (subset.size() == 2) {
        int matching = 0;
        for (int position = 0; position < instance.k; ++position)
            matching += instance.vertices[subset[0]][position] ==
                                instance.vertices[subset[1]][position]
                            ? 1
                            : 0;
        return {-2, matching};
    }
    if (subset.size() == 1)
        return {-1};

    std::vector<int> best;
    bool initialized = false;
    for (const int anchor : subset) {
        std::vector<int> shift(instance.n);
        std::vector<bool> used(instance.n, false);
        for (int position = 0; position < instance.k; ++position) {
            shift[instance.vertices[anchor][position]] = position;
            used[instance.vertices[anchor][position]] = true;
        }
        int next_symbol = instance.k;
        for (int symbol = 0; symbol < instance.n; ++symbol)
            if (!used[symbol])
                shift[symbol] = next_symbol++;

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

inline std::uint64_t bit_mask(const int bit) {
    return std::uint64_t{1} << (bit % 64);
}

inline int word_index(const int bit) { return bit / 64; }

inline bool is_isomorphic(const std::vector<int> &subset,
                          const std::vector<int> &target,
                          const Instance &instance,
                          const std::vector<Automorphism> &stabilizer) {
    if (target.size() == 1 && target[0] == -1)
        return subset.size() == 1;
    if (target.size() == 2 && target[0] == -2 && subset.size() == 2) {
        int matching = 0;
        for (int position = 0; position < instance.k; ++position)
            matching += instance.vertices[subset[0]][position] ==
                                instance.vertices[subset[1]][position]
                            ? 1
                            : 0;
        return target[1] == matching;
    }
    return std::any_of(subset.begin(), subset.end(), [&](const int anchor) {
        std::vector<int> shift(instance.n);
        std::vector<bool> used(instance.n, false);
        for (int position = 0; position < instance.k; ++position) {
            shift[instance.vertices[anchor][position]] = position;
            used[instance.vertices[anchor][position]] = true;
        }
        int next_symbol = instance.k;
        for (int symbol = 0; symbol < instance.n; ++symbol)
            if (!used[symbol])
                shift[symbol] = next_symbol++;
        std::vector<int> shifted;
        shifted.reserve(subset.size());
        for (const int vertex : subset) {
            int code = 0;
            for (int position = 0; position < instance.k; ++position)
                code = instance.n * code +
                       shift[instance.vertices[vertex][position]];
            shifted.push_back(instance.index.at(code));
        }
        return std::any_of(stabilizer.begin(), stabilizer.end(),
                           [&](const Automorphism &automorphism) {
                               std::vector<int> mapped;
                               mapped.reserve(shifted.size());
                               std::transform(shifted.begin(), shifted.end(),
                                              std::back_inserter(mapped),
                                              [&](const int vertex) {
                                                  return automorphism.apply(
                                                      vertex, instance);
                                              });
                               std::sort(mapped.begin(), mapped.end());
                               return mapped == target;
                           });
    });
}

} // namespace arrangement
