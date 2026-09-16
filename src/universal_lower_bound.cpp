// Exhaustive, all-subsets checker for UniversalLowerBound.
//
// This is deliberately independent of predict.cpp: the predictor evaluates
// only the Hamming-ball witness, while this program enumerates literal subsets
// of A(n,k) and reproduces the Lean fiber definitions of boundary, defect, and
// cross-collisions.  Cross-check small runs against
// scripts/check_universal_lower_bound.py.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Vertex = std::vector<int>;

struct Metrics {
    std::int64_t boundary;
    std::int64_t defect;
    std::int64_t collisions;
};

std::int64_t e_seq(int size) {
    std::int64_t total = 0;
    for (int value = 0; value < size; ++value)
        total += __builtin_popcount(static_cast<unsigned>(value));
    return total;
}

std::int64_t c_constant(int size) {
    std::int64_t total = size - 1 - e_seq(size);
    for (int value = 1; value < size; ++value)
        total += 32 - __builtin_clz(static_cast<unsigned>(value));
    return total;
}

std::uint64_t choose_capped(int n, int r, std::uint64_t cap) {
    if (r < 0 || r > n)
        return 0;
    r = std::min(r, n - r);
    std::uint64_t result = 1;
    for (int i = 1; i <= r; ++i) {
        std::uint64_t numerator = static_cast<std::uint64_t>(n - r + i);
        std::uint64_t denominator = static_cast<std::uint64_t>(i);
        const auto cancel_numerator = std::gcd(numerator, denominator);
        numerator /= cancel_numerator;
        denominator /= cancel_numerator;
        const auto cancel_result = std::gcd(result, denominator);
        result /= cancel_result;
        denominator /= cancel_result;
        if (denominator != 1 || result > cap / numerator)
            return cap + 1;
        result *= numerator;
    }
    return result;
}

bool connected(const std::vector<int> &subset,
               const std::vector<Vertex> &vertices) {
    if (subset.size() < 2)
        return true;
    std::vector<bool> seen(subset.size());
    std::queue<std::size_t> pending;
    seen[0] = true;
    pending.push(0);
    while (!pending.empty()) {
        const auto from = pending.front();
        pending.pop();
        for (std::size_t to = 0; to < subset.size(); ++to) {
            if (seen[to])
                continue;
            int differences = 0;
            for (std::size_t p = 0; p < vertices[subset[from]].size(); ++p)
                differences +=
                    vertices[subset[from]][p] != vertices[subset[to]][p];
            if (differences == 1) {
                seen[to] = true;
                pending.push(to);
            }
        }
    }
    return std::all_of(seen.begin(), seen.end(),
                       [](bool value) { return value; });
}

class Checker {
  public:
    Checker(int n, int k, int size)
        : n_(n), k_(k), size_(size), e_(e_seq(size)), c_(c_constant(size)),
          slack_dimension_(n - k) {
        generate_vertices({}, 0);
        build_fibers();
        member_stamp_.assign(vertices_.size(), 0);
        boundary_stamp_.assign(vertices_.size(), 0);
        union_stamp_.assign(vertices_.size(), 0);
    }

    void run() {
        const auto raw_rhs =
            (static_cast<std::int64_t>(size_) * k_ - e_) * slack_dimension_ -
            c_;
        boundary_rhs_ = std::max<std::int64_t>(0, raw_rhs);
        choose_subsets(0, size_);
        std::cout << "A(" << n_ << ',' << k_ << ") R=" << size_ << ": C("
                  << vertices_.size() << ',' << size_ << ")=" << checked_
                  << " rhs=" << boundary_rhs_ << " min=" << minimum_boundary_
                  << " (" << (minimum_connected_ ? "connected" : "disconnected")
                  << ") min_disconnected=";
        if (minimum_disconnected_ == std::numeric_limits<std::int64_t>::max())
            std::cout << '-';
        else
            std::cout << minimum_disconnected_;
        std::cout << " min_compensated_slack=";
        if (minimum_compensated_slack_ ==
            std::numeric_limits<std::int64_t>::max())
            std::cout << "n/a (truncated target)";
        else
            std::cout << minimum_compensated_slack_;
        std::cout << ": PASS\n";
    }

  private:
    int n_, k_, size_;
    std::int64_t e_, c_, slack_dimension_, boundary_rhs_ = 0;
    std::vector<Vertex> vertices_;
    std::vector<std::vector<int>> root_id_;
    std::vector<std::vector<std::vector<int>>> fibers_;
    std::vector<std::vector<std::uint64_t>> root_stamp_;
    std::vector<std::uint64_t> member_stamp_, boundary_stamp_, union_stamp_;
    std::vector<int> subset_;
    std::uint64_t stamp_ = 0, checked_ = 0;
    std::int64_t minimum_boundary_ = std::numeric_limits<std::int64_t>::max();
    std::int64_t minimum_disconnected_ =
        std::numeric_limits<std::int64_t>::max();
    std::int64_t minimum_compensated_slack_ =
        std::numeric_limits<std::int64_t>::max();
    bool minimum_connected_ = true;

    void generate_vertices(Vertex vertex, int next_symbol) {
        if (static_cast<int>(vertex.size()) == k_) {
            vertices_.push_back(std::move(vertex));
            return;
        }
        for (int symbol = 0; symbol < n_; ++symbol) {
            if (std::find(vertex.begin(), vertex.end(), symbol) != vertex.end())
                continue;
            auto next = vertex;
            next.push_back(symbol);
            generate_vertices(std::move(next), next_symbol + 1);
        }
    }

    void build_fibers() {
        root_id_.assign(k_, std::vector<int>(vertices_.size()));
        fibers_.resize(k_);
        root_stamp_.resize(k_);
        for (int position = 0; position < k_; ++position) {
            std::map<Vertex, int> ids;
            for (std::size_t index = 0; index < vertices_.size(); ++index) {
                Vertex root = vertices_[index];
                root.erase(root.begin() + position);
                auto [it, inserted] =
                    ids.emplace(std::move(root), static_cast<int>(ids.size()));
                if (inserted)
                    fibers_[position].emplace_back();
                root_id_[position][index] = it->second;
                fibers_[position][it->second].push_back(
                    static_cast<int>(index));
            }
            root_stamp_[position].assign(fibers_[position].size(), 0);
        }
    }

    Metrics metrics() {
        const auto subset_stamp = ++stamp_;
        for (int vertex : subset_)
            member_stamp_[vertex] = subset_stamp;
        const auto union_mark = ++stamp_;
        std::int64_t roots = 0, total_coordinate_boundary = 0, boundary = 0;
        for (int position = 0; position < k_; ++position) {
            const auto root_mark = ++stamp_;
            const auto coordinate_mark = ++stamp_;
            for (int vertex : subset_) {
                const int root = root_id_[position][vertex];
                if (root_stamp_[position][root] == root_mark)
                    continue;
                root_stamp_[position][root] = root_mark;
                ++roots;
                for (int neighbor : fibers_[position][root]) {
                    if (member_stamp_[neighbor] == subset_stamp ||
                        boundary_stamp_[neighbor] == coordinate_mark)
                        continue;
                    boundary_stamp_[neighbor] = coordinate_mark;
                    ++total_coordinate_boundary;
                    if (union_stamp_[neighbor] != union_mark) {
                        union_stamp_[neighbor] = union_mark;
                        ++boundary;
                    }
                }
            }
        }
        const std::int64_t defect =
            static_cast<std::int64_t>(size_) * k_ - roots;
        return {boundary, defect, total_coordinate_boundary - boundary};
    }

    void inspect_subset() {
        ++checked_;
        if (checked_ % 10'000'000 == 0)
            std::cerr << "progress: " << checked_ << " subsets checked\n";
        const Metrics values = metrics();
        const bool is_connected = connected(subset_, vertices_);
        if (values.boundary < minimum_boundary_) {
            minimum_boundary_ = values.boundary;
            minimum_connected_ = is_connected;
        }
        if (!is_connected)
            minimum_disconnected_ =
                std::min(minimum_disconnected_, values.boundary);
        if (values.boundary < boundary_rhs_)
            throw std::runtime_error(
                "UniversalLowerBound counterexample found");
        if (boundary_rhs_ > 0) {
            const std::int64_t candidate_rhs =
                c_ - e_ + (slack_dimension_ + 1) * (e_ - values.defect);
            const std::int64_t slack = candidate_rhs - values.collisions;
            minimum_compensated_slack_ =
                std::min(minimum_compensated_slack_, slack);
            if (slack < 0)
                throw std::runtime_error(
                    "compensated-collision counterexample found");
        }
    }

    void choose_subsets(int start, int needed) {
        if (needed == 0) {
            inspect_subset();
            return;
        }
        for (int index = start;
             index <= static_cast<int>(vertices_.size()) - needed; ++index) {
            subset_.push_back(index);
            choose_subsets(index + 1, needed - 1);
            subset_.pop_back();
        }
    }
};

int parse_positive(const char *text, const char *name) {
    char *end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (*text == '\0' || *end != '\0' || value < 0 ||
        value > std::numeric_limits<int>::max())
        throw std::invalid_argument(std::string("invalid ") + name + ": " +
                                    text);
    return static_cast<int>(value);
}

std::uint64_t parse_limit(const char *text) {
    char *end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (*text == '\0' || *end != '\0' || value == 0)
        throw std::invalid_argument(std::string("invalid subset limit: ") +
                                    text);
    return static_cast<std::uint64_t>(value);
}

} // namespace

int main(int argc, char **argv) {
    constexpr std::uint64_t default_limit = 250'000'000;
    if (argc != 4 && argc != 6) {
        std::cerr << "Usage: " << argv[0] << " n k R [--max-subsets LIMIT]\n"
                  << "Exhaustively checks all R-subsets of A(n,k); limit: "
                  << default_limit << " subsets.\n";
        return 2;
    }
    try {
        const int n = parse_positive(argv[1], "n");
        const int k = parse_positive(argv[2], "k");
        const int size = parse_positive(argv[3], "R");
        std::uint64_t subset_limit = default_limit;
        if (argc == 6) {
            if (std::string(argv[4]) != "--max-subsets")
                throw std::invalid_argument("expected --max-subsets LIMIT");
            subset_limit = parse_limit(argv[5]);
        }
        if (k > n)
            throw std::invalid_argument("k must be at most n");
        std::uint64_t vertex_count = 1;
        for (int i = 0; i < k; ++i) {
            if (vertex_count >
                default_limit / static_cast<std::uint64_t>(n - i))
                throw std::invalid_argument(
                    "A(n,k) is too large for this exhaustive checker");
            vertex_count *= static_cast<std::uint64_t>(n - i);
        }
        const std::uint64_t combinations =
            choose_capped(static_cast<int>(vertex_count), size, subset_limit);
        if (combinations > subset_limit)
            throw std::invalid_argument(
                "subset count exceeds the configured limit; rerun with "
                "--max-subsets LIMIT");
        Checker(n, k, size).run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
