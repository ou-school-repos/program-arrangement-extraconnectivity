// Evaluate the affine lower envelope of a d2_pattern_catalogue file.
// Usage: envelope_sweep CATALOGUE R k max_m

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {

struct Signature {
    int defect;
    int collisions;
    int active_coordinates;
    int extra_symbols;

    auto key() const {
        return std::tie(defect, collisions, active_coordinates, extra_symbols);
    }
};

bool operator<(const Signature &left, const Signature &right) {
    return left.key() < right.key();
}

long long boundary(const Signature &signature, const long long volume,
                   const long long dimension, const long long slack) {
    // Deleting a constant coordinate leaves all R rows distinct, so it adds
    // R roots and contributes zero to the full defect.
    return (volume * dimension - signature.defect) * slack - signature.defect -
           signature.collisions;
}

long long parse_positive(const char *text, const char *name) {
    char *end = nullptr;
    const long long value = std::strtoll(text, &end, 10);
    if (end == text || *end != '\0' || value < 1) {
        std::cerr << name << " must be a positive integer\n";
        std::exit(2);
    }
    return value;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " CATALOGUE R k max_m\n";
        return 2;
    }
    const std::string path = argv[1];
    const long long volume = parse_positive(argv[2], "R");
    const long long dimension = parse_positive(argv[3], "k");
    const long long max_slack = parse_positive(argv[4], "max_m");
    std::ifstream input(path);
    if (!input) {
        std::cerr << "Cannot open catalogue: " << path << '\n';
        return 2;
    }

    long long catalogue_volume = 0;
    std::set<Signature> signatures;
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("D2CAT R=", 0) == 0) {
            std::istringstream header(line.substr(8));
            header >> catalogue_volume;
            continue;
        }
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream row(line);
        Signature signature{};
        long long classes = 0;
        if (row >> signature.defect >> signature.collisions >>
            signature.active_coordinates >> signature.extra_symbols >> classes)
            signatures.insert(signature);
    }
    if (catalogue_volume != volume || signatures.empty()) {
        std::cerr << "Catalogue is empty or its R header does not match R="
                  << volume << '\n';
        return 2;
    }

    std::cout << "R=" << volume << " k=" << dimension
              << " active-coordinate envelope\n";
    std::cout << "m n boundary max_defect maxD_maxX winners(D,X,p,e)\n";
    for (long long slack = 1; slack <= max_slack; ++slack) {
        std::vector<Signature> feasible;
        std::copy_if(signatures.begin(), signatures.end(),
                     std::back_inserter(feasible), [&](const Signature &s) {
                         return s.active_coordinates <= dimension &&
                                s.extra_symbols <= slack;
                     });
        if (feasible.empty()) {
            std::cout << slack << ' ' << dimension + slack
                      << " no-feasible-signature\n";
            continue;
        }

        int max_defect = std::numeric_limits<int>::min();
        int max_collision_at_max_defect = std::numeric_limits<int>::min();
        long long best = std::numeric_limits<long long>::max();
        for (const Signature &signature : feasible) {
            max_defect = std::max(max_defect, signature.defect);
            best =
                std::min(best, boundary(signature, volume, dimension, slack));
        }
        for (const Signature &signature : feasible)
            if (signature.defect == max_defect)
                max_collision_at_max_defect =
                    std::max(max_collision_at_max_defect, signature.collisions);

        std::vector<Signature> winners;
        bool maximum_defect_wins = false;
        bool collision_maximal_winner = false;
        for (const Signature &signature : feasible) {
            if (boundary(signature, volume, dimension, slack) != best)
                continue;
            winners.push_back(signature);
            if (signature.defect == max_defect) {
                maximum_defect_wins = true;
                collision_maximal_winner |=
                    signature.collisions == max_collision_at_max_defect;
            }
        }
        std::cout << slack << ' ' << dimension + slack << ' ' << best << ' '
                  << (maximum_defect_wins ? "yes" : "no") << ' '
                  << (collision_maximal_winner ? "yes" : "no") << ' ';
        for (std::size_t index = 0; index < winners.size(); ++index) {
            if (index != 0)
                std::cout << ';';
            const Signature &signature = winners[index];
            std::cout << '(' << signature.defect << ',' << signature.collisions
                      << ',' << signature.active_coordinates << ','
                      << signature.extra_symbols << ')';
        }
        std::cout << '\n';
    }
}
