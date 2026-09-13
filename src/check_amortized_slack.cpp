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

bool testBit(const Bits &b, int i) {
    return (b.w[i / WBITS] & (Word{1} << (i % WBITS))) != 0;
}

// T: among the shared external targets (ebA ∩ ebB), count those NOT
// explained by a distance-1 cross pair -- i.e. a shared target w such
// that no neighbor-of-w-in-Fa is directly adjacent to a
// neighbor-of-w-in-Fb. Those are the "distance-2 channel" targets
// (docs/proof-sketch-weighted-potential.md, subcube-intersection
// attack, corrected constant-term identity T+(B_ab+B_ba)=ΔE+ΔC).
// X: the number of direct cross-edges (u,v), u in Fa, v in Fb,
// adjacent. Computed by summing, over u in Fa, |nbrMask[u] ∩ Fb| --
// distinct from B_ba/B_ab, which count touching *vertices*, not edges.
int compute_X(const Bits &membersA, const Bits &membersB,
              const std::vector<Bits> &nbrMask, int N) {
    int X = 0;
    for (int u = 0; u < N; ++u) {
        if (!testBit(membersA, u))
            continue;
        X += popcountAnd(nbrMask[u], membersB);
    }
    return X;
}

int compute_T(const Bits &ebA, const Bits &ebB, const Bits &membersA,
              const Bits &membersB, const std::vector<Bits> &nbrMask, int N,
              std::size_t words) {
    Bits shared(words);
    for (std::size_t i = 0; i < words; ++i)
        shared.w[i] = ebA.w[i] & ebB.w[i];

    int T = 0;
    for (int w = 0; w < N; ++w) {
        if (!testBit(shared, w))
            continue;
        Bits Na(words), Nb(words);
        for (std::size_t i = 0; i < words; ++i) {
            Na.w[i] = nbrMask[w].w[i] & membersA.w[i];
            Nb.w[i] = nbrMask[w].w[i] & membersB.w[i];
        }
        bool dist1 = false;
        for (int a = 0; a < N && !dist1; ++a) {
            if (!testBit(Na, a))
                continue;
            if (nbrMask[a].intersects(Nb))
                dist1 = true;
        }
        if (!dist1)
            ++T;
    }
    return T;
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
    const std::int64_t dE = e_seq(ca + cb) - e_seq(ca) - e_seq(cb);
    const std::int64_t dC =
        c_constant(ca + cb) - c_constant(ca) - c_constant(cb);
    std::cerr << "A(" << n << "," << k << ") c_a=" << ca << " c_b=" << cb
              << " (N=" << N << " vertices): Delta=" << delta << " (dE=" << dE
              << " dC=" << dC << ", dE+dC=" << (dE + dC) << ")\n";

    auto tightA = tight_fibers(n, k, ca, N, nbrMask, words);
    auto tightB =
        (ca == cb) ? tightA : tight_fibers(n, k, cb, N, nbrMask, words);

    std::cerr << "  #tight_a=" << tightA.size() << " #tight_b=" << tightB.size()
              << "\n";

    std::int64_t worst = std::numeric_limits<std::int64_t>::min();
    std::size_t checked = 0;
    std::size_t adjacent_pairs = 0;
    std::size_t margin_zero_pairs = 0;
    std::size_t worst_margin_pairs = 0;
    std::size_t identity_checked = 0;
    std::size_t identity_holds = 0;
    std::size_t identity_first_violation_reported = 0;
    std::size_t x_checked = 0;
    std::size_t x_holds = 0;
    std::size_t d1_holds = 0;
    std::size_t mechanism_first_violation_reported = 0;
    // Histogram of (T, B_ab+B_ba) over margin=0 pairs: is the split
    // constant across all margin=0 configurations, or does it vary
    // while the sum T+(B_ab+B_ba)=dE+dC stays fixed? See advisor
    // review, docs/proof-sketch-weighted-potential.md
    // subcube-intersection attack -- a varying split is itself a
    // structural signal, not just noise.
    std::map<std::pair<int, int>, std::size_t> tb_histogram;
    // First witness pair index (i,j into tightA/tightB) for each
    // distinct (T, B_ab+B_ba) bucket, so a specific pair can be
    // hand-traced afterward.
    std::map<std::pair<int, int>, std::pair<std::size_t, std::size_t>>
        tb_witness;
    const int m = n - k;
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
            if (margin > worst) {
                // A new best (least-deficient) margin displaces the old
                // one -- the (T,B) histogram/witnesses track the *worst*
                // (i.e. margin-maximizing) pairs only, so they must reset
                // whenever that bucket moves. See advisor review,
                // docs/proof-sketch-weighted-potential.md
                // subcube-intersection attack, non-embeddable-regime
                // extension.
                worst = margin;
                tb_histogram.clear();
                tb_witness.clear();
                worst_margin_pairs = 0;
            }
            ++checked;

            // Only margin=0 (critical-case-tight) pairs are relevant to
            // the T+(B_ab+B_ba)=dE+dC identity -- that identity is
            // specific to the exactly-tight/embeddable regime and is
            // tautologically false whenever margin!=0, so it stays
            // gated here rather than moving to the dynamic worst-margin
            // bucket below.
            if (margin == 0) {
                ++margin_zero_pairs;
                const int T =
                    compute_T(ebA, ebB, membersA, membersB, nbrMask, N, words);
                const std::int64_t lhs =
                    static_cast<std::int64_t>(T) + b_ab + b_ba;
                const std::int64_t rhs_val = dE + dC;
                ++identity_checked;
                if (lhs == rhs_val) {
                    ++identity_holds;
                } else if (identity_first_violation_reported < 5) {
                    ++identity_first_violation_reported;
                    std::cerr << "  identity mismatch: T=" << T
                              << " b_ab=" << b_ab << " b_ba=" << b_ba
                              << " lhs=" << lhs << " dE+dC=" << rhs_val << "\n";
                }

                // Sharper (non-tautological) mechanism check: does the
                // cross-edge count actually equal dE, and do those edges'
                // (m-1) fresh-symbol targets actually account for D1
                // cleanly (no collisions), rather than the net T+B
                // identity above merely balancing via compensating
                // errors? See advisor review, docs/proof-sketch-
                // weighted-potential.md subcube-intersection attack.
                const int X = compute_X(membersA, membersB, nbrMask, N);
                const int D1 = shared - T; // I - T, the distance-1-
                                           // attributed share of I
                ++x_checked;
                const bool x_ok = (static_cast<std::int64_t>(X) == dE);
                const bool d1_ok = (static_cast<std::int64_t>(D1) ==
                                    dE * static_cast<std::int64_t>(m - 1));
                if (x_ok)
                    ++x_holds;
                if (d1_ok)
                    ++d1_holds;
                if ((!x_ok || !d1_ok) &&
                    mechanism_first_violation_reported < 5) {
                    ++mechanism_first_violation_reported;
                    std::cerr << "  mechanism mismatch: X=" << X << " dE=" << dE
                              << " D1=" << D1 << " dE*(m-1)=" << (dE * (m - 1))
                              << "\n";
                }
            }

            // (T, B_ab+B_ba) tracking for the dynamic worst-margin
            // bucket: unlike the identity/mechanism checks above, T
            // itself is meaningful for any margin (it's just a count of
            // distance-2-only shared targets), so this runs whenever the
            // pair matches the current worst margin, embeddable or not.
            if (margin == worst) {
                ++worst_margin_pairs;
                const int T =
                    compute_T(ebA, ebB, membersA, membersB, nbrMask, N, words);
                const auto key = std::make_pair(T, b_ab + b_ba);
                ++tb_histogram[key];
                tb_witness.emplace(key, std::make_pair(i, j));
            }
        }
        if ((i + 1) % 200 == 0) {
            auto elapsed = std::chrono::duration<double>(
                               std::chrono::steady_clock::now() - t0)
                               .count();
            std::cerr << "  ... " << (i + 1) << "/" << tightA.size()
                      << " tight-A fibers done, " << checked
                      << " pairs checked, running max(I+B_ba+B_ab-Delta)="
                      << worst << ", identity holds " << identity_holds << "/"
                      << identity_checked << " (" << elapsed << "s elapsed)\n";
        }
    }

    std::cout << "A(" << n << "," << k << ") c_a=" << ca << " c_b=" << cb
              << ": checked " << checked << " disjoint tight-fiber pairs ("
              << adjacent_pairs
              << " directly Fa-Fb adjacent); max(I+B_ba+B_ab-Delta) = " << worst
              << "\n";
    std::cout << "  margin=0 pairs: " << margin_zero_pairs
              << "; T+(B_ab+B_ba)=dE+dC identity holds " << identity_holds
              << "/" << identity_checked << "\n";
    std::cout << "  mechanism checks (m=" << m << "): X=dE holds " << x_holds
              << "/" << x_checked << "; D1=dE*(m-1) holds " << d1_holds << "/"
              << x_checked << "\n";
    if (worst > 0)
        std::cout << "*** CRITICAL-CASE VIOLATION (I+B_ba+B_ab >= Delta) "
                     "***\n";
    if (identity_checked > 0 && identity_holds != identity_checked)
        std::cout << "*** T+(B_ab+B_ba)=dE+dC IDENTITY VIOLATED on "
                  << (identity_checked - identity_holds) << " pair(s) ***\n";
    if (x_checked > 0 && (x_holds != x_checked || d1_holds != x_checked))
        std::cout << "*** MECHANISM CHECK FAILED (X!=dE or D1!=dE*(m-1)) "
                     "on some pair(s) -- net identity may be masking "
                     "compensating errors ***\n";

    // (T, B_ab+B_ba) distribution over the worst-margin pairs (margin=0
    // in the embeddable regime; the least-negative achievable margin
    // otherwise): is the split constant, or does it vary?
    if (!tb_histogram.empty()) {
        std::cout << "  (T, B_ab+B_ba) distribution over " << worst_margin_pairs
                  << " worst-margin (=" << worst << ") pairs:\n";
        for (const auto &[key, count] : tb_histogram) {
            std::cout << "    T=" << key.first << " B_ab+B_ba=" << key.second
                      << " (sum=" << (key.first + key.second) << "): " << count
                      << " pair(s)";
            auto wit = tb_witness.at(key);
            const auto &[wMembersA, wEbA] = tightA[wit.first];
            const auto &[wMembersB, wEbB] = tightB[wit.second];
            std::cout << "  -- witness Fa={";
            for (int id = 0; id < N; ++id) {
                if (!testBit(wMembersA, id))
                    continue;
                for (int x : verts[id])
                    std::cout << x;
                std::cout << " ";
            }
            std::cout << "} Fb={";
            for (int id = 0; id < N; ++id) {
                if (!testBit(wMembersB, id))
                    continue;
                for (int x : verts[id])
                    std::cout << x;
                std::cout << " ";
            }
            std::cout << "}\n";
        }
        if (tb_histogram.size() > 1)
            std::cout << "  *** (T,B) SPLIT VARIES across worst-margin pairs "
                         "-- not a fixed geometric split ***\n";
    }
    return 0;
}
