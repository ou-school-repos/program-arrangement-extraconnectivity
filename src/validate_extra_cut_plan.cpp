// Zero-allocation planning report for the full-Star validator.
// Usage: validate_extra_cut_plan n k

#include "arrangement_utils.hpp"
#include "build_info.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

using boost::multiprecision::cpp_int;

namespace {

cpp_int permutation_count(int n, int k) {
    cpp_int result = 1;
    for (int i = 0; i < k; ++i)
        result *= n - i;
    return result;
}

cpp_int power_count(int n, int k) {
    cpp_int result = 1;
    for (int i = 0; i < k; ++i)
        result *= n;
    return result;
}

long long defect_sum(int r) {
    long long result = 0;
    for (int value = 0; value < r; ++value)
        result += __builtin_popcount(static_cast<unsigned>(value));
    return result;
}

int bit_length(int value) {
    int result = 0;
    while (value > 0) {
        ++result;
        value >>= 1;
    }
    return result;
}

long long collision_constant(int r) {
    long long result = r == 0 ? 0 : r - 1 - defect_sum(r);
    for (int value = 1; value < r; ++value)
        result += bit_length(value);
    return result;
}

std::string format_bytes(const cpp_int &bytes) {
    const cpp_int gib = cpp_int{1} << 30;
    const cpp_int mib = cpp_int{1} << 20;
    if (bytes >= gib)
        return (bytes / gib).convert_to<std::string>() + " GiB (binary)";
    return (bytes / mib).convert_to<std::string>() + " MiB (binary)";
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " n k\n";
        return 1;
    }

    const int n = std::stoi(argv[1]);
    const int k = std::stoi(argv[2]);
    if (n <= k || k < 1 || n > 64 || k > 21) {
        std::cerr << "Error: require 64 >= n > k >= 1 and k <= 21.\n";
        return 1;
    }

    const int m = n - k;
    const int degree = k * m;
    const int volume = 1 + degree;
    const int g = degree;
    const int d = bit_length(degree);
    const cpp_int vertices = permutation_count(n, k);
    const cpp_int flat_slots = power_count(n, k);
    const cpp_int edges = vertices * degree / 2;
    const cpp_int bitset_bytes = (vertices + 7) / 8;
    const cpp_int state_bytes = bitset_bytes * 3;

    const long long hamming = static_cast<long long>(volume) * degree -
                              (collision_constant(volume) +
                               static_cast<long long>(m) * defect_sum(volume));
    const long long star_boundary =
        static_cast<long long>(k) * (k - 1) * m * (m + 1) / 2;
    const long long delta = hamming - star_boundary;
    const bool gate = d <= k && d <= m;
    const bool rank32 = vertices <= std::numeric_limits<std::uint32_t>::max();
    const bool rank64 = vertices <= std::numeric_limits<std::uint64_t>::max();

    std::cout << "Build: " << build_version << '\n'
              << "Planning full-Star A(" << n << ',' << k << ")\n"
              << "No graph/BFS state allocated; this is an arithmetic plan.\n"
              << "m = " << m << "\n"
              << "degree = " << degree << "\n"
              << "|V| = nP_k = " << vertices << '\n'
              << "n^k = " << flat_slots << '\n'
              << "|E| = |V|*degree/2 = " << edges << '\n'
              << "R = " << volume << ", g = " << g << '\n'
              << "d = ceil(log2(R)) = " << d << '\n'
              << "|N(S)| = " << star_boundary << '\n'
              << "Hamming baseline = " << hamming << '\n'
              << "Delta = Hamming - Star = " << delta << '\n'
              << "Embedding gate: " << (gate ? "open" : "closed") << '\n'
              << "Ranked uint32 frontier: "
              << (rank32 ? "fits" : "does not fit") << '\n'
              << "Ranked uint64 frontier: "
              << (rank64 ? "fits" : "does not fit") << '\n'
              << "One bitset: " << std::setw(16) << bitset_bytes << " bytes ["
              << format_bytes(bitset_bytes) << "]\n"
              << "Three bitmap states: " << std::setw(16) << state_bytes
              << " bytes [" << format_bytes(state_bytes) << "]\n"
              << "Flat n^k guard: " << std::setw(16) << flat_slots << " slots\n"
              << "Star classification: ";

    if (delta > 0 && gate)
        std::cout << "hard-counterexample candidate\n";
    else if (delta > 0)
        std::cout << "soft-counterexample candidate\n";
    else
        std::cout << "satisfies Hamming comparison\n";

    std::cout << "Note: connectivity and extra-cut validity still require the "
                 "validator.\n";
    return 0;
}
