// Exhaustive finite audit of the arithmetic surrogate used in the
// defect/collision lemma.
//
// Usage: ./audit_surrogate [max_R]
//
// This checks the component deltas and the actual residual
// B(a) + B(b) - B(a+b) for every positive a,b with a+b <= max_R.
// It is an audit, not a proof of the corresponding universal statements.

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using Int = std::int64_t;

Int ceil_log2_plus_one(Int i) {
    // Returns ceil(log2(i + 1)) for i >= 1, without floating point.
    Int x = i + 1;
    Int power = 1;
    Int result = 0;
    while (power < x) {
        power <<= 1;
        ++result;
    }
    return result;
}

struct Witness {
    Int value = std::numeric_limits<Int>::max();
    Int a = 0;
    Int b = 0;

    void consider(Int candidate, Int lhs, Int rhs) {
        if (candidate < value) {
            value = candidate;
            a = lhs;
            b = rhs;
        }
    }
};

void print_witness(const char *name, const Witness &w) {
    std::cout << name << "=" << w.value << " witness=(" << w.a << "," << w.b
              << ")\n";
}

} // namespace

int main(int argc, char **argv) {
    if (argc > 1 &&
        (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "usage: " << argv[0] << " [max_R]\n"
                  << "  max_R defaults to 10000\n";
        return 0;
    }

    Int limit = 10000;
    if (argc > 1) {
        try {
            std::size_t parsed = 0;
            limit = std::stoll(argv[1], &parsed);
            if (parsed != std::string(argv[1]).size())
                throw std::invalid_argument("trailing characters");
        } catch (const std::exception &) {
            std::cerr << "usage: " << argv[0] << " [max_R]\n"
                      << "error: max_R must be an integer\n";
            return 2;
        }
    }
    if (limit < 2) {
        std::cerr << "max_R must be at least 2\n";
        return 2;
    }

    std::vector<Int> E(static_cast<std::size_t>(limit + 1));
    std::vector<Int> S(static_cast<std::size_t>(limit + 1));
    std::vector<Int> C(static_cast<std::size_t>(limit + 1));

    for (Int r = 1; r <= limit; ++r) {
        E[static_cast<std::size_t>(r)] =
            E[static_cast<std::size_t>(r - 1)] +
            __builtin_popcountll(static_cast<unsigned long long>(r - 1));
        if (r >= 2)
            S[static_cast<std::size_t>(r)] =
                S[static_cast<std::size_t>(r - 1)] + ceil_log2_plus_one(r - 1);
        C[static_cast<std::size_t>(r)] = r - 1 +
                                         S[static_cast<std::size_t>(r)] -
                                         E[static_cast<std::size_t>(r)];
    }

    Witness dE, dS, dC, residual;
    Int checked = 0;
    for (Int a = 1; a < limit; ++a) {
        for (Int b = 1; a + b <= limit; ++b) {
            const Int r = a + b;
            const Int delta_e = E[static_cast<std::size_t>(r)] -
                                E[static_cast<std::size_t>(a)] -
                                E[static_cast<std::size_t>(b)];
            const Int delta_s = S[static_cast<std::size_t>(r)] -
                                S[static_cast<std::size_t>(a)] -
                                S[static_cast<std::size_t>(b)];
            const Int delta_c = C[static_cast<std::size_t>(r)] -
                                C[static_cast<std::size_t>(a)] -
                                C[static_cast<std::size_t>(b)];

            // The B residual at m=1 is 1 + Delta S.  For m >= 1,
            // residual(m) = (m - 1) Delta E + 1 + Delta S.
            const Int b_residual_m1 = 1 + delta_s;

            dE.consider(delta_e, a, b);
            dS.consider(delta_s, a, b);
            dC.consider(delta_c, a, b);
            residual.consider(b_residual_m1, a, b);
            ++checked;
        }
    }

    std::cout << "Audited positive splits with a+b <= " << limit << ": "
              << checked << "\n";
    print_witness("min_delta_E", dE);
    print_witness("min_delta_S", dS);
    print_witness("min_delta_C", dC);
    print_witness("min_B_residual_at_m=1", residual);

    const bool component_ok = dE.value >= 0 && dS.value >= 0;
    const bool residual_ok = residual.value >= 0;
    std::cout << "E_superadditive_on_scan=" << (dE.value >= 0 ? "yes" : "no")
              << '\n';
    std::cout << "S_superadditive_on_scan=" << (dS.value >= 0 ? "yes" : "no")
              << '\n';
    std::cout << "B_subadditive_on_scan_for_all_m>=1="
              << (component_ok && residual_ok ? "yes" : "no") << '\n';

    return component_ok && residual_ok ? 0 : 1;
}
