// Connected boundary-aware simulated annealing for A(n,k).
//
// Default: A(10,5), R=26.  The binary Hamming slice starts at boundary 298.
// Usage: ./simulated_annealing_hunt [steps] [restarts] [seed]

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int N = 10;
constexpr int K = 5;
constexpr int R = 26;

uint64_t pack(const std::vector<int> &vertex) {
    uint64_t result = 0;
    for (int p = 0; p < K; ++p)
        result |= static_cast<uint64_t>(vertex[p]) << ((K - 1 - p) * 5);
    return result;
}

std::string render(uint64_t vertex) {
    std::string result;
    for (int p = 0; p < K; ++p) {
        const int symbol = (vertex >> ((K - 1 - p) * 5)) & 0x1F;
        result += static_cast<char>('0' + symbol);
    }
    return result;
}

void enumerate_vertices(std::vector<uint64_t> &vertices,
                        std::vector<int> &prefix, std::vector<bool> &used) {
    if (static_cast<int>(prefix.size()) == K) {
        vertices.push_back(pack(prefix));
        return;
    }
    for (int symbol = 0; symbol < N; ++symbol) {
        if (used[symbol])
            continue;
        used[symbol] = true;
        prefix.push_back(symbol);
        enumerate_vertices(vertices, prefix, used);
        prefix.pop_back();
        used[symbol] = false;
    }
}

std::vector<uint64_t> hamming_ball() {
    std::vector<uint64_t> result;
    for (int index = 0; index < R; ++index) {
        std::vector<int> vertex{0, 1, 2, 3, 4};
        for (int bit = 0; bit < 5; ++bit)
            if (index & (1 << bit))
                vertex[bit] = K + bit;
        result.push_back(pack(vertex));
    }
    return result;
}

struct State {
    explicit State(const std::vector<int> &initial,
                   const std::vector<std::vector<int>> &adjacency)
        : members(adjacency.size(), false), counts(adjacency.size(), 0),
          boundary(adjacency.size(), false), member_pos(adjacency.size(), -1),
          boundary_pos(adjacency.size(), -1), adjacency(adjacency) {
        for (int vertex : initial)
            members[vertex] = true;
        member_list = initial;
        for (int i = 0; i < static_cast<int>(member_list.size()); ++i)
            member_pos[member_list[i]] = i;
        for (int vertex : initial)
            for (int neighbor : adjacency[vertex])
                ++counts[neighbor];
        for (int vertex = 0; vertex < static_cast<int>(adjacency.size());
             ++vertex)
            refresh_boundary(vertex);
        score = static_cast<int>(boundary_list.size());
    }

    int boundary_size() const { return static_cast<int>(boundary_list.size()); }

    int swap_vertices(int outgoing, int incoming) {
        members[outgoing] = false;
        remove_member(outgoing);
        for (int neighbor : adjacency[outgoing])
            --counts[neighbor];
        members[incoming] = true;
        member_pos[incoming] = static_cast<int>(member_list.size());
        member_list.push_back(incoming);
        for (int neighbor : adjacency[incoming])
            ++counts[neighbor];
        std::vector<int> changed = adjacency[outgoing];
        changed.insert(changed.end(), adjacency[incoming].begin(),
                       adjacency[incoming].end());
        changed.push_back(outgoing);
        changed.push_back(incoming);
        std::sort(changed.begin(), changed.end());
        changed.erase(std::unique(changed.begin(), changed.end()),
                      changed.end());
        for (int vertex : changed)
            refresh_boundary(vertex);
        score = boundary_size();
        return score;
    }

    void remove_member(int vertex) {
        const int position = member_pos[vertex];
        const int replacement = member_list.back();
        member_list[position] = replacement;
        member_pos[replacement] = position;
        member_list.pop_back();
        member_pos[vertex] = -1;
    }

    void refresh_boundary(int vertex) {
        const bool wanted = !members[vertex] && counts[vertex] > 0;
        if (wanted == boundary[vertex])
            return;
        boundary[vertex] = wanted;
        if (wanted) {
            boundary_pos[vertex] = static_cast<int>(boundary_list.size());
            boundary_list.push_back(vertex);
            return;
        }
        const int position = boundary_pos[vertex];
        const int replacement = boundary_list.back();
        boundary_list[position] = replacement;
        boundary_pos[replacement] = position;
        boundary_list.pop_back();
        boundary_pos[vertex] = -1;
    }

    std::vector<bool> members;
    std::vector<int> counts;
    std::vector<bool> boundary;
    std::vector<int> member_list;
    std::vector<int> boundary_list;
    std::vector<int> member_pos;
    std::vector<int> boundary_pos;
    std::vector<std::vector<int>> adjacency;
    int score = 0;
};

bool connected(const State &state, int start) {
    std::vector<bool> seen(state.members.size(), false);
    std::vector<int> stack{start};
    seen[start] = true;
    int found = 0;
    while (!stack.empty()) {
        const int vertex = stack.back();
        stack.pop_back();
        ++found;
        for (int neighbor : state.adjacency[vertex])
            if (state.members[neighbor] && !seen[neighbor]) {
                seen[neighbor] = true;
                stack.push_back(neighbor);
            }
    }
    return found == R;
}

int exact_boundary(const State &state) {
    std::vector<bool> external(state.members.size(), false);
    for (int vertex = 0; vertex < static_cast<int>(state.members.size());
         ++vertex)
        if (state.members[vertex])
            for (int neighbor : state.adjacency[vertex])
                if (!state.members[neighbor])
                    external[neighbor] = true;
    return static_cast<int>(std::count(external.begin(), external.end(), true));
}

} // namespace

int main(int argc, char **argv) {
    const int steps = argc > 1 ? std::stoi(argv[1]) : 1'000'000;
    const int restarts = argc > 2 ? std::stoi(argv[2]) : 8;
    const uint64_t seed_value = argc > 3 ? std::stoull(argv[3]) : 20260916;

    std::vector<uint64_t> vertices;
    std::vector<int> prefix;
    std::vector<bool> used(N, false);
    enumerate_vertices(vertices, prefix, used);
    std::unordered_map<uint64_t, int> index;
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i)
        index[vertices[i]] = i;

    std::vector<std::vector<int>> adjacency(vertices.size());
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        for (int p = 0; p < K; ++p) {
            const uint64_t mask = UINT64_C(0x1F) << ((K - 1 - p) * 5);
            for (int symbol = 0; symbol < N; ++symbol) {
                bool present = false;
                for (int q = 0; q < K; ++q)
                    present |=
                        static_cast<int>((vertices[i] >> ((K - 1 - q) * 5)) &
                                         0x1F) == symbol;
                if (!present)
                    adjacency[i].push_back(index[(vertices[i] & ~mask) |
                                                 (static_cast<uint64_t>(symbol)
                                                  << ((K - 1 - p) * 5))]);
            }
        }
    }

    std::vector<int> seed;
    for (uint64_t vertex : hamming_ball())
        seed.push_back(index[vertex]);
    State initial(seed, adjacency);
    if (initial.score != exact_boundary(initial) || initial.score != 298) {
        std::cerr << "initial boundary verification failed\n";
        return 2;
    }
    State best = initial;
    std::mt19937_64 rng(seed_value);
    const auto started = std::chrono::steady_clock::now();
    std::cout << "A(10,5) R=26: initial boundary=298\n";

    for (int restart = 0; restart < restarts; ++restart) {
        State state = restart == 0 ? initial : best;
        const double temperature_start = 3.0;
        const double temperature_floor = 0.05;
        const double temperature_reheat = 0.5 * temperature_start;
        double temperature = temperature_start;
        int accepted = 0;
        for (int step = 0; step < steps; ++step) {
            std::uniform_int_distribution<int> member_pick(
                0, static_cast<int>(state.member_list.size()) - 1);
            std::uniform_int_distribution<int> boundary_pick(
                0, static_cast<int>(state.boundary_list.size()) - 1);
            const int outgoing = state.member_list[member_pick(rng)];
            const int incoming = state.boundary_list[boundary_pick(rng)];
            const int old_score = state.score;
            state.swap_vertices(outgoing, incoming);
            const int delta = state.score - old_score;
            const bool accept =
                connected(state, incoming) &&
                (delta <= 0 ||
                 std::uniform_real_distribution<double>(0.0, 1.0)(rng) <
                     std::exp(-delta / std::max(temperature, 1e-9)));
            if (accept) {
                ++accepted;
                if (state.score < best.score) {
                    best = state;
                    if (best.score != exact_boundary(best)) {
                        std::cerr
                            << "incremental boundary verification failed\n";
                        return 2;
                    }
                    std::cout << "hit boundary=" << best.score
                              << " restart=" << restart << " step=" << step
                              << "\n";
                    if (best.score < 298)
                        goto done;
                }
            } else {
                state.swap_vertices(incoming, outgoing);
            }
            temperature *= 0.9999;
            if (temperature < temperature_floor)
                temperature = temperature_reheat;
            if ((step + 1) % 1'000'000 == 0) {
                const double elapsed =
                    std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - started)
                        .count();
                std::cout << "progress restart=" << restart + 1 << "/"
                          << restarts << " step=" << step + 1 << "/" << steps
                          << " (" << 100.0 * (step + 1) / steps << "%)"
                          << " best=" << best.score << " accepted=" << accepted
                          << " elapsed=" << elapsed << "s\n";
            }
        }
        std::cout << "restart=" << restart + 1 << "/" << restarts
                  << " best=" << best.score << " accepted=" << accepted << "\n";
    }

done:
    std::cout << "best boundary=" << best.score << " connected="
              << (connected(best, seed.front()) ? "true" : "false")
              << " verified="
              << (exact_boundary(best) == best.score ? "true" : "false")
              << "\nvertices:\n";
    for (int vertex = 0; vertex < static_cast<int>(vertices.size()); ++vertex)
        if (best.members[vertex])
            std::cout << "  " << render(vertices[vertex]) << '\n';
    return 0;
}
