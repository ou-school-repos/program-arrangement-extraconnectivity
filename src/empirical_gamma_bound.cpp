// Empirical audit of the L-J-Delta ledger.
// Usage: ./empirical_gamma_bound [n] [k] [max_R] [csv-path]

#include "arrangement_core.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using arrangement::Automorphism;
using arrangement::Instance;

namespace {

std::vector<int> representative(const std::vector<int> &subset,
                                const Instance &instance,
                                const std::vector<Automorphism> &stabilizer) {
    if (subset.size() == 1)
        return {0};
    return arrangement::canonical_key(subset, instance, stabilizer);
}

std::vector<unsigned char> boundary(const std::vector<int> &subset,
                                    const Instance &instance) {
    std::vector<unsigned char> selected(instance.vertices.size(), false);
    std::vector<unsigned char> result(instance.vertices.size(), false);
    for (const int vertex : subset)
        selected[vertex] = true;
    for (const int vertex : subset) {
        for (int position = 0; position < instance.k; ++position) {
            const int root = instance.root_id[position][vertex];
            for (const int neighbor : instance.lines[position][root])
                if (!selected[neighbor])
                    result[neighbor] = true;
        }
    }
    return result;
}

int count_true(const std::vector<unsigned char> &values) {
    return static_cast<int>(std::count(values.begin(), values.end(), true));
}

int cross_edges(const std::vector<int> &a, const std::vector<int> &b,
                const Instance &instance) {
    std::vector<unsigned char> in_b(instance.vertices.size(), false);
    for (const int vertex : b)
        in_b[vertex] = true;
    int result = 0;
    for (const int vertex : a) {
        for (int position = 0; position < instance.k; ++position) {
            const int root = instance.root_id[position][vertex];
            result += static_cast<int>(std::count_if(
                instance.lines[position][root].begin(),
                instance.lines[position][root].end(),
                [&](const int neighbor) { return in_b[neighbor] != 0; }));
        }
    }
    return result;
}

} // namespace

int main(int argc, char **argv) {
    int n = 5;
    int k = 3;
    int max_r = 6;
    std::string csv_path;
    std::vector<double> phi;
    int positional = 0;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "usage: " << argv[0]
                      << " [n] [k] [max_R] [csv-path]"
                         " [--phi=v0,v1,...]\n";
            return 0;
        }
        if (arg.rfind("--phi=", 0) == 0) {
            std::stringstream values(arg.substr(6));
            std::string item;
            while (std::getline(values, item, ',')) {
                if (item.empty()) {
                    std::cerr << "empty value in --phi list\n";
                    return 2;
                }
                phi.push_back(std::stod(item));
            }
            continue;
        }
        if (arg.rfind("--", 0) == 0) {
            std::cerr << "unknown option: " << arg << '\n';
            return 2;
        }
        if (positional == 0)
            n = std::stoi(arg);
        else if (positional == 1)
            k = std::stoi(arg);
        else if (positional == 2)
            max_r = std::stoi(arg);
        else if (positional == 3)
            csv_path = arg;
        else {
            std::cerr << "too many positional arguments\n";
            return 2;
        }
        ++positional;
    }

    if (n < 1 || k < 1 || k >= n) {
        std::cerr << "Error: require n >= 1, 1 <= k < n\n";
        return 2;
    }
    if (max_r < 2 || max_r > 31) {
        std::cerr << "Error: require 2 <= max_R <= 31\n";
        return 2;
    }

    const auto Phi = [&phi](const std::size_t size) {
        return size < phi.size() ? phi[size] : 0.0;
    };

    const Instance instance(n, k);
    const auto stabilizer = arrangement::origin_stabilizer(instance);
    std::vector<int> origin(k);
    std::iota(origin.begin(), origin.end(), 0);
    const int origin_id = instance.index.at(instance.encode(origin));

    std::vector<std::vector<std::vector<int>>> layers(max_r + 1);
    layers[1] = {{origin_id}};
    std::cout << "Generating canonical subsets for A(" << n << ',' << k
              << ") through R=" << max_r << "\n";
    for (int r = 2; r <= max_r; ++r) {
        std::set<std::vector<int>> next;
        for (const auto &subset : layers[r - 1]) {
            for (int vertex = 0;
                 vertex < static_cast<int>(instance.vertices.size());
                 ++vertex) {
                if (std::find(subset.begin(), subset.end(), vertex) !=
                    subset.end())
                    continue;
                auto expanded = subset;
                expanded.push_back(vertex);
                std::sort(expanded.begin(), expanded.end());
                next.insert(representative(expanded, instance, stabilizer));
            }
        }
        layers[r].assign(next.begin(), next.end());
        std::cout << "R=" << r << " canonical-subsets=" << layers[r].size()
                  << '\n';
    }

    std::ofstream csv;
    std::ostream *out = &std::cout;
    if (!csv_path.empty()) {
        csv.open(csv_path);
        if (!csv) {
            std::cerr << "unable to open CSV: " << csv_path << '\n';
            return 2;
        }
        out = &csv;
    }
    *out << "R,A_size,B_size,L,J,K,Delta,residual,gamma\n";

    double max_gamma = 0.0;
    std::vector<int> witness;
    for (int r = 2; r <= max_r; ++r) {
        for (const auto &subset : layers[r]) {
            const int split_count = static_cast<int>((1U << (r - 1)) - 1);
            for (int mask = 1; mask <= split_count; ++mask) {
                std::vector<int> a;
                std::vector<int> b;
                for (int i = 0; i < r; ++i)
                    ((mask & (1 << i)) != 0 ? a : b).push_back(subset[i]);

                const auto boundary_a = boundary(a, instance);
                const auto boundary_b = boundary(b, instance);
                const auto boundary_union = boundary(subset, instance);
                const int boundary_a_size = count_true(boundary_a);
                const int boundary_b_size = count_true(boundary_b);
                const int union_size = count_true(boundary_union);
                int j = 0;
                for (std::size_t i = 0; i < boundary_a.size(); ++i)
                    j += boundary_a[i] && boundary_b[i];
                const int l = boundary_a_size + boundary_b_size - union_size;
                const int k_cross = cross_edges(a, b, instance);
                const double delta =
                    Phi(a.size()) + Phi(b.size()) - Phi(subset.size());
                const double residual = l - delta - j;
                const double gamma =
                    k_cross > 0 ? std::max(0.0, residual / k_cross)
                                : (residual > 0
                                       ? std::numeric_limits<double>::infinity()
                                       : 0.0);
                *out << r << ',' << a.size() << ',' << b.size() << ',' << l
                     << ',' << j << ',' << k_cross << ',' << delta << ','
                     << residual << ',' << gamma << '\n';
                if (gamma > max_gamma) {
                    max_gamma = gamma;
                    witness = subset;
                }
            }
        }
    }
    std::cout << "max-gamma-diagnostic=" << std::setprecision(12) << max_gamma
              << "\n";
    if (!witness.empty()) {
        std::cout << "witness-size=" << witness.size() << " vertices=";
        for (const int vertex : witness)
            std::cout << ' ' << vertex;
        std::cout << '\n';
    }
    return 0;
}
