// Exhaustive finite-host catalogue of connected arrangement-graph patterns.
//
// For a fixed R, the host A(2R-2,R-1) contains a representative of every
// connected R-vertex pattern: a spanning tree uses at most R-1 coordinates
// and introduces at most R-1 new symbols. The catalogue records (D,X,p,s_a),
// then evaluates the connected-pattern envelope for requested A(n,k) cells.
// This is Phi_conn, not the unrestricted vertex-boundary profile Phi.
//
// Usage: pattern_catalogue R [n k]...

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int max_r = 9;
constexpr int max_positions = 8;
using Word = std::array<std::int8_t, max_positions>;
using Signature =
    std::array<int, 4>; // defect, collisions, active positions, symbols

int R = 0;
int K = 0;
int N = 0;
int vertex_count = 0;
std::vector<Word> words;
std::vector<std::vector<int>> adjacency;
std::vector<bool> in_set;
std::vector<int> set_neighbor_count;
std::vector<std::uint64_t> boundary_stamp;
std::uint64_t current_stamp = 0;
std::array<int, max_r> selected{};
std::set<Signature> signatures;
std::map<Signature, std::vector<int>> examples;
std::uint64_t leaves = 0;

bool parse_int(const char *text, int &value) {
    char *end = nullptr;
    errno = 0;
    const long parsed = std::strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < INT_MIN ||
        parsed > INT_MAX)
        return false;
    value = static_cast<int>(parsed);
    return true;
}

void evaluate_leaf() {
    ++leaves;
    ++current_stamp;
    int external_boundary = 0;
    for (int i = 0; i < R; ++i) {
        for (const int neighbor : adjacency[static_cast<std::size_t>(
                 selected[static_cast<std::size_t>(i)])]) {
            const auto index = static_cast<std::size_t>(neighbor);
            if (!in_set[index] && boundary_stamp[index] != current_stamp) {
                boundary_stamp[index] = current_stamp;
                ++external_boundary;
            }
        }
    }

    int root_sum = 0;
    int active_positions = 0;
    std::set<int> active_symbols;
    for (int coordinate = 0; coordinate < K; ++coordinate) {
        std::set<std::vector<int>> roots;
        std::set<int> coordinate_symbols;
        for (int i = 0; i < R; ++i) {
            const Word &word = words[static_cast<std::size_t>(
                selected[static_cast<std::size_t>(i)])];
            std::vector<int> root;
            for (int position = 0; position < K; ++position) {
                if (position != coordinate)
                    root.push_back(static_cast<int>(
                        word[static_cast<std::size_t>(position)]));
            }
            roots.insert(std::move(root));
            coordinate_symbols.insert(
                static_cast<int>(word[static_cast<std::size_t>(coordinate)]));
        }
        root_sum += static_cast<int>(roots.size());
        if (coordinate_symbols.size() > 1) {
            ++active_positions;
            active_symbols.insert(coordinate_symbols.begin(),
                                  coordinate_symbols.end());
        }
    }

    const int defect = R * K - root_sum;
    const int collisions = root_sum * (N - K + 1) - R * K - external_boundary;
    const Signature signature{defect, collisions, active_positions,
                              static_cast<int>(active_symbols.size())};
    if (signatures.insert(signature).second)
        examples[signature] =
            std::vector<int>(selected.begin(), selected.begin() + R);
}

void enumerate_connected(int size, std::vector<int> extension) {
    if (size == R) {
        evaluate_leaf();
        return;
    }

    while (!extension.empty()) {
        const int vertex = extension.back();
        extension.pop_back();

        std::vector<int> new_extension;
        for (const int neighbor : adjacency[static_cast<std::size_t>(vertex)]) {
            const auto index = static_cast<std::size_t>(neighbor);
            if (neighbor > 0 && !in_set[index] &&
                set_neighbor_count[index] == 0)
                new_extension.push_back(neighbor);
        }

        selected[static_cast<std::size_t>(size)] = vertex;
        in_set[static_cast<std::size_t>(vertex)] = true;
        ++set_neighbor_count[static_cast<std::size_t>(vertex)];
        for (const int neighbor : adjacency[static_cast<std::size_t>(vertex)])
            ++set_neighbor_count[static_cast<std::size_t>(neighbor)];

        std::vector<int> next_extension = extension;
        std::copy_if(new_extension.begin(), new_extension.end(),
                     std::back_inserter(next_extension),
                     [&](const int candidate) {
                         return std::find(extension.begin(), extension.end(),
                                          candidate) == extension.end();
                     });
        enumerate_connected(size + 1, std::move(next_extension));

        in_set[static_cast<std::size_t>(vertex)] = false;
        --set_neighbor_count[static_cast<std::size_t>(vertex)];
        for (const int neighbor : adjacency[static_cast<std::size_t>(vertex)])
            --set_neighbor_count[static_cast<std::size_t>(neighbor)];
    }
}

void build_host() {
    std::vector<int> prefix;
    std::function<void()> generate = [&]() {
        if (static_cast<int>(prefix.size()) == K) {
            Word word{};
            for (int i = 0; i < K; ++i)
                word[static_cast<std::size_t>(i)] = static_cast<std::int8_t>(
                    prefix[static_cast<std::size_t>(i)]);
            words.push_back(word);
            return;
        }
        for (int symbol = 0; symbol < N; ++symbol) {
            if (std::find(prefix.begin(), prefix.end(), symbol) ==
                prefix.end()) {
                prefix.push_back(symbol);
                generate();
                prefix.pop_back();
            }
        }
    };
    generate();

    if (words.size() > static_cast<std::size_t>(INT_MAX))
        throw std::runtime_error(
            "host graph exceeds integer vertex-index capacity");
    vertex_count = static_cast<int>(words.size());
    std::map<Word, int> vertex_id;
    for (int i = 0; i < vertex_count; ++i)
        vertex_id.emplace(words[static_cast<std::size_t>(i)], i);

    adjacency.assign(words.size(), {});
    for (int i = 0; i < vertex_count; ++i) {
        for (int coordinate = 0; coordinate < K; ++coordinate) {
            for (int symbol = 0; symbol < N; ++symbol) {
                bool already_used = false;
                for (int position = 0; position < K; ++position) {
                    already_used |= words[static_cast<std::size_t>(i)]
                                         [static_cast<std::size_t>(position)] ==
                                    symbol;
                }
                if (already_used)
                    continue;
                Word neighbor = words[static_cast<std::size_t>(i)];
                neighbor[static_cast<std::size_t>(coordinate)] =
                    static_cast<std::int8_t>(symbol);
                adjacency[static_cast<std::size_t>(i)].push_back(
                    vertex_id.at(neighbor));
            }
        }
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2 || argc % 2 != 0) {
        std::fprintf(stderr, "Usage: %s R [n k]...\n", argv[0]);
        return 1;
    }
    if (!parse_int(argv[1], R) || R < 1 || R > max_r) {
        std::fprintf(stderr, "R must be in 1..%d (fixed pattern buffers).\n",
                     max_r);
        return 1;
    }

    std::vector<std::pair<int, int>> queries;
    for (int arg = 2; arg < argc; arg += 2) {
        int n = 0;
        int k = 0;
        if (!parse_int(argv[arg], n) || !parse_int(argv[arg + 1], k) || k < 1 ||
            n <= k || n > 100000000) {
            std::fprintf(stderr,
                         "Queries must satisfy 1 <= k < n <= 100000000.\n");
            return 1;
        }
        queries.emplace_back(n, k);
    }

    K = std::max(1, R - 1);
    N = K + R - 1;
    try {
        build_host();
    } catch (const std::exception &error) {
        std::fprintf(stderr, "pattern_catalogue: %s\n", error.what());
        return 1;
    }

    in_set.assign(words.size(), false);
    set_neighbor_count.assign(words.size(), 0);
    boundary_stamp.assign(words.size(), 0);
    selected[0] = 0;
    in_set[0] = true;
    ++set_neighbor_count[0];
    for (const int neighbor : adjacency[0])
        ++set_neighbor_count[static_cast<std::size_t>(neighbor)];
    std::vector<int> initial_extension = adjacency[0];
    if (R == 1)
        evaluate_leaf();
    else
        enumerate_connected(1, std::move(initial_extension));

    std::printf(
        "R=%d host=A(%d,%d) connected sets through root=%llu signatures=%zu\n",
        R, N, K, static_cast<unsigned long long>(leaves), signatures.size());
    std::printf("D X p s_a  example\n");
    for (const Signature &signature : signatures) {
        std::printf("%d %d %d %d  ", signature[0], signature[1], signature[2],
                    signature[3]);
        for (const int index : examples.at(signature)) {
            std::printf("(");
            for (int position = 0; position < K; ++position) {
                if (position != 0)
                    std::printf(",");
                std::printf("%d",
                            static_cast<int>(words[static_cast<std::size_t>(
                                index)][static_cast<std::size_t>(position)]));
            }
            std::printf(")");
        }
        std::printf("\n");
    }

    for (const auto &[n, k] : queries) {
        long long best = std::numeric_limits<long long>::max();
        Signature best_signature{};
        for (const Signature &signature : signatures) {
            const int defect = signature[0];
            const int active_positions = signature[2];
            const int active_symbols = signature[3];
            if (active_positions > k ||
                active_symbols - active_positions > n - k)
                continue;
            const long long boundary =
                static_cast<long long>(R * k - defect) * (n - k) - defect -
                signature[1];
            if (boundary < best) {
                best = boundary;
                best_signature = signature;
            }
        }
        if (best == std::numeric_limits<long long>::max()) {
            std::printf(
                "predict A(%d,%d): no feasible connected pattern signature\n",
                n, k);
        } else {
            std::printf("predict A(%d,%d): Phi_conn(%d)=%lld via "
                        "(D,X,p,s_a)=(%d,%d,%d,%d)\n",
                        n, k, R, best, best_signature[0], best_signature[1],
                        best_signature[2], best_signature[3]);
        }
    }
    return 0;
}
