// Exhaustive distance-two-connected pattern catalogue up to row, coordinate,
// and global-symbol relabelling.  Patterns are stored as small injective word
// matrices; nauty canonicalizes their four-partite incidence graphs.
//
// Usage: ./d2_pattern_catalogue R [--max-states N] [--all-patterns PATH]
//
// The output signatures use active-coordinate defect
//   D_a = R*p - sum_i |projection_i(S)|,
// so their boundary line is
//   (R*k - D_a)*m - D_a - X, m=n-k.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" {
#include <nauty/nauty.h>
}

namespace {

using Word = std::vector<int>;
using Matrix = std::vector<Word>;

struct Pattern {
    Matrix rows;
    int symbol_count = 0;
    std::string key;
};

struct Signature {
    int defect = 0;
    int collisions = 0;
    int active_coordinates = 0;
    int extra_symbols = 0;

    bool operator<(const Signature &other) const {
        return std::tie(defect, collisions, active_coordinates, extra_symbols) <
               std::tie(other.defect, other.collisions,
                        other.active_coordinates, other.extra_symbols);
    }
};

int target_volume = 0;
int coordinate_bound = 0;
int symbol_bound = 0;
std::size_t max_states = 100'000;
std::string all_patterns_path;
std::uint64_t generated_extensions = 0;
std::uint64_t canonical_rejections = 0;
std::uint64_t duplicate_children = 0;

bool is_injective(const Word &word) {
    std::set<int> symbols(word.begin(), word.end());
    return symbols.size() == word.size();
}

void normalize_new_symbols(Word &word, const int old_symbol_count) {
    std::map<int, int> renaming;
    int next_symbol = old_symbol_count;
    for (int &symbol : word) {
        if (symbol >= old_symbol_count) {
            const auto [position, inserted] =
                renaming.emplace(symbol, next_symbol);
            if (inserted)
                ++next_symbol;
            symbol = position->second;
        }
    }
}

int matrix_symbol_count(const Matrix &rows) {
    int largest = std::accumulate(
        rows.begin(), rows.end(), -1, [](int max_val, const Word &row) {
            return std::accumulate(row.begin(), row.end(), max_val,
                                   [](int a, int b) { return std::max(a, b); });
        });
    return largest + 1;
}

void normalize_matrix_symbols(Matrix &rows) {
    std::set<int> used;
    for (const Word &row : rows)
        used.insert(row.begin(), row.end());
    std::map<int, int> renaming;
    int next_symbol = 0;
    for (const int symbol : used)
        renaming.emplace(symbol, next_symbol++);
    for (Word &row : rows)
        std::transform(row.begin(), row.end(), row.begin(),
                       [&renaming](int symbol) { return renaming.at(symbol); });
}

std::string canonical_key(const Matrix &input_rows) {
    Matrix rows = input_rows;
    normalize_matrix_symbols(rows);
    const int row_count = static_cast<int>(rows.size());
    const int symbol_count = matrix_symbol_count(rows);
    const int cell_count = row_count * coordinate_bound;
    const int node_count =
        row_count + coordinate_bound + symbol_count + cell_count;
    const int word_count = SETWORDSNEEDED(node_count);

    std::vector<graph> graph_data(static_cast<std::size_t>(word_count) *
                                  static_cast<std::size_t>(node_count));
    std::vector<graph> canonical_data(graph_data.size());
    std::vector<int> lab(static_cast<std::size_t>(node_count));
    std::vector<int> partition(static_cast<std::size_t>(node_count));
    std::vector<int> orbits(static_cast<std::size_t>(node_count));
    EMPTYGRAPH(graph_data.data(), word_count, node_count);

    const int row_base = 0;
    const int coordinate_base = row_count;
    const int symbol_base = coordinate_base + coordinate_bound;
    const int cell_base = symbol_base + symbol_count;
    for (int row = 0; row < row_count; ++row) {
        for (int coordinate = 0; coordinate < coordinate_bound; ++coordinate) {
            const int cell = cell_base + row * coordinate_bound + coordinate;
            ADDONEEDGE(graph_data.data(), row_base + row, cell, word_count);
            ADDONEEDGE(graph_data.data(), coordinate_base + coordinate, cell,
                       word_count);
            ADDONEEDGE(graph_data.data(), symbol_base + rows[row][coordinate],
                       cell, word_count);
        }
    }

    int boundary = 0;
    const std::vector<int> class_sizes = {row_count, coordinate_bound,
                                          symbol_count, cell_count};
    for (std::size_t color = 0; color < class_sizes.size(); ++color) {
        for (int offset = 0; offset < class_sizes[color]; ++offset) {
            lab[static_cast<std::size_t>(boundary + offset)] =
                boundary + offset;
            partition[static_cast<std::size_t>(boundary + offset)] = 1;
        }
        boundary += class_sizes[color];
        if (boundary > 0)
            partition[static_cast<std::size_t>(boundary - 1)] = 0;
    }

    DEFAULTOPTIONS_GRAPH(options);
    options.getcanon = TRUE;
    options.defaultptn = FALSE;
    statsblk stats{};
    densenauty(graph_data.data(), lab.data(), partition.data(), orbits.data(),
               &options, &stats, word_count, node_count, canonical_data.data());
    if (stats.errstatus != 0)
        throw std::runtime_error(
            "nauty failed to canonicalize an incidence graph");

    // Serialize the canonical graph as a sparse adjacency list.  Include all
    // color-class sizes, which are fixed by the input matrix dimensions.
    std::string key;
    auto append_int = [&key](const int value) {
        for (unsigned shift = 0; shift < 4; ++shift)
            key.push_back(static_cast<char>(
                (static_cast<std::uint32_t>(value) >> (shift * 8U)) & 0xffU));
    };
    append_int(row_count);
    append_int(coordinate_bound);
    append_int(symbol_count);
    append_int(cell_count);
    for (int vertex = 0; vertex < node_count; ++vertex) {
        int degree = 0;
        for (int neighbor = 0; neighbor < node_count; ++neighbor)
            if (ISELEMENT(canonical_data.data() +
                              static_cast<std::size_t>(vertex) * word_count,
                          neighbor))
                ++degree;
        append_int(degree);
        for (int neighbor = 0; neighbor < node_count; ++neighbor)
            if (ISELEMENT(canonical_data.data() +
                              static_cast<std::size_t>(vertex) * word_count,
                          neighbor))
                append_int(neighbor);
    }
    return key;
}

bool distance_two_adjacent(const Word &left, const Word &right) {
    int differing[2] = {-1, -1};
    int count = 0;
    for (int coordinate = 0; coordinate < coordinate_bound; ++coordinate) {
        if (left[coordinate] == right[coordinate])
            continue;
        if (count == 2)
            return false;
        differing[count++] = coordinate;
    }
    if (count == 1)
        return true;
    if (count != 2)
        return false;

    // A length-two path exists exactly when at least one coordinatewise mix
    // of the endpoints is itself an injective word.
    Word mix = left;
    mix[differing[0]] = right[differing[0]];
    if (is_injective(mix))
        return true;
    mix = left;
    mix[differing[1]] = right[differing[1]];
    return is_injective(mix);
}

bool distance_two_connected(const Matrix &rows) {
    if (rows.size() <= 1)
        return true;
    std::vector<char> visited(rows.size(), 0);
    std::vector<std::size_t> pending{0};
    visited[0] = 1;
    std::size_t reached = 1;
    while (!pending.empty()) {
        const std::size_t current = pending.back();
        pending.pop_back();
        for (std::size_t candidate = 0; candidate < rows.size(); ++candidate) {
            if (visited[candidate] != 0)
                continue;
            if (distance_two_adjacent(rows[current], rows[candidate])) {
                visited[candidate] = 1;
                ++reached;
                pending.push_back(candidate);
            }
        }
    }
    return reached == rows.size();
}

std::string canonical_parent_key(const Matrix &child) {
    std::string best;
    bool found = false;
    for (std::size_t deleted = 0; deleted < child.size(); ++deleted) {
        Matrix parent;
        parent.reserve(child.size() - 1);
        for (std::size_t row = 0; row < child.size(); ++row)
            if (row != deleted)
                parent.push_back(child[row]);
        if (!distance_two_connected(parent))
            continue;
        const std::string key = canonical_key(parent);
        if (!found || key < best) {
            best = key;
            found = true;
        }
    }
    if (!found)
        throw std::logic_error("connected child has no connected row deletion");
    return best;
}

std::vector<std::pair<Word, int>> one_step_neighbors(const Word &word,
                                                     const int used_symbols) {
    std::vector<std::pair<Word, int>> result;
    for (int coordinate = 0; coordinate < coordinate_bound; ++coordinate) {
        for (int symbol = 0; symbol <= used_symbols; ++symbol) {
            if (std::find(word.begin(), word.end(), symbol) != word.end())
                continue;
            Word neighbor = word;
            neighbor[coordinate] = symbol;
            const int next_used =
                symbol == used_symbols ? used_symbols + 1 : used_symbols;
            result.emplace_back(std::move(neighbor), next_used);
        }
    }
    return result;
}

std::set<Word> candidate_rows(const Pattern &parent) {
    std::set<Word> candidates;
    std::set<Word> existing(parent.rows.begin(), parent.rows.end());
    for (const Word &source : parent.rows) {
        const auto first_steps =
            one_step_neighbors(source, parent.symbol_count);
        for (const auto &[first, first_symbol_count] : first_steps) {
            if (existing.find(first) == existing.end())
                candidates.insert(first);
            const auto second_steps =
                one_step_neighbors(first, first_symbol_count);
            for (const auto &[second, unused_symbol_count] : second_steps) {
                (void)unused_symbol_count;
                if (existing.find(second) != existing.end())
                    continue;
                Word normalized = second;
                normalize_new_symbols(normalized, parent.symbol_count);
                if (is_injective(normalized))
                    candidates.insert(std::move(normalized));
            }
        }
    }
    return candidates;
}

Signature compute_signature(const Matrix &rows) {
    const int volume = static_cast<int>(rows.size());
    std::vector<int> active;
    std::set<int> active_symbols;
    for (int coordinate = 0; coordinate < coordinate_bound; ++coordinate) {
        const int first = rows.front()[coordinate];
        const bool varies = std::any_of(rows.begin() + 1, rows.end(),
                                        [coordinate, first](const Word &row) {
                                            return row[coordinate] != first;
                                        });
        if (!varies)
            continue;
        active.push_back(coordinate);
        for (const Word &row : rows)
            active_symbols.insert(row[coordinate]);
    }

    int projection_sum = 0;
    for (const int removed : active) {
        std::set<Word> roots;
        for (const Word &row : rows) {
            Word root;
            root.reserve(active.size() - 1);
            for (const int coordinate : active)
                if (coordinate != removed)
                    root.push_back(row[coordinate]);
            roots.insert(std::move(root));
        }
        projection_sum += static_cast<int>(roots.size());
    }
    // A constant coordinate contributes R distinct roots, hence zero defect.
    // Therefore this active-coordinate defect is already the full defect
    // R*k - sum_i |projection_i(S)| for every embedding dimension k >= p.
    const int defect =
        volume * static_cast<int>(active.size()) - projection_sum;

    std::set<Word> members(rows.begin(), rows.end());
    std::map<Word, std::set<int>> directions;
    for (const Word &source : rows) {
        for (const int coordinate : active) {
            for (const int symbol : active_symbols) {
                if (std::find(source.begin(), source.end(), symbol) !=
                    source.end())
                    continue;
                Word neighbor = source;
                neighbor[coordinate] = symbol;
                if (members.find(neighbor) == members.end())
                    directions[std::move(neighbor)].insert(coordinate);
            }
        }
    }
    int collisions = 0;
    for (const auto &[neighbor, coordinate_set] : directions) {
        (void)neighbor;
        collisions += static_cast<int>(coordinate_set.size()) - 1;
    }

    const int active_coordinate_count = static_cast<int>(active.size());
    const int extra_symbols =
        static_cast<int>(active_symbols.size()) - active_coordinate_count;
    return {defect, collisions, active_coordinate_count, extra_symbols};
}

std::string describe_rows(const Matrix &rows) {
    std::ostringstream output;
    output << '"';
    for (std::size_t row = 0; row < rows.size(); ++row) {
        if (row != 0)
            output << ';';
        for (std::size_t coordinate = 0; coordinate < rows[row].size();
             ++coordinate) {
            if (coordinate != 0)
                output << ',';
            output << rows[row][coordinate];
        }
    }
    output << '"';
    return output.str();
}

} // namespace

int run(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " R [--max-states N] [--all-patterns PATH]\n";
        return 2;
    }
    char *end = nullptr;
    const long parsed_volume = std::strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || parsed_volume < 1 ||
        parsed_volume > 30) {
        std::cerr << "R must be an integer in 1..30\n";
        return 2;
    }
    target_volume = static_cast<int>(parsed_volume);
    for (int argument = 2; argument < argc; ++argument) {
        if (std::string(argv[argument]) == "--max-states" &&
            argument + 1 < argc) {
            const long long parsed_limit =
                std::strtoll(argv[++argument], &end, 10);
            if (end == argv[argument] || *end != '\0' || parsed_limit < 1) {
                std::cerr << "--max-states must be a positive integer\n";
                return 2;
            }
            max_states = static_cast<std::size_t>(parsed_limit);
        } else if (std::string(argv[argument]) == "--all-patterns" &&
                   argument + 1 < argc) {
            all_patterns_path = argv[++argument];
            if (all_patterns_path.empty()) {
                std::cerr << "--all-patterns requires a file path\n";
                return 2;
            }
        } else {
            std::cerr << "Unknown or incomplete option: " << argv[argument]
                      << '\n';
            return 2;
        }
    }

    coordinate_bound = std::max(1, 2 * (target_volume - 1));
    symbol_bound = coordinate_bound + 2 * (target_volume - 1);
    const int max_nodes = target_volume + coordinate_bound + symbol_bound +
                          target_volume * coordinate_bound;
    nauty_check(WORDSIZE, SETWORDSNEEDED(max_nodes), max_nodes, NAUTYVERSIONID);

    Word root(static_cast<std::size_t>(coordinate_bound));
    for (int coordinate = 0; coordinate < coordinate_bound; ++coordinate)
        root[static_cast<std::size_t>(coordinate)] = coordinate;
    Pattern initial{{root}, coordinate_bound, canonical_key(Matrix{root})};
    std::unordered_map<std::string, Pattern> current;
    current.emplace(initial.key, std::move(initial));

    std::cout << "D2CAT R=" << target_volume
              << " coordinate_bound=" << coordinate_bound
              << " symbol_bound=" << symbol_bound
              << " signatures_use=active_defect\n";
    std::cout << "level patterns extensions canonical_rejections duplicates\n";
    std::cout.flush();
    for (int level = 1; level < target_volume; ++level) {
        std::unordered_map<std::string, Pattern> next;
        std::unordered_map<std::string, std::string> child_parent_cache;
        std::size_t processed_parents = 0;
        for (const auto &[parent_key, parent] : current) {
            ++processed_parents;
            if (processed_parents % 100 == 0)
                std::cerr << "level " << level << " processed "
                          << processed_parents << '/' << current.size()
                          << " parents; retained " << next.size()
                          << " children\n";
            const std::set<Word> additions = candidate_rows(parent);
            for (const Word &addition : additions) {
                ++generated_extensions;
                Matrix child = parent.rows;
                child.push_back(addition);
                const int child_symbols = matrix_symbol_count(child);
                if (child_symbols > symbol_bound)
                    throw std::logic_error("symbol bound exceeded");

                const std::string child_key = canonical_key(child);
                auto parent_entry = child_parent_cache.find(child_key);
                if (parent_entry == child_parent_cache.end()) {
                    parent_entry =
                        child_parent_cache
                            .emplace(child_key, canonical_parent_key(child))
                            .first;
                }
                if (parent_entry->second != parent_key) {
                    ++canonical_rejections;
                    continue;
                }
                if (next.find(child_key) != next.end()) {
                    ++duplicate_children;
                    continue;
                }
                if (next.size() >= max_states) {
                    std::cerr << "Stopped incomplete at level " << level + 1
                              << ": --max-states limit " << max_states
                              << " reached.\n";
                    return 3;
                }
                next.emplace(child_key, Pattern{std::move(child), child_symbols,
                                                child_key});
            }
        }
        current = std::move(next);
        std::cout << level + 1 << ' ' << current.size() << ' '
                  << generated_extensions << ' ' << canonical_rejections << ' '
                  << duplicate_children << '\n';
        std::cout.flush();
        if (current.empty()) {
            std::cerr << "No patterns survived at level " << level + 1 << '\n';
            return 1;
        }
    }

    std::ofstream all_patterns;
    if (!all_patterns_path.empty()) {
        all_patterns.open(all_patterns_path);
        if (!all_patterns) {
            std::cerr << "Cannot open pattern output: " << all_patterns_path
                      << '\n';
            return 2;
        }
        all_patterns << "# active_defect X p e canonical_rows\n";
    }
    std::map<Signature, std::pair<std::uint64_t, Matrix>> signatures;
    for (const auto &[key, pattern] : current) {
        (void)key;
        const Signature signature = compute_signature(pattern.rows);
        if (all_patterns)
            all_patterns << signature.defect << ' ' << signature.collisions
                         << ' ' << signature.active_coordinates << ' '
                         << signature.extra_symbols << ' '
                         << describe_rows(pattern.rows) << '\n';
        auto [entry, inserted] = signatures.emplace(
            signature, std::make_pair(std::uint64_t{0}, pattern.rows));
        ++entry->second.first;
        (void)inserted;
    }
    std::cout << "# active_defect X p e isomorphism_classes witness_rows\n";
    for (const auto &[signature, data] : signatures)
        std::cout << signature.defect << ' ' << signature.collisions << ' '
                  << signature.active_coordinates << ' '
                  << signature.extra_symbols << ' ' << data.first << ' '
                  << describe_rows(data.second) << '\n';
    std::cerr << "Completed R=" << target_volume << " with " << current.size()
              << " canonical distance-two-connected patterns and "
              << signatures.size() << " signatures.\n";
    return 0;
}

int main(int argc, char **argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "d2_pattern_catalogue: " << error.what() << '\n';
        return 2;
    }
}
