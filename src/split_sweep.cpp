// Enumerate integer-partition lower bounds for unrestricted profiles.
// Input file format: one row per volume, `r connected_profile`.
// Usage: split_sweep PROFILE_TABLE R connected_profile_R

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace {

using ProfileTable = std::map<int, long long>;

long long parse_integer(const char *text, const char *name) {
    char *end = nullptr;
    const long long value = std::strtoll(text, &end, 10);
    if (end == text || *end != '\0') {
        std::cerr << name << " must be an integer\n";
        std::exit(2);
    }
    return value;
}

void enumerate_partitions(
    const int remaining, const int minimum_part, const ProfileTable &profiles,
    std::vector<int> &partition,
    std::vector<std::pair<std::vector<int>, long long>> &results) {
    if (remaining == 0) {
        if (partition.size() < 2)
            return;
        long long sum = std::accumulate(
            partition.begin(), partition.end(), 0LL,
            [&](long long acc, int part) { return acc + profiles.at(part); });
        results.emplace_back(partition, sum);
        return;
    }
    for (int part = minimum_part; part <= remaining; ++part) {
        if (profiles.find(part) == profiles.end())
            continue;
        partition.push_back(part);
        enumerate_partitions(remaining - part, part, profiles, partition,
                             results);
        partition.pop_back();
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " PROFILE_TABLE R connected_profile_R\n";
        return 2;
    }
    const std::string path = argv[1];
    const long long parsed_volume = parse_integer(argv[2], "R");
    const long long connected_profile =
        parse_integer(argv[3], "connected_profile_R");
    if (parsed_volume < 2 || parsed_volume > 50 || connected_profile < 0) {
        std::cerr << "Require 2 <= R <= 50 and connected_profile_R >= 0\n";
        return 2;
    }
    const int volume = static_cast<int>(parsed_volume);

    std::ifstream input(path);
    if (!input) {
        std::cerr << "Cannot open profile table: " << path << '\n';
        return 2;
    }
    ProfileTable profiles;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream row(line);
        int part = 0;
        long long value = 0;
        if (!(row >> part >> value))
            continue;
        if (part < 1 || value < 0) {
            std::cerr << "Invalid profile row: " << line << '\n';
            return 2;
        }
        profiles[part] = value;
    }
    if (profiles.find(1) == profiles.end()) {
        std::cerr << "Profile table must include volume 1\n";
        return 2;
    }
    for (int part = 1; part < volume; ++part)
        if (profiles.find(part) == profiles.end()) {
            std::cerr << "Profile table is missing volume " << part << '\n';
            return 2;
        }

    std::vector<std::pair<std::vector<int>, long long>> partitions;
    std::vector<int> current;
    enumerate_partitions(volume, 1, profiles, current, partitions);
    if (partitions.empty()) {
        std::cerr << "No complete profile data for partitions of R=" << volume
                  << '\n';
        return 2;
    }
    std::sort(partitions.begin(), partitions.end(),
              [](const auto &left, const auto &right) {
                  if (left.second != right.second)
                      return left.second < right.second;
                  return left.first < right.first;
              });

    const long long best_split = partitions.front().second;
    std::cout << "R=" << volume << " connected=" << connected_profile
              << " best_partition_lower_bound=" << best_split << '\n';
    std::cout << "partition sum comparison\n";
    for (const auto &[partition, sum] : partitions) {
        for (std::size_t index = 0; index < partition.size(); ++index) {
            if (index != 0)
                std::cout << '+';
            std::cout << partition[index];
        }
        std::cout << ' ' << sum << ' '
                  << (sum < connected_profile ? "below-connected; check packing"
                                              : "not-below-connected")
                  << '\n';
    }
    if (best_split >= connected_profile)
        std::cout << "Conclusion: integer-partition bounds rule out a lower "
                     "disconnected profile.\n";
    else
        std::cout << "Conclusion: the split bound is inconclusive until a "
                     "distance-three placement is checked.\n";
}
