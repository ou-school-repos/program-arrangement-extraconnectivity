// C++ port of scripts/check_amortized_slack.py — the amortized-slack
// critical-case test for Proposition 5.3
// (docs/proof-sketch-weighted-potential.md, "amortized slack" trailhead,
// 2026-09-13 session).
//
// Diverged from the Python script (2026-09-13, later same session): tests
// the corrected combined quantity I + B_ba + B_ab <= Delta, not just
// I <= Delta, where B_ba = |F_b ∩ ∂F_a| and B_ab = |F_a ∩ ∂F_b| are the
// direct Fa<->Fb adjacency crossover counts. Derived from the exact
// identity |∂V'| = |∂F_a| + |∂F_b| - |B_ba| - |B_ab| - |I| (verified
// algebraically by decomposing ∂F_a = E_a ⊔ B_ba with E_a = ∂F_a \ F_b,
// so ∂V' = E_a ∪ E_b exactly and E_a ∩ E_b = I). The original I-only
// version undercounted whenever Fa and Fb share a direct edge; this
// version runs over the same disjoint tight-fiber pairs (no adjacency
// filter existed or exists) but reports the full combined quantity.
//
// Usage: ./check_amortized_slack n k c_a c_b
//
// Build: make check_amortized_slack (repo Makefile; -std=c++17 -O3)

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
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

// Fixed-width bitset over the vertex index space, sized at runtime to
// ceil(N/64) words (N = number of vertices = P(n,k)).
struct Bits {
    std::vector<Word> w;
    explicit Bits(std::size_t words = 0) : w(words, 0) {}
    void set(int i) { w[i / WBITS] |= (Word{1} << (i % WBITS)); }
    Bits andNot(const Bits &o) const { // this \ o
        Bits r(w.size());
        for (std::size_t i = 0; i < w.size(); ++i)
            r.w[i] = w[i] & ~o.w[i];
        return r;
    }
    void orWith(const Bits &o) {
        for (std::size_t i = 0; i < w.size(); ++i)
            w[i] |= o.w[i];
    }
    bool intersects(const Bits &o) const {
        for (std::size_t i = 0; i < w.size(); ++i)
            if (w[i] & o.w[i])
                return true;
        return false;
    }
    int popcount() const {
        int c = 0;
        for (Word x : w)
            c += __builtin_popcountll(x);
        return c;
    }
};

int popcountAnd(const Bits &a, const Bits &b) {
    int c = 0;
    for (std::size_t i = 0; i < a.w.size(); ++i)
        c += __builtin_popcountll(a.w[i] & b.w[i]);
    return c;
}

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

// Enumerate all size-c combinations of vertex indices {0,...,N-1}, keep
// only the ones whose external-boundary size hits rhs(n,k,c) exactly
// ("tight" fibers), and return them as (member-bitset, eb-bitset) pairs.
std::vector<std::pair<Bits, Bits>>
tight_fibers(int n, int k, int c, int N, const std::vector<Bits> &nbrMask,
             std::size_t words) {
    const std::int64_t target = rhs(n, k, c);
    std::vector<std::pair<Bits, Bits>> out;
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

    std::size_t scanned = 0;
    do {
        Bits members(words), eb(words);
        for (int id : idx)
            members.set(id);
        for (int id : idx)
            eb.orWith(nbrMask[id]);
        eb = eb.andNot(members);
        if (eb.popcount() == target)
            out.emplace_back(members, eb);
        ++scanned;
        if (scanned % 2'000'000 == 0)
            std::cerr << "  ...scanning size-" << c << ": " << scanned
                      << " combinations so far, " << out.size() << " tight\n";
    } while (next_combination());

    std::cerr << "  scanned " << scanned << " size-" << c
              << " combinations, found " << out.size() << " tight\n";
    return out;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " n k c_a c_b\n";
        return 1;
    }
    const int n = std::atoi(argv[1]);
    const int k = std::atoi(argv[2]);
    const int ca = std::atoi(argv[3]);
    const int cb = std::atoi(argv[4]);

    auto verts = generate_vertices(n, k);
    const int N = static_cast<int>(verts.size());
    const std::size_t words = (N + WBITS - 1) / WBITS;

    // Index map for O(log N) neighbor lookup (built once; N is at most a
    // few hundred for the sizes this tool is meant for).
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

    const std::int64_t delta =
        rhs(n, k, ca) + rhs(n, k, cb) - rhs(n, k, ca + cb);
    std::cerr << "A(" << n << "," << k << ") c_a=" << ca << " c_b=" << cb
              << " (N=" << N << " vertices): Delta=" << delta << "\n";

    auto tightA = tight_fibers(n, k, ca, N, nbrMask, words);
    auto tightB =
        (ca == cb) ? tightA : tight_fibers(n, k, cb, N, nbrMask, words);

    std::cerr << "  #tight_a=" << tightA.size() << " #tight_b=" << tightB.size()
              << "\n";

    std::int64_t worst = std::numeric_limits<std::int64_t>::min();
    std::size_t checked = 0;
    std::size_t adjacent_pairs = 0;
    auto t0 = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < tightA.size(); ++i) {
        const auto &[membersA, ebA] = tightA[i];
        for (std::size_t j = 0; j < tightB.size(); ++j) {
            if (ca == cb && j <= i)
                continue; // unordered pair, and Fa != Fb implied by
                          // disjointness check
            const auto &[membersB, ebB] = tightB[j];
            if (membersA.intersects(membersB))
                continue;
            // I: strictly-external shared targets (unaffected by Fa-Fb
            // adjacency, since ebA/ebB already exclude their own fiber but
            // not necessarily the other one -- see B_ba/B_ab below).
            const int shared = popcountAnd(ebA, ebB);
            // B_ba = |F_b ∩ ∂F_a|: Fb-members directly adjacent to Fa.
            // B_ab = |F_a ∩ ∂F_b|: Fa-members directly adjacent to Fb.
            const int b_ba = popcountAnd(ebA, membersB);
            const int b_ab = popcountAnd(ebB, membersA);
            const int combined = shared + b_ba + b_ab;
            if (b_ba > 0 || b_ab > 0)
                ++adjacent_pairs;
            const std::int64_t margin = combined - delta;
            if (margin > worst)
                worst = margin;
            ++checked;
        }
        if ((i + 1) % 200 == 0) {
            auto elapsed = std::chrono::duration<double>(
                               std::chrono::steady_clock::now() - t0)
                               .count();
            std::cerr << "  ... " << (i + 1) << "/" << tightA.size()
                      << " tight-A fibers done, " << checked
                      << " pairs checked, running max(I+B_ba+B_ab-Delta)="
                      << worst << " (" << elapsed << "s elapsed)\n";
        }
    }

    std::cout << "A(" << n << "," << k << ") c_a=" << ca << " c_b=" << cb
              << ": checked " << checked << " disjoint tight-fiber pairs ("
              << adjacent_pairs
              << " directly Fa-Fb adjacent); max(I+B_ba+B_ab-Delta) = " << worst
              << "\n";
    if (worst > 0)
        std::cout << "*** CRITICAL-CASE VIOLATION (I+B_ba+B_ab >= Delta) "
                     "***\n";
    return 0;
}
