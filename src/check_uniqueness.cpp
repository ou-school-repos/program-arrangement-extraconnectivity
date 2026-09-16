// Checks whether the "tight" fibers used by check_amortized_slack.cpp
// (subsets whose external boundary hits rhs(c) exactly) are always
// Hamming-ball-isomorphic, or whether a non-Hamming-ball configuration
// can also hit the boundary minimum via cross-collision compensation.
//
// Context: docs/proof-sketch-weighted-potential.md, "amortized slack"
// trailhead. A claim was made that "s_a=s_b=0 is exactly when fibers are
// hypercube embeddings" -- but the repo's own open uniqueness_conjecture
// (proofs/Arrangement/ArrangementExtraconnectivity.lean) explicitly
// leaves open whether a topology with strictly fewer internal edges than
// the Hamming ball (defect < E_seq(R)) can still match its boundary via
// a larger cross-collision count. This tool tests that directly: for
// every subset of size c whose boundary equals rhs(c), report whether
// its defect equals E_seq(c) (Hamming-ball-consistent) or not
// (a potential counterexample to "tight implies Hamming-ball-isomorphic").
//
// Usage: ./check_uniqueness n k c
//
// Build: make check_uniqueness (repo Makefile; -std=c++17 -O3)

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <vector>

namespace {

using Word = std::uint64_t;
constexpr int WBITS = 64;

std::int64_t e_seq(int size) {
    std::int64_t total = 0;
    for (int v = 0; v < size; ++v)
        total += __builtin_popcount(static_cast<unsigned>(v));
    return total;
}

std::int64_t c_constant(int size) {
    if (size == 0)
        return 0;
    std::int64_t total = size - 1 - e_seq(size);
    for (int v = 1; v < size; ++v)
        total += 32 - __builtin_clz(static_cast<unsigned>(v));
    return total;
}

std::int64_t rhs(int n, int k, int R) {
    return (static_cast<std::int64_t>(R) * k - e_seq(R)) *
               static_cast<std::int64_t>(n - k) -
           c_constant(R);
}

struct Bits {
    std::vector<Word> w;
    explicit Bits(std::size_t words = 0) : w(words, 0) {}
    void set(int i) { w[i / WBITS] |= (Word{1} << (i % WBITS)); }
    Bits andNot(const Bits &o) const {
        Bits r(w.size());
        for (std::size_t i = 0; i < w.size(); ++i)
            r.w[i] = w[i] & ~o.w[i];
        return r;
    }
    void orWith(const Bits &o) {
        for (std::size_t i = 0; i < w.size(); ++i)
            w[i] |= o.w[i];
    }
    int popcount() const {
        int c = 0;
        for (Word x : w)
            c += __builtin_popcountll(x);
        return c;
    }
};

std::vector<std::vector<int>> generate_vertices(int n, int k) {
    std::vector<std::vector<int>> verts;
    std::vector<int> used(n, 0);
    std::vector<int> cur;
    std::function<void()> rec = [&]() {
        if (static_cast<int>(cur.size()) == k) {
            verts.push_back(cur);
            return;
        }
        for (int s = 0; s < n; ++s) {
            if (used[s])
                continue;
            used[s] = 1;
            cur.push_back(s);
            rec();
            cur.pop_back();
            used[s] = 0;
        }
    };
    rec();
    return verts;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " n k c\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int c = std::atoi(argv[3]);

    auto verts = generate_vertices(n, k);
    const int N = static_cast<int>(verts.size());
    const std::size_t words = (N + WBITS - 1) / WBITS;

    std::map<std::vector<int>, int> index;
    for (int i = 0; i < N; ++i)
        index[verts[i]] = i;

    std::vector<Bits> nbrMask(N, Bits(words));
    for (int i = 0; i < N; ++i) {
        const auto &v = verts[i];
        std::vector<int> used(n, 0);
        for (int s : v)
            used[s] = 1;
        for (int pos = 0; pos < k; ++pos) {
            for (int s = 0; s < n; ++s) {
                if (used[s])
                    continue;
                std::vector<int> w = v;
                w[pos] = s;
                nbrMask[i].set(index.at(w));
            }
        }
    }

    const std::int64_t target = rhs(n, k, c);
    const std::int64_t e_opt = e_seq(c);

    std::vector<int> idx(c);
    std::iota(idx.begin(), idx.end(), 0);
    auto next_combination = [&]() -> bool {
        int i = c - 1;
        while (i >= 0 && idx[i] == N - c + i)
            --i;
        if (i < 0)
            return false;
        ++idx[i];
        for (int j = i + 1; j < c; ++j)
            idx[j] = idx[j - 1] + 1;
        return true;
    };

    long tight = 0;
    long non_hb = 0;
    do {
        Bits members(words), eb(words);
        for (int id : idx)
            members.set(id);
        for (int id : idx)
            eb.orWith(nbrMask[id]);
        eb = eb.andNot(members);
        if (eb.popcount() != target)
            continue;
        ++tight;

        // defect = c*k - sum over positions of #distinct roots at that
        // position (root = the (k-1)-tuple with position p deleted).
        long unique_roots = 0;
        for (int p = 0; p < k; ++p) {
            std::vector<std::vector<int>> roots;
            roots.reserve(c);
            for (int id : idx) {
                auto v = verts[id];
                v.erase(v.begin() + p);
                roots.push_back(std::move(v));
            }
            std::sort(roots.begin(), roots.end());
            roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
            unique_roots += static_cast<long>(roots.size());
        }
        const long defect = static_cast<long>(c) * k - unique_roots;

        if (defect != e_opt) {
            ++non_hb;
            if (non_hb <= 5) {
                std::cerr << "  non-HB tight fiber, defect=" << defect
                          << " (E_opt=" << e_opt << "): ";
                for (int id : idx) {
                    for (int x : verts[id])
                        std::cerr << x;
                    std::cerr << " ";
                }
                std::cerr << "\n";
            }
        }
    } while (next_combination());

    std::cout << "A(" << n << "," << k << ") c=" << c << ": tight=" << tight
              << " non-HB-defect tight=" << non_hb << "\n";
    return 0;
}
