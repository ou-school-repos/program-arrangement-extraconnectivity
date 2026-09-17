// Exact unified-envelope transfer prototype.
// Usage: ./transfer_dp_window [n] [k] [max_R] [max_states] [--no-canonical]
// max_states is optional; omitted or zero means unlimited.

#include "arrangement_core.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

using arrangement::Automorphism;
using arrangement::Instance;

namespace {

struct EnvelopeVertex {
    int vertex;
    std::uint8_t incidence;
    bool selected;
    bool boundary;

    bool operator==(const EnvelopeVertex &other) const {
        return vertex == other.vertex && incidence == other.incidence &&
               selected == other.selected && boundary == other.boundary;
    }
    bool operator<(const EnvelopeVertex &other) const {
        if (vertex != other.vertex)
            return vertex < other.vertex;
        if (incidence != other.incidence)
            return incidence < other.incidence;
        if (selected != other.selected)
            return selected < other.selected;
        return boundary < other.boundary;
    }
};

struct WindowState {
    int volume = 0;
    int finalized_boundary = 0;
    std::vector<EnvelopeVertex> envelope;

    bool operator==(const WindowState &other) const {
        return volume == other.volume &&
               finalized_boundary == other.finalized_boundary &&
               envelope == other.envelope;
    }
};

struct StateHash {
    std::size_t operator()(const WindowState &state) const {
        std::size_t hash = static_cast<std::size_t>(state.volume);
        hash = hash * 1315423911U +
               static_cast<std::size_t>(state.finalized_boundary);
        for (const auto &entry : state.envelope) {
            hash = hash * 1315423911U + static_cast<std::size_t>(entry.vertex);
            hash = hash * 1315423911U + entry.incidence;
            hash = hash * 1315423911U + entry.selected;
            hash = hash * 1315423911U + entry.boundary;
        }
        return hash;
    }
};

std::vector<Automorphism> full_automorphisms(const Instance &instance) {
    std::vector<Automorphism> group;
    std::vector<int> coordinates(instance.k);
    std::iota(coordinates.begin(), coordinates.end(), 0);
    std::vector<int> symbols(instance.n);
    std::iota(symbols.begin(), symbols.end(), 0);
    do {
        do {
            group.push_back({coordinates, symbols});
        } while (std::next_permutation(symbols.begin(), symbols.end()));
        std::iota(symbols.begin(), symbols.end(), 0);
    } while (std::next_permutation(coordinates.begin(), coordinates.end()));
    return group;
}

std::vector<Automorphism> context_automorphisms(
    const Instance &instance, const std::vector<std::vector<int>> &fibers,
    const std::vector<int> &order, const std::size_t next_step,
    const std::vector<Automorphism> &full_group) {
    std::map<std::vector<int>, int> fiber_ids;
    for (int fiber = 0; fiber < static_cast<int>(fibers.size()); ++fiber)
        fiber_ids[fibers[fiber]] = fiber;
    std::vector<bool> processed(fibers.size(), false);
    for (std::size_t i = 0; i < next_step; ++i)
        processed[order[i]] = true;

    std::vector<Automorphism> result;
    for (const auto &automorphism : full_group) {
        bool valid = true;
        for (std::size_t i = 0; i < next_step && valid; ++i) {
            std::vector<int> mapped(fibers[order[i]].size());
            std::transform(fibers[order[i]].begin(), fibers[order[i]].end(),
                           mapped.begin(), [&](const int vertex) {
                               return automorphism.apply(vertex, instance);
                           });
            std::sort(mapped.begin(), mapped.end());
            const auto found = fiber_ids.find(mapped);
            valid = found != fiber_ids.end() && processed[found->second];
        }
        if (valid && next_step < order.size()) {
            std::vector<int> mapped(fibers[order[next_step]].size());
            std::transform(fibers[order[next_step]].begin(),
                           fibers[order[next_step]].end(), mapped.begin(),
                           [&](const int vertex) {
                               return automorphism.apply(vertex, instance);
                           });
            std::sort(mapped.begin(), mapped.end());
            valid = mapped == fibers[order[next_step]];
        }
        if (valid)
            result.push_back(automorphism);
    }
    return result;
}

WindowState canonicalize(const WindowState &state, const Instance &instance,
                         const std::vector<Automorphism> &context_group) {
    if (context_group.size() <= 1 || state.envelope.empty())
        return state;
    WindowState best = state;
    bool initialized = false;
    for (const auto &automorphism : context_group) {
        WindowState candidate = state;
        for (auto &entry : candidate.envelope)
            entry.vertex = automorphism.apply(entry.vertex, instance);
        std::sort(candidate.envelope.begin(), candidate.envelope.end());
        if (!initialized || candidate.envelope < best.envelope) {
            best = std::move(candidate);
            initialized = true;
        }
    }
    return best;
}

std::vector<int> greedy_fiber_order(const Instance &instance,
                                    std::vector<std::vector<int>> &fibers) {
    const int fiber_count = std::accumulate(
        instance.lines.begin(), instance.lines.end(), 0,
        [](const int total, const std::vector<std::vector<int>> &lines) {
            return total + static_cast<int>(lines.size());
        });
    fibers.assign(fiber_count, {});

    int offset = 0;
    for (int coordinate = 0; coordinate < instance.k; ++coordinate) {
        for (std::size_t root = 0; root < instance.lines[coordinate].size();
             ++root) {
            for (const int vertex : instance.lines[coordinate][root])
                fibers[offset + static_cast<int>(root)].push_back(vertex);
        }
        offset += static_cast<int>(instance.lines[coordinate].size());
    }

    std::vector<bool> processed(fiber_count, false);
    std::vector<int> incidence(instance.vertices.size(), 0);
    std::vector<int> order;
    order.reserve(fiber_count);
    int frontier = 0;

    for (int step = 0; step < fiber_count; ++step) {
        int best_fiber = -1;
        int best_width = static_cast<int>(instance.vertices.size()) + 1;
        int best_completed = -1;
        for (int fiber = 0; fiber < fiber_count; ++fiber) {
            if (processed[fiber] || (step == 0 && fiber != 0))
                continue;
            int width = frontier;
            int completed = 0;
            for (const int vertex : fibers[fiber]) {
                if (incidence[vertex] == 0)
                    ++width;
                if (incidence[vertex] == instance.k - 1) {
                    --width;
                    ++completed;
                }
            }
            if (width < best_width ||
                (width == best_width && completed > best_completed) ||
                (width == best_width && completed == best_completed &&
                 fiber < best_fiber)) {
                best_fiber = fiber;
                best_width = width;
                best_completed = completed;
            }
        }
        if (best_fiber < 0)
            return {};
        processed[best_fiber] = true;
        order.push_back(best_fiber);
        for (const int vertex : fibers[best_fiber]) {
            if (incidence[vertex] == 0)
                ++frontier;
            ++incidence[vertex];
            if (incidence[vertex] == instance.k)
                --frontier;
        }
    }
    return order;
}

} // namespace

int main(int argc, char **argv) {
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0]
                  << " [n] [k] [max_R] [max_states] [--no-canonical]\n"
                     "max_states is optional; zero means unlimited.\n";
        return 0;
    }
    int n = 5;
    int k = 3;
    int max_volume_arg = -1;
    std::size_t max_states = 0;
    bool use_canonicalization = true;
    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--no-canonical") {
            use_canonicalization = false;
        } else if (argument.rfind("--", 0) == 0) {
            std::cerr << "unknown option: " << argument << '\n';
            return 2;
        } else if (positional == 0) {
            n = std::stoi(argument);
            ++positional;
        } else if (positional == 1) {
            k = std::stoi(argument);
            ++positional;
        } else if (positional == 2) {
            max_volume_arg = std::stoi(argument);
            ++positional;
        } else if (positional == 3) {
            max_states = std::stoull(argument);
            ++positional;
        } else {
            std::cerr << "too many positional arguments\n";
            return 2;
        }
    }
    const Instance instance(n, k);
    const int max_volume = max_volume_arg >= 0
                               ? max_volume_arg
                               : static_cast<int>(instance.vertices.size());

    std::vector<std::vector<int>> fibers;
    const auto order = greedy_fiber_order(instance, fibers);
    const auto full_group = use_canonicalization ? full_automorphisms(instance)
                                                 : std::vector<Automorphism>{};
    std::vector<std::unordered_map<WindowState, int, StateHash>> current(
        max_volume + 1);
    current[0][WindowState{}] = 0;

    std::cout << "A(" << n << ',' << k << ") unified-envelope window DP\n"
              << "fibers=" << order.size() << " max_R=" << max_volume
              << " max_states="
              << (max_states == 0 ? "unlimited" : std::to_string(max_states))
              << "\n";

    for (std::size_t step = 0; step < order.size(); ++step) {
        const auto &fiber = fibers[order[step]];
        const auto context_group =
            use_canonicalization
                ? context_automorphisms(instance, fibers, order, step + 1,
                                        full_group)
                : std::vector<Automorphism>{};
        std::vector<std::unordered_map<WindowState, int, StateHash>> next(
            max_volume + 1);
        std::size_t transitions = 0;

        for (int volume = 0; volume <= max_volume; ++volume) {
            for (const auto &[state, ignored] : current[volume]) {
                (void)ignored;
                std::vector<int> new_vertices;
                for (const int vertex : fiber) {
                    const auto found = std::find_if(
                        state.envelope.begin(), state.envelope.end(),
                        [vertex](const EnvelopeVertex &entry) {
                            return entry.vertex == vertex;
                        });
                    if (found == state.envelope.end() || found->incidence == 0)
                        new_vertices.push_back(vertex);
                }
                const std::uint64_t patterns = std::uint64_t{1}
                                               << new_vertices.size();
                for (std::uint64_t mask = 0; mask < patterns; ++mask) {
                    ++transitions;
                    const int added = __builtin_popcountll(mask);
                    if (volume + added > max_volume)
                        continue;

                    std::vector<std::uint8_t> incidence(
                        instance.vertices.size());
                    std::vector<bool> selected(instance.vertices.size(), false);
                    std::vector<bool> boundary(instance.vertices.size(), false);
                    std::vector<int> active;
                    for (const auto &entry : state.envelope) {
                        incidence[entry.vertex] = entry.incidence;
                        selected[entry.vertex] = entry.selected;
                        boundary[entry.vertex] = entry.boundary;
                        active.push_back(entry.vertex);
                    }
                    for (std::size_t j = 0; j < new_vertices.size(); ++j) {
                        const int vertex = new_vertices[j];
                        if (std::find(active.begin(), active.end(), vertex) ==
                            active.end())
                            active.push_back(vertex);
                        if ((mask >> j) & 1U) {
                            selected[vertex] = true;
                            boundary[vertex] = false;
                        }
                    }

                    for (std::size_t j = 0; j < new_vertices.size(); ++j) {
                        if (!((mask >> j) & 1U))
                            continue;
                        const int selected_vertex = new_vertices[j];
                        for (int coordinate = 0; coordinate < instance.k;
                             ++coordinate) {
                            const int root =
                                instance.root_id[coordinate][selected_vertex];
                            for (const int neighbor :
                                 instance.lines[coordinate][root]) {
                                if (neighbor == selected_vertex)
                                    continue;
                                boundary[neighbor] = true;
                                if (std::find(active.begin(), active.end(),
                                              neighbor) == active.end())
                                    active.push_back(neighbor);
                            }
                        }
                    }

                    for (const int vertex : fiber)
                        ++incidence[vertex];

                    int finalized = state.finalized_boundary;
                    std::vector<EnvelopeVertex> envelope;
                    for (const int vertex : active) {
                        if (incidence[vertex] == k) {
                            if (boundary[vertex] && !selected[vertex])
                                ++finalized;
                        } else {
                            envelope.push_back({vertex, incidence[vertex],
                                                selected[vertex],
                                                boundary[vertex]});
                        }
                    }
                    std::sort(envelope.begin(), envelope.end());
                    WindowState candidate{volume + added, finalized,
                                          std::move(envelope)};
                    candidate =
                        canonicalize(candidate, instance, context_group);
                    const int candidate_volume = candidate.volume;
                    next[candidate_volume].emplace(std::move(candidate),
                                                   finalized);
                    if (max_states != 0 &&
                        next[candidate_volume].size() > max_states)
                        break;
                }
                if (max_states != 0 && next[volume].size() > max_states)
                    break;
            }
        }
        current = std::move(next);
        const std::size_t state_count = std::accumulate(
            current.begin(), current.end(), std::size_t{0},
            [](const std::size_t total,
               const std::unordered_map<WindowState, int, StateHash> &layer) {
                return total + layer.size();
            });
        std::cout << "step=" << step + 1 << '/' << order.size()
                  << " fiber=" << order[step] << " transitions=" << transitions
                  << " context-group=" << context_group.size()
                  << " states=" << state_count << '\n';
        if (max_states != 0 && state_count > max_states) {
            std::cout << "state-limit-reached\n";
            return 3;
        }
    }

    std::cout << "profile:\n";
    for (int volume = 0; volume <= max_volume; ++volume) {
        int best = static_cast<int>(instance.vertices.size()) + 1;
        for (const auto &[state, ignored] : current[volume]) {
            (void)ignored;
            if (state.envelope.empty())
                best = std::min(best, state.finalized_boundary);
        }
        std::cout << "R=" << volume << " best="
                  << (best == static_cast<int>(instance.vertices.size()) + 1
                          ? -1
                          : best)
                  << '\n';
    }
    return 0;
}
