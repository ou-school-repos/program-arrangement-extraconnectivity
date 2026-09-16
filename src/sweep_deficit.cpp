// Sweeps the amortized-slack worst-margin quantity over many (n,k,c_a,c_b)
// cells and tabulates it against the two "starvation gaps" from the
// two-sided embeddability rule (docs/proof-sketch-weighted-potential.md,
// subcube-intersection attack):
//
//   dk = ceil(log2(c_a+c_b)) - k        (coordinate-starvation gap)
//   dm = ceil(log2(c_a+c_b)) - (n-k)    (alphabet-starvation gap)
//
// margin=0 (embeddable) is known/conjectured to require dk<=0 AND dm<=0.
// This tool asks: for dk>0 and/or dm>0, is `worst` (the least-negative
// achievable margin, i.e. -deficit) a clean function of (dk,dm) alone,
// independent of the specific (n,k,c_a,c_b) realizing those gaps?
//
// Deliberately stripped down from check_amortized_slack.cpp: no T/(T,B)
// histogram, no identity/mechanism checks, no per-pair witness tracking --
// just the worst-margin scan, to run many cells quickly. See that file's
// header for the underlying I/B_ba/B_ab/Delta definitions.
//
// Usage: ./sweep_deficit n1 k1 ca1 cb1  [n2 k2 ca2 cb2  ...]
//   (repeat the four-argument group once per cell to sweep)
//
// Build: make sweep_deficit (repo Makefile; -std=c++17 -O3)

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
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

int ceil_log2(int R) {
    int bits = 0;
    while ((1 << bits) < R)
        ++bits;
    return bits;
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

    do {
        Bits members(words), eb(words);
        for (int id : idx)
            members.set(id);
        for (int id : idx)
            eb.orWith(nbrMask[id]);
        eb = eb.andNot(members);
        if (eb.popcount() == target)
            out.emplace_back(members, eb);
    } while (next_combination());

    return out;
}

struct Cell {
    int n, k, ca, cb;
};

} // namespace

int main(int argc, char **argv) {
    if (argc < 5 || (argc - 1) % 4 != 0) {
        std::cerr << "Usage: " << argv[0]
                  << " n1 k1 ca1 cb1 [n2 k2 ca2 cb2 ...]\n";
        return 1;
    }

    std::vector<Cell> cells;
    for (int i = 1; i < argc; i += 4)
        cells.push_back({std::atoi(argv[i]), std::atoi(argv[i + 1]),
                         std::atoi(argv[i + 2]), std::atoi(argv[i + 3])});

    std::cout << std::left << std::setw(4) << "n" << std::setw(4) << "k"
              << std::setw(5) << "ca" << std::setw(5) << "cb" << std::setw(5)
              << "R" << std::setw(6) << "m" << std::setw(8) << "ceilog2"
              << std::setw(5) << "dk" << std::setw(5) << "dm" << std::setw(8)
              << "Delta" << std::setw(8) << "worst" << "deficit\n";

    for (const auto &cell : cells) {
        const int n = cell.n, k = cell.k, ca = cell.ca, cb = cell.cb;
        const int m = n - k;
        const int R = ca + cb;
        const int L = ceil_log2(R);
        const int dk = L - k;
        const int dm = L - m;

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

        const std::int64_t delta = rhs(n, k, ca) + rhs(n, k, cb) - rhs(n, k, R);

        auto tightA = tight_fibers(n, k, ca, N, nbrMask, words);
        auto tightB =
            (ca == cb) ? tightA : tight_fibers(n, k, cb, N, nbrMask, words);

        std::int64_t worst = std::numeric_limits<std::int64_t>::min();
        for (std::size_t i = 0; i < tightA.size(); ++i) {
            const auto &[membersA, ebA] = tightA[i];
            for (std::size_t j = 0; j < tightB.size(); ++j) {
                if (ca == cb && j <= i)
                    continue;
                const auto &[membersB, ebB] = tightB[j];
                if (membersA.intersects(membersB))
                    continue;
                const int shared = popcountAnd(ebA, ebB);
                const int b_ba = popcountAnd(ebA, membersB);
                const int b_ab = popcountAnd(ebB, membersA);
                const std::int64_t margin = shared + b_ba + b_ab - delta;
                if (margin > worst)
                    worst = margin;
            }
        }

        std::cout << std::left << std::setw(4) << n << std::setw(4) << k
                  << std::setw(5) << ca << std::setw(5) << cb << std::setw(5)
                  << R << std::setw(6) << m << std::setw(8) << L << std::setw(5)
                  << dk << std::setw(5) << dm << std::setw(8) << delta;
        if (worst == std::numeric_limits<std::int64_t>::min())
            std::cout << std::setw(8) << "n/a"
                      << "n/a (no disjoint tight "
                         "pairs found)\n";
        else
            std::cout << std::setw(8) << worst << -worst << "\n";
    }
    return 0;
}
