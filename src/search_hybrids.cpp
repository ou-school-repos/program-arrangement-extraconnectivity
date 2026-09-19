// Local structural search for subsets that may improve on the full Star.
//
// This is an empirical obstruction search, not an optimality proof.  It never
// allocates the full arrangement graph: only the requested subset and the
// candidate vertices adjacent to it are held in memory.
//
// Usage: search_hybrids n k [iterations] [restarts]

#include "arrangement_utils.hpp"
#include "build_info.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Vertex = packed_code_t;

bool contains(const std::vector<Vertex> &vertices, const Vertex value) {
    return std::find(vertices.begin(), vertices.end(), value) != vertices.end();
}

bool adjacent(const Vertex left, const Vertex right, const int k) {
    int differences = 0;
    for (int position = 0; position < k; ++position) {
        if (((left >> (6 * position)) & 63) !=
            ((right >> (6 * position)) & 63)) {
            ++differences;
            if (differences > 1)
                return false;
        }
    }
    return differences == 1;
}

std::uint64_t internal_edges(const std::vector<Vertex> &subset, const int k) {
    std::size_t index = 0;
    return std::accumulate(
        subset.begin(), subset.end(), std::uint64_t{0},
        [&](const std::uint64_t total, const Vertex vertex) {
            const std::uint64_t result =
                total +
                std::accumulate(
                    subset.begin() + static_cast<std::ptrdiff_t>(index + 1),
                    subset.end(), std::uint64_t{0},
                    [&](const std::uint64_t inner_total,
                        const Vertex later_vertex) {
                        return inner_total +
                               (adjacent(vertex, later_vertex, k) ? 1U : 0U);
                    });
            ++index;
            return result;
        });
}

std::uint64_t edges_to_subset(const Vertex vertex,
                              const std::vector<Vertex> &subset, const int k,
                              const std::size_t skipped) {
    std::uint64_t result = 0;
    for (std::size_t i = 0; i < subset.size(); ++i)
        if (i != skipped)
            result += adjacent(vertex, subset[i], k) ? 1U : 0U;
    return result;
}

std::vector<Vertex> full_star(const PackedArrangementGraph &graph) {
    std::vector<int> center(graph.k);
    std::iota(center.begin(), center.end(), 0);
    const Vertex center_code = graph.encode(center);
    std::vector<Vertex> result{center_code};
    graph.for_each_neighbor(center_code, [&](const Vertex neighbor) {
        result.push_back(neighbor);
    });
    return result;
}

// Start with a small core, then add the currently most adjacent candidate.
// This makes edge- and multi-core seeds well-defined at exactly |Star|.
std::vector<Vertex> complete_seed(const PackedArrangementGraph &graph,
                                  std::vector<Vertex> core,
                                  const std::size_t target_size) {
    std::vector<Vertex> candidates;
    auto collect_boundary = [&]() {
        for (const Vertex vertex : core) {
            graph.for_each_neighbor(vertex, [&](const Vertex neighbor) {
                if (!contains(core, neighbor) &&
                    !contains(candidates, neighbor))
                    candidates.push_back(neighbor);
            });
        }
    };

    collect_boundary();
    while (core.size() < target_size) {
        if (candidates.empty())
            collect_boundary();
        if (candidates.size() == 0)
            throw std::runtime_error("could not complete hybrid seed");

        std::size_t best = 0;
        std::uint64_t best_gain = 0;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            const std::uint64_t gain =
                edges_to_subset(candidates[i], core, graph.k,
                                std::numeric_limits<std::size_t>::max());
            if (i == 0 || gain > best_gain) {
                best = i;
                best_gain = gain;
            }
        }
        core.push_back(candidates[best]);
        candidates[best] = candidates.back();
        candidates.pop_back();

        if (candidates.empty() && core.size() < target_size)
            collect_boundary();
    }
    return core;
}

std::vector<Vertex> edge_core(const PackedArrangementGraph &graph,
                              const std::size_t target_size) {
    std::vector<Vertex> star = full_star(graph);
    if (star.size() < 2)
        throw std::runtime_error("graph has no edge core");
    std::vector<Vertex> core{star[0], star[1]};
    return complete_seed(graph, std::move(core), target_size);
}

std::vector<Vertex> multi_core(const PackedArrangementGraph &graph,
                               const std::size_t target_size) {
    const std::vector<Vertex> star = full_star(graph);
    std::vector<Vertex> core{star[0]};
    if (star.size() > 2)
        core.push_back(star[1]);
    if (star.size() > 3)
        core.push_back(star[2]);
    return complete_seed(graph, std::move(core), target_size);
}

bool choose_absorb(const PackedArrangementGraph &graph,
                   const std::vector<Vertex> &subset, const Vertex anchor,
                   std::mt19937 &rng, Vertex &result) {
    std::size_t eligible = 0;
    graph.for_each_neighbor(anchor, [&](const Vertex neighbor) {
        if (!contains(subset, neighbor))
            ++eligible;
    });
    if (eligible == 0)
        return false;

    std::uniform_int_distribution<std::size_t> pick(0, eligible - 1);
    const std::size_t wanted = pick(rng);
    std::size_t seen = 0;
    bool found = false;
    graph.for_each_neighbor(anchor, [&](const Vertex neighbor) {
        if (!found && !contains(subset, neighbor)) {
            if (seen == wanted) {
                result = neighbor;
                found = true;
            }
            ++seen;
        }
    });
    return found;
}

struct VertexHash {
    static std::uint64_t splitmix64(std::uint64_t value) noexcept {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }

    static std::uint64_t rotate_left(std::uint64_t value,
                                     const unsigned amount) noexcept {
        return (value << amount) | (value >> (64U - amount));
    }

    std::size_t operator()(const Vertex value) const noexcept {
        const std::uint64_t low = static_cast<std::uint64_t>(value);
        const std::uint64_t high = static_cast<std::uint64_t>(value >> 64);
        const std::uint64_t mixed_low = splitmix64(low);
        const std::uint64_t mixed_high = splitmix64(high);
        return static_cast<std::size_t>(mixed_low ^
                                        rotate_left(mixed_high, 29));
    }
};

class BoundaryTracker {
  public:
    BoundaryTracker() : keys_(capacity_, Vertex{0}), counts_(capacity_, 0) {}

    void build(const PackedArrangementGraph &graph,
               const std::vector<Vertex> &subset) {
        std::fill(keys_.begin(), keys_.end(), Vertex{0});
        std::fill(counts_.begin(), counts_.end(), Count{0});
        size_ = 0;
        tombstones_ = 0;
        for (const Vertex vertex : subset)
            add_boundary_contributions(graph, subset, vertex);
    }

    std::size_t size() const { return size_; }

    void apply_swap(const PackedArrangementGraph &graph,
                    std::vector<Vertex> &subset, const std::size_t index,
                    const Vertex absorb) {
        const Vertex evict = subset[index];
        remove_member(graph, subset, evict);
        erase_boundary(absorb);
        subset[index] = absorb;
        add_member(graph, subset, absorb);
    }

    void rollback_swap(const PackedArrangementGraph &graph,
                       std::vector<Vertex> &subset, const std::size_t index,
                       const Vertex evict, const Vertex absorb) {
        remove_member(graph, subset, absorb);
        subset[index] = evict;
        add_member(graph, subset, evict);
    }

  private:
    using Count = std::uint16_t;
    static constexpr std::size_t capacity_ = 1U << 16;
    static constexpr std::size_t mask_ = capacity_ - 1;
    static constexpr std::size_t max_entries_ = capacity_ * 3 / 4;
    static constexpr Count tombstone_ = std::numeric_limits<Count>::max();
    std::vector<Vertex> keys_;
    std::vector<Count> counts_;
    std::size_t size_ = 0;
    std::size_t tombstones_ = 0;
    VertexHash hasher_;

    void increment(const Vertex vertex) {
        const std::size_t start = hasher_(vertex) & mask_;
        std::size_t position = start;
        std::size_t first_tombstone = capacity_;
        while (counts_[position] != 0) {
            if (counts_[position] == tombstone_) {
                if (first_tombstone == capacity_)
                    first_tombstone = position;
            } else if (keys_[position] == vertex) {
                counts_[position] =
                    static_cast<Count>(counts_[position] + Count{1});
                return;
            }
            position = (position + 1) & mask_;
            if (position == start)
                throw std::length_error(
                    "hybrid boundary tracker has no empty slot");
        }
        if (first_tombstone != capacity_)
            position = first_tombstone;
        else if (size_ + tombstones_ >= max_entries_) {
            rehash();
            increment(vertex);
            return;
        }
        keys_[position] = vertex;
        counts_[position] = Count{1};
        ++size_;
        if (first_tombstone != capacity_)
            --tombstones_;
    }

    void decrement(const Vertex vertex) {
        const std::size_t start = hasher_(vertex) & mask_;
        std::size_t position = start;
        while (counts_[position] != 0) {
            if (counts_[position] != tombstone_ && keys_[position] == vertex)
                break;
            position = (position + 1) & mask_;
            if (position == start)
                break;
        }
        if (counts_[position] == 0 || counts_[position] == tombstone_)
            throw std::logic_error("boundary incidence underflow");
        if (counts_[position] > Count{1}) {
            counts_[position] =
                static_cast<Count>(counts_[position] - Count{1});
            return;
        }

        counts_[position] = tombstone_;
        keys_[position] = Vertex{0};
        --size_;
        ++tombstones_;
        if (tombstones_ > capacity_ / 4)
            rehash();
    }

    void erase_boundary(const Vertex vertex) {
        const std::size_t start = hasher_(vertex) & mask_;
        std::size_t position = start;
        while (counts_[position] != 0) {
            if (counts_[position] != tombstone_ && keys_[position] == vertex)
                break;
            position = (position + 1) & mask_;
            if (position == start)
                return;
        }
        if (counts_[position] == 0 || counts_[position] == tombstone_)
            return;
        counts_[position] = tombstone_;
        keys_[position] = Vertex{0};
        --size_;
        ++tombstones_;
        if (tombstones_ > capacity_ / 4)
            rehash();
    }

    void rehash() {
        const std::vector<Vertex> old_keys = std::move(keys_);
        const std::vector<Count> old_counts = std::move(counts_);
        keys_.assign(capacity_, Vertex{0});
        counts_.assign(capacity_, Count{0});
        size_ = 0;
        tombstones_ = 0;
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (old_counts[i] == 0 || old_counts[i] == tombstone_)
                continue;
            std::size_t position = hasher_(old_keys[i]) & mask_;
            while (counts_[position] != 0)
                position = (position + 1) & mask_;
            keys_[position] = old_keys[i];
            counts_[position] = old_counts[i];
            ++size_;
        }
    }

    void add_boundary_contributions(const PackedArrangementGraph &graph,
                                    const std::vector<Vertex> &subset,
                                    const Vertex vertex) {
        graph.for_each_neighbor(vertex, [&](const Vertex neighbor) {
            if (!contains(subset, neighbor))
                increment(neighbor);
        });
    }

    void remove_member(const PackedArrangementGraph &graph,
                       const std::vector<Vertex> &subset, const Vertex vertex) {
        graph.for_each_neighbor(vertex, [&](const Vertex neighbor) {
            if (contains(subset, neighbor))
                increment(vertex);
            else
                decrement(neighbor);
        });
    }

    void add_member(const PackedArrangementGraph &graph,
                    const std::vector<Vertex> &subset, const Vertex vertex) {
        erase_boundary(vertex);
        add_boundary_contributions(graph, subset, vertex);
    }
};

void print_vertex(const Vertex code, const int k) {
    std::cout << "  [";
    for (int position = 0; position < k; ++position) {
        if (position != 0)
            std::cout << ',';
        std::cout << static_cast<unsigned>((code >> (6 * position)) & 63);
    }
    std::cout << "]\n";
}

void print_witness(const std::vector<Vertex> &subset, const int k) {
    std::cout << "Witness subset (" << subset.size() << " vertices):\n";
    for (const Vertex vertex : subset)
        print_vertex(vertex, k);
}

std::vector<Vertex> climb(const PackedArrangementGraph &graph,
                          std::vector<Vertex> subset,
                          const std::uint64_t iterations, std::mt19937 &rng,
                          std::size_t &best_boundary) {
    BoundaryTracker tracker;
    tracker.build(graph, subset);
    std::size_t current_boundary = tracker.size();
    best_boundary = current_boundary;
    std::uniform_int_distribution<std::size_t> pick(0, subset.size() - 1);
    constexpr std::uint64_t plateau_limit = 32;
    std::uint64_t plateau_steps = 0;

    for (std::uint64_t iteration = 0; iteration < iterations; ++iteration) {
        const std::size_t evict_index = pick(rng);
        const Vertex evict = subset[evict_index];
        const Vertex anchor = subset[pick(rng)];
        Vertex absorb = 0;
        if (!choose_absorb(graph, subset, anchor, rng, absorb))
            continue;
        tracker.apply_swap(graph, subset, evict_index, absorb);
        const std::size_t candidate_boundary = tracker.size();
        const bool improves = candidate_boundary < current_boundary;
        const bool crosses_plateau = candidate_boundary == current_boundary &&
                                     plateau_steps < plateau_limit;
        if (improves || crosses_plateau) {
            current_boundary = candidate_boundary;
            plateau_steps = improves ? 0 : plateau_steps + 1;
            if (current_boundary < best_boundary) {
                best_boundary = current_boundary;
                std::cout << "  improvement at iteration " << iteration
                          << ": vertex boundary " << best_boundary << '\n';
            }
        } else {
            tracker.rollback_swap(graph, subset, evict_index, evict, absorb);
        }
    }
    return subset;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " n k [iterations] [restarts]\n";
        return 1;
    }

    try {
        const int n = std::stoi(argv[1]);
        const int k = std::stoi(argv[2]);
        const std::uint64_t iterations =
            argc >= 4 ? std::stoull(argv[3]) : 5'000'000U;
        const std::size_t restarts = argc >= 5 ? std::stoull(argv[4]) : 8U;
        if (n <= k || k < 1 || n > 64 || k > 21 || iterations == 0 ||
            restarts == 0)
            throw std::invalid_argument("invalid search parameters");

        const PackedArrangementGraph graph(n, k);
        const std::vector<Vertex> star = full_star(graph);
        const std::size_t target_size = star.size();
        const int degree = k * (n - k);
        const std::uint64_t star_edges = internal_edges(star, k);
        BoundaryTracker star_tracker;
        star_tracker.build(graph, star);
        const std::size_t star_boundary = star_tracker.size();

        std::cout << "Build: " << build_version << '\n'
                  << "Local hybrid search for A(" << n << ',' << k << ")\n"
                  << "Target subset size: " << target_size << '\n'
                  << "Degree: " << degree << '\n'
                  << "Full-Star internal edges: " << star_edges << '\n'
                  << "Full-Star boundary: " << star_boundary << '\n'
                  << "Iterations per restart: " << iterations << '\n'
                  << "Restarts: " << restarts << '\n';

        std::mt19937 rng(0x51A7U);
        std::size_t global_best_boundary = star_boundary;
        std::vector<Vertex> global_best = star;
        for (std::size_t restart = 0; restart < restarts; ++restart) {
            std::vector<Vertex> seed = restart % 2 == 0
                                           ? edge_core(graph, target_size)
                                           : multi_core(graph, target_size);
            BoundaryTracker seed_tracker;
            seed_tracker.build(graph, seed);
            std::cout << "Restart " << (restart + 1) << '/' << restarts
                      << ": initial boundary " << seed_tracker.size() << '\n';
            std::size_t best_boundary = 0;
            std::vector<Vertex> result =
                climb(graph, std::move(seed), iterations, rng, best_boundary);
            if (best_boundary < global_best_boundary) {
                global_best_boundary = best_boundary;
                global_best = std::move(result);
                std::cout << "NEW BEST: vertex boundary "
                          << global_best_boundary << '\n';
            }
        }

        std::cout << "Best observed vertex boundary: " << global_best_boundary
                  << '\n';
        if (global_best_boundary < star_boundary) {
            std::cout << "Potential smaller-boundary structure found; this is "
                         "not a proof of optimality.\n";
            print_witness(global_best, k);
        } else {
            std::cout << "No sampled structure beat the full Star.\n";
        }
    } catch (const std::exception &error) {
        std::cerr << "search_hybrids: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
