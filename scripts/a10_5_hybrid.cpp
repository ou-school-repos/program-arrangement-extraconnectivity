// Bounded C++ orbit search with an incremental Z3 completion oracle.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <z3++.h>

constexpr int kAlphabet = 10;
constexpr int kDimension = 5;
constexpr int kVolume = 26;
constexpr int kBoundaryCeiling = 297;

using Vertex = std::array<int, kDimension>;

namespace {

int encode(const Vertex &vertex) {
    return std::accumulate(
        vertex.begin(), vertex.end(), 0,
        [](const int code, const int symbol) { return 10 * code + symbol; });
}

struct Instance {
    std::vector<Vertex> vertices;
    std::vector<int> flat_index;
    std::vector<std::vector<std::vector<int>>> lines;
    std::vector<std::vector<int>> root_id;

    Instance() {
        Vertex vertex{};
        std::array<bool, kAlphabet> used{};
        enumerate(0, vertex, used);
        std::cout << "geometry: enumerated " << vertices.size() << " vertices\n"
                  << std::flush;
        flat_index.assign(100000, -1);
        for (int id = 0; id < static_cast<int>(vertices.size()); ++id)
            flat_index[encode(vertices[id])] = id;

        root_id.assign(kDimension, std::vector<int>(vertices.size(), -1));
        lines.resize(kDimension);
        for (int position = 0; position < kDimension; ++position) {
            std::unordered_map<int, int> roots;
            for (int id = 0; id < static_cast<int>(vertices.size()); ++id) {
                int root = 0;
                for (int coordinate = 0; coordinate < kDimension;
                     ++coordinate) {
                    if (coordinate != position)
                        root = 10 * root + vertices[id][coordinate];
                }
                const auto [entry, inserted] =
                    roots.emplace(root, static_cast<int>(roots.size()));
                if (inserted)
                    lines[position].emplace_back();
                root_id[position][id] = entry->second;
                lines[position][entry->second].push_back(id);
            }
            std::cout << "geometry: coordinate " << position
                      << " lines=" << lines[position].size() << '\n'
                      << std::flush;
        }
    }

    void enumerate(int position, Vertex &vertex,
                   std::array<bool, kAlphabet> &used) {
        if (position == kDimension) {
            vertices.push_back(vertex);
            return;
        }
        for (int symbol = 0; symbol < kAlphabet; ++symbol) {
            if (used[symbol])
                continue;
            used[symbol] = true;
            vertex[position] = symbol;
            enumerate(position + 1, vertex, used);
            used[symbol] = false;
        }
    }
};

struct State {
    const Instance &instance;
    std::vector<bool> selected;
    std::vector<int> incidence;
    std::vector<bool> external;
    std::vector<std::vector<int>> root_count;
    int selected_count = 0;
    int boundary_size = 0;
    int total_incidences = 0;

    explicit State(const Instance &graph)
        : instance(graph), selected(graph.vertices.size(), false),
          incidence(graph.vertices.size(), 0),
          external(graph.vertices.size(), false), root_count(kDimension) {
        for (int position = 0; position < kDimension; ++position)
            root_count[position].assign(graph.lines[position].size(), 0);
    }

    void refresh(const int vertex) {
        const bool wanted = !selected[vertex] && incidence[vertex] > 0;
        if (wanted == external[vertex])
            return;
        external[vertex] = wanted;
        boundary_size += wanted ? 1 : -1;
    }

    void add(const int vertex) {
        selected[vertex] = true;
        ++selected_count;
        for (int position = 0; position < kDimension; ++position) {
            const int root = instance.root_id[position][vertex];
            if (root_count[position][root]++ != 0)
                continue;
            ++total_incidences;
            for (const int member : instance.lines[position][root]) {
                ++incidence[member];
                refresh(member);
            }
        }
        refresh(vertex);
        total_incidences = active_roots() * 6 - selected_count * kDimension;
    }

    void remove(const int vertex) {
        selected[vertex] = false;
        --selected_count;
        for (int position = 0; position < kDimension; ++position) {
            const int root = instance.root_id[position][vertex];
            if (--root_count[position][root] != 0)
                continue;
            --total_incidences;
            for (const int member : instance.lines[position][root]) {
                --incidence[member];
                refresh(member);
            }
        }
        refresh(vertex);
        total_incidences = active_roots() * 6 - selected_count * kDimension;
    }

    int active_roots() const {
        int total = 0;
        for (const auto &roots : root_count)
            total += static_cast<int>(
                std::count_if(roots.begin(), roots.end(),
                              [](const int count) { return count > 0; }));
        return total;
    }

    int optimistic_bound() const {
        const int remaining = kVolume - selected_count;
        if (remaining <= 0)
            return boundary_size;
        const int bleed_bound = boundary_size - remaining;
        int incidence_total = 0;
        int incidence_max = 0;
        for (const auto &roots : root_count) {
            const int active = static_cast<int>(
                std::count_if(roots.begin(), roots.end(),
                              [](const int count) { return count > 0; }));
            const int coordinate_bound = std::max(0, active * 6 - kVolume);
            incidence_total += coordinate_bound;
            incidence_max = std::max(incidence_max, coordinate_bound);
        }
        const int aggregate = (incidence_total + kDimension - 1) / kDimension;
        return std::max({bleed_bound, aggregate, incidence_max, 0});
    }
};

struct Automorphism {
    std::array<std::uint8_t, kDimension> coordinates{};
    std::array<std::uint8_t, kAlphabet> symbols{};

    int apply(const int vertex, const Instance &instance) const {
        return instance.flat_index[std::accumulate(
            coordinates.begin(), coordinates.end(), 0,
            [this, &instance, vertex](const int code, const int position) {
                return 10 * code + symbols[instance.vertices[vertex][position]];
            })];
    }
};

std::vector<Automorphism> origin_stabilizer() {
    std::vector<Automorphism> result;
    std::array<int, kDimension> coordinates{0, 1, 2, 3, 4};
    do {
        std::array<int, kAlphabet - kDimension> free_symbols{5, 6, 7, 8, 9};
        do {
            Automorphism automorphism;
            for (int position = 0; position < kDimension; ++position) {
                automorphism.coordinates[position] =
                    static_cast<std::uint8_t>(coordinates[position]);
                automorphism.symbols[coordinates[position]] =
                    static_cast<std::uint8_t>(position);
            }
            for (int index = 0; index < kAlphabet - kDimension; ++index)
                automorphism.symbols[kDimension + index] =
                    static_cast<std::uint8_t>(free_symbols[index]);
            result.push_back(automorphism);
        } while (
            std::next_permutation(free_symbols.begin(), free_symbols.end()));
        if (result.size() % 2880 == 0)
            std::cout << "stabilizer: generated " << result.size()
                      << " elements\n"
                      << std::flush;
    } while (std::next_permutation(coordinates.begin(), coordinates.end()));
    return result;
}

class CompletionOracle {
    z3::context context;
    z3::solver solver;
    std::vector<z3::expr> selected;
    std::vector<z3::expr> boundary;
    std::vector<z3::expr> active_lines;

  public:
    CompletionOracle(const Instance &instance, const unsigned timeout_ms)
        : solver(context) {
        z3::params parameters(context);
        parameters.set("timeout", timeout_ms);
        solver.set(parameters);
        for (int id = 0; id < static_cast<int>(instance.vertices.size());
             ++id) {
            selected.push_back(
                context.bool_const(("hybrid_x_" + std::to_string(id)).c_str()));
            boundary.push_back(
                context.bool_const(("hybrid_y_" + std::to_string(id)).c_str()));
            solver.add(implies(boundary.back(), !selected.back()));
        }
        std::cout << "oracle: created " << selected.size()
                  << " selected and boundary variables\n"
                  << std::flush;
        z3::expr volume = context.int_val(0);
        z3::expr boundary_size = context.int_val(0);
        for (int id = 0; id < static_cast<int>(selected.size()); ++id) {
            volume = volume + z3::ite(selected[id], context.int_val(1),
                                      context.int_val(0));
            boundary_size =
                boundary_size +
                z3::ite(boundary[id], context.int_val(1), context.int_val(0));
        }
        solver.add(volume == kVolume);
        solver.add(boundary_size <= kBoundaryCeiling);

        std::vector<Z3_ast> aggregate_args;
        std::vector<int> aggregate_coefficients;
        for (int position = 0; position < kDimension; ++position) {
            std::cout << "oracle: building coordinate " << position << '\n'
                      << std::flush;
            for (const auto &line : instance.lines[position]) {
                const z3::expr active = context.bool_const(
                    ("hybrid_a_" + std::to_string(active_lines.size()))
                        .c_str());
                active_lines.push_back(active);
                z3::expr occupied = context.bool_val(false);
                z3::expr selected_on_line = context.int_val(0);
                for (const int id : line) {
                    occupied = occupied || selected[id];
                    selected_on_line = selected_on_line +
                                       z3::ite(selected[id], context.int_val(1),
                                               context.int_val(0));
                    solver.add(implies(selected[id], active));
                    solver.add(implies(active && !selected[id], boundary[id]));
                }
                solver.add(implies(active, occupied));
                solver.add(z3::ite(active, context.int_val(1),
                                   context.int_val(0)) <= selected_on_line);
                if (active_lines.size() % 1000 == 0)
                    std::cout
                        << "oracle: line constraints=" << active_lines.size()
                        << '\n'
                        << std::flush;
            }
        }
        std::cout << "oracle: adding aggregate cut over " << active_lines.size()
                  << " lines and " << boundary.size() << " boundary variables\n"
                  << std::flush;
        for (const z3::expr &active : active_lines) {
            aggregate_args.push_back(active);
            aggregate_coefficients.push_back(6);
        }
        for (const z3::expr &external : boundary) {
            aggregate_args.push_back(external);
            aggregate_coefficients.push_back(-5);
        }
        solver.add(z3::expr(
            context,
            Z3_mk_pble(context, static_cast<unsigned>(aggregate_args.size()),
                       aggregate_args.data(), aggregate_coefficients.data(),
                       130)));
        solver.add(selected[0]);
    }

    z3::check_result check(const std::vector<int> &subset,
                           const unsigned timeout_ms) {
        solver.push();
        z3::params parameters(context);
        parameters.set("timeout", timeout_ms);
        solver.set(parameters);
        for (const int vertex : subset)
            solver.add(selected[vertex]);
        const z3::check_result result = solver.check();
        solver.pop();
        return result;
    }
};

struct Metrics {
    std::uint64_t nodes = 0;
    std::uint64_t orbit_skipped = 0;
    std::uint64_t oracle_calls = 0;
    std::uint64_t sat = 0;
    std::uint64_t unsat = 0;
    std::uint64_t unknown = 0;
    bool call_limit = false;
    bool found = false;
};

std::vector<int> orbit_representatives(
    const Instance &instance, const State &state, const int start,
    const std::vector<Automorphism> &stabilizer, Metrics &metrics) {
    std::vector<unsigned char> seen(instance.vertices.size(), 0);
    std::vector<int> representatives;
    for (int vertex = 0; vertex < static_cast<int>(instance.vertices.size());
         ++vertex) {
        if (state.selected[vertex] || seen[vertex])
            continue;
        int orbit_size = 0;
        int representative = vertex;
        for (const Automorphism &automorphism : stabilizer) {
            const int image = automorphism.apply(vertex, instance);
            if (seen[image] == 0) {
                seen[image] = 1;
                ++orbit_size;
            }
            representative = std::min(representative, image);
        }
        metrics.orbit_skipped += static_cast<std::uint64_t>(orbit_size - 1);
        if (representative >= start)
            representatives.push_back(representative);
    }
    std::sort(representatives.begin(), representatives.end());
    return representatives;
}

bool search(const Instance &instance, State &state, std::vector<int> &subset,
            const int crossover_depth, const unsigned timeout_ms,
            const std::uint64_t max_calls,
            const std::vector<Automorphism> &stabilizer,
            CompletionOracle &oracle, Metrics &metrics) {
    if (state.optimistic_bound() > kBoundaryCeiling)
        return false;
    if (state.selected_count == kVolume) {
        if (state.boundary_size <= kBoundaryCeiling) {
            metrics.found = true;
            return true;
        }
        return false;
    }
    if (state.selected_count == crossover_depth) {
        if (max_calls != 0 && metrics.oracle_calls >= max_calls) {
            metrics.call_limit = true;
            return false;
        }
        ++metrics.oracle_calls;
        const z3::check_result result = oracle.check(subset, timeout_ms);
        if (result == z3::sat) {
            ++metrics.sat;
            metrics.found = true;
            return true;
        }
        if (result == z3::unsat) {
            ++metrics.unsat;
            return false;
        }
        ++metrics.unknown;
    }

    const int start = subset.back() + 1;
    const std::vector<int> representatives =
        orbit_representatives(instance, state, start, stabilizer, metrics);
    for (const int vertex : representatives) {
        std::vector<Automorphism> next_stabilizer;
        std::copy_if(stabilizer.begin(), stabilizer.end(),
                     std::back_inserter(next_stabilizer),
                     [&instance, vertex](const Automorphism &automorphism) {
                         return automorphism.apply(vertex, instance) == vertex;
                     });
        state.add(vertex);
        subset.push_back(vertex);
        ++metrics.nodes;
        if (search(instance, state, subset, crossover_depth, timeout_ms,
                   max_calls, next_stabilizer, oracle, metrics))
            return true;
        subset.pop_back();
        state.remove(vertex);
        if (metrics.call_limit)
            return false;
    }
    return false;
}

} // namespace

int main(int argc, char **argv) {
    int crossover_depth = 12;
    unsigned timeout_ms = 5000;
    std::uint64_t max_calls = 1000;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument.rfind("--crossover-depth=", 0) == 0)
            crossover_depth = std::stoi(argument.substr(18));
        else if (argument.rfind("--oracle-timeout-ms=", 0) == 0)
            timeout_ms = static_cast<unsigned>(std::stoul(argument.substr(20)));
        else if (argument.rfind("--max-oracle-calls=", 0) == 0)
            max_calls = std::stoull(argument.substr(19));
        else if (argument == "-h" || argument == "--help") {
            std::cout << "usage: " << argv[0]
                      << " [--crossover-depth=N] [--oracle-timeout-ms=N]"
                         " [--max-oracle-calls=N]\n";
            return 0;
        } else {
            std::cerr << "unknown option: " << argument << '\n';
            return 2;
        }
    }
    if (crossover_depth < 1 || crossover_depth > kVolume) {
        std::cerr << "crossover depth must be in [1,26]\n";
        return 2;
    }

    std::cout << "initializing A(10,5)\n";
    const Instance instance;
    std::cout << "geometry ready: vertices=" << instance.vertices.size()
              << " lines="
              << std::accumulate(
                     instance.lines.begin(), instance.lines.end(), 0U,
                     [](const unsigned total, const auto &position_lines) {
                         return total + position_lines.size();
                     })
              << '\n'
              << std::flush;
    const auto stabilizer = origin_stabilizer();
    std::cout << "stabilizer ready: " << stabilizer.size() << " automorphisms\n"
              << std::flush;
    State state(instance);
    state.add(0);
    std::vector<int> subset{0};
    std::cout << "building Z3 model\n" << std::flush;
    CompletionOracle oracle(instance, timeout_ms);
    std::cout << "Z3 model ready\n" << std::flush;
    Metrics metrics;
    const auto started = std::chrono::steady_clock::now();
    search(instance, state, subset, crossover_depth, timeout_ms, max_calls,
           stabilizer, oracle, metrics);
    const double elapsed = std::chrono::duration<double>(
                               std::chrono::steady_clock::now() - started)
                               .count();
    std::cout << "nodes=" << metrics.nodes
              << " orbit_skipped=" << metrics.orbit_skipped
              << " oracle_calls=" << metrics.oracle_calls
              << " sat=" << metrics.sat << " unsat=" << metrics.unsat
              << " unknown=" << metrics.unknown
              << " call_limit=" << (metrics.call_limit ? "yes" : "no")
              << " found=" << (metrics.found ? "yes" : "no")
              << " elapsed=" << elapsed << "s\n";
    return metrics.found ? 0 : (metrics.call_limit ? 3 : 0);
}
