// Differential regression test for the optimized and exhaustive pattern
// catalogues. The slow R=6 case is opt-in: --include-r6.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <tuple>
#include <vector>

namespace {

struct Signature {
    int defect;
    int collisions;
    int active_positions;
    int active_symbols;

    bool operator==(const Signature &other) const {
        return defect == other.defect && collisions == other.collisions &&
               active_positions == other.active_positions &&
               active_symbols == other.active_symbols;
    }

    bool operator<(const Signature &other) const {
        return std::tie(defect, collisions, active_positions, active_symbols) <
               std::tie(other.defect, other.collisions, other.active_positions,
                        other.active_symbols);
    }
};

using SignatureSet = std::set<Signature>;

struct Catalogue {
    std::uint64_t rooted_sets = 0;
    std::size_t signature_count = 0;
    SignatureSet signatures;
};

std::string run_command(const std::string &command) {
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
        throw std::runtime_error("popen failed for: " + command);

    std::string output;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr)
        output += buffer;

    const int status = pclose(pipe);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        throw std::runtime_error("command failed: " + command + "\n" + output);
    return output;
}

Catalogue parse_reference_output(const std::string &output) {
    Catalogue result;
    std::istringstream lines(output);
    std::string line;
    bool found_header = false;
    constexpr const char *rooted_count = "connected sets through root=";

    while (std::getline(lines, line)) {
        const std::size_t header = line.find(rooted_count);
        if (header != std::string::npos) {
            std::istringstream counts(line.substr(
                header + std::char_traits<char>::length(rooted_count)));
            std::string signature_field;
            if (!(counts >> result.rooted_sets >> signature_field) ||
                signature_field.rfind("signatures=", 0) != 0)
                throw std::runtime_error("cannot parse catalogue header: " +
                                         line);
            result.signature_count = static_cast<std::size_t>(std::stoull(
                signature_field.substr(std::string("signatures=").size())));
            found_header = true;
            continue;
        }

        Signature signature{};
        std::istringstream row(line);
        if (row >> signature.defect >> signature.collisions >>
            signature.active_positions >> signature.active_symbols)
            result.signatures.insert(signature);
    }

    if (!found_header)
        throw std::runtime_error("reference catalogue header not found");
    if (result.signatures.size() != result.signature_count)
        throw std::runtime_error(
            "parsed signature count disagrees with header");
    return result;
}

SignatureSet parse_optimized_frontier(const std::string &output) {
    SignatureSet result;
    std::istringstream lines(output);
    std::string line;
    while (std::getline(lines, line)) {
        Signature signature{};
        std::istringstream row(line);
        if (row >> signature.defect >> signature.collisions >>
            signature.active_positions >> signature.active_symbols)
            result.insert(signature);
    }
    return result;
}

bool dominates(const Signature &left, const Signature &right) {
    if (left.defect != right.defect)
        return false;
    const int left_extra = left.active_symbols - left.active_positions;
    const int right_extra = right.active_symbols - right.active_positions;
    const bool no_worse = left.collisions >= right.collisions &&
                          left.active_positions <= right.active_positions &&
                          left_extra <= right_extra;
    const bool strictly_better =
        left.collisions > right.collisions ||
        left.active_positions < right.active_positions ||
        left_extra < right_extra;
    return no_worse && strictly_better;
}

SignatureSet pareto_frontier(const SignatureSet &signatures) {
    SignatureSet frontier;
    for (const Signature &candidate : signatures) {
        const bool is_dominated =
            std::any_of(signatures.begin(), signatures.end(),
                        [&candidate](const Signature &other) {
                            return dominates(other, candidate);
                        });
        if (!is_dominated)
            frontier.insert(candidate);
    }
    return frontier;
}

std::string describe(const Signature &signature) {
    std::ostringstream text;
    text << '(' << signature.defect << ',' << signature.collisions << ','
         << signature.active_positions << ',' << signature.active_symbols
         << ')';
    return text.str();
}

void compare_case(const int r) {
    static const std::map<int, std::pair<std::uint64_t, std::size_t>> expected =
        {{2, {1, 1}},
         {3, {14, 3}},
         {4, {921, 10}},
         {5, {145524, 34}},
         {6, {42397005, 103}}};

    const Catalogue reference = parse_reference_output(
        run_command("./bin/pattern_catalogue " + std::to_string(r)));
    const SignatureSet expected_frontier =
        pareto_frontier(reference.signatures);
    const SignatureSet optimized = parse_optimized_frontier(
        run_command("./bin/arrangement " + std::to_string(r)));

    const auto known = expected.at(r);
    if (reference.rooted_sets != known.first ||
        reference.signature_count != known.second) {
        throw std::runtime_error(
            "reference regression at R=" + std::to_string(r) +
            ": rooted/signature counts changed");
    }
    if (optimized != expected_frontier) {
        std::ostringstream message;
        message << "frontier mismatch at R=" << r
                << "\nmissing from optimized:";
        for (const Signature &signature : expected_frontier)
            if (optimized.find(signature) == optimized.end())
                message << ' ' << describe(signature);
        message << "\nunexpected in optimized:";
        for (const Signature &signature : optimized)
            if (expected_frontier.find(signature) == expected_frontier.end())
                message << ' ' << describe(signature);
        throw std::runtime_error(message.str());
    }

    std::cout << "R=" << r << ": " << reference.rooted_sets << " rooted sets, "
              << reference.signature_count
              << " signatures; Pareto frontier agrees ("
              << expected_frontier.size() << " entries).\n";
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 1) {
        std::cerr << "Usage: " << argv[0] << '\n';
        return 2;
    }

    try {
        for (int r = 2; r <= 6; ++r)
            compare_case(r);
    } catch (const std::exception &error) {
        std::cerr << "pattern catalogue differential test failed: "
                  << error.what() << '\n';
        return 1;
    }
    return 0;
}
