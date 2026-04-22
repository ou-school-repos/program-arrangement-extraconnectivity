#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

// Extraconnectivity of Arrangement Graphs — brute force search
// (Cheng et al.)
//
// We use R as the number of positions in each vertex (permutation length).
// K >= R; WLOG the last K-R symbols are identical across all vertices.

static constexpr int R = 5;

static std::string ver[R];

// Results: for each distinct nk1 value, track the best cons and an example.
static std::vector<int> nk1ans;
static std::vector<int> consans;
static std::vector<std::string> ex;

// ── Neighbor-set calculation ────────────────────────────────────────────────
// Returns {nk1coef, cons} for the current vertex set.
static std::pair<int, int> calc() {
    int nk1coef = 0;
    int cons = 0;

    for (int i = 1; i < R; i++) {
        const std::string &cur = ver[i];
        std::vector<std::string> dcverts;
        std::vector<int> chgs;

        bool isShared[R] = {};
        int isSharednum = 0;

        for (int j = 0; j < i; j++) {
            const std::string &cur2 = ver[j];
            int differs = 0, diff1 = 0, diff2 = 0;

            for (int k = 0; k < R; k++) {
                if (cur[k] != cur2[k]) {
                    if (differs == 0) {
                        diff1 = k;
                        differs++;
                    } else if (differs == 1) {
                        diff2 = k;
                        differs++;
                    } else {
                        differs++;
                        break;
                    }
                }
            }

            if (differs == 1) {
                if (!isShared[diff1]) {
                    isShared[diff1] = true;
                    isSharednum++;
                    nk1coef++;
                }
            }

            if (differs == 2) {
                if (diff1 > diff2)
                    std::swap(diff1, diff2);

                if (cur[diff1] != cur2[diff2]) {
                    std::string v = cur;
                    v[diff1] = cur2[diff1];
                    if (std::find(dcverts.begin(), dcverts.end(), v) ==
                        dcverts.end()) {
                        dcverts.push_back(v);
                        chgs.push_back(diff1);
                    }
                }
                if (cur[diff2] != cur2[diff1]) {
                    std::string v = cur;
                    v[diff2] = cur2[diff2];
                    if (std::find(dcverts.begin(), dcverts.end(), v) ==
                        dcverts.end()) {
                        dcverts.push_back(v);
                        chgs.push_back(diff2);
                    }
                }
            }
        }

        // Prune vertices whose changed position is shared or whose
        // changed character appears again later in the same vertex.
        for (int n = 0; n < static_cast<int>(chgs.size()); n++) {
            if (isShared[chgs[n]]) {
                chgs.erase(chgs.begin() + n);
                dcverts.erase(dcverts.begin() + n);
                n--;
            } else {
                const std::string &dv = dcverts[n];
                int pos = chgs[n];
                char ch = dv[pos];
                // Check if same char appears at a later position in dv.
                bool dup = false;
                for (int p = pos + 1; p < R; p++) {
                    if (dv[p] == ch) {
                        dup = true;
                        break;
                    }
                }
                if (dup) {
                    chgs.erase(chgs.begin() + n);
                    dcverts.erase(dcverts.begin() + n);
                    n--;
                }
            }
        }

        // (Original paper has an empty loop at i==2 — preserved for fidelity.)
        // if (i == 2) { for (int n = 0; n < 3; n++) {} }

        cons += static_cast<int>(dcverts.size());
        cons = (cons - isSharednum) + 1;
    }

    return {nk1coef, cons};
}

// ── Recursive brute-force search ────────────────────────────────────────────
// point  — index of vertex we're choosing next
// nodl   — number of distinct letters used so far
// largchg — largest position index that differs from the canonical ABCDE…
static void solve(int point, int nodl, int largchg) {
    if (point != R) {
        // Collect existing vertices into a set for O(1) dedup.
        std::unordered_set<std::string> seen;
        for (int i = 0; i < point; i++) {
            seen.insert(ver[i]);
        }

        for (int i = 0; i < point; i++) {
            for (int j = 0; j <= nodl; j++) {
                char c = static_cast<char>('A' + j);
                if (ver[i].find(c) != std::string::npos)
                    continue;

                for (int k = 0; k <= largchg + 1; k++) {
                    std::string temp = ver[i];
                    temp[k] = c;
                    if (seen.find(temp) == seen.end()) {
                        seen.insert(temp);
                        ver[point] = temp;
                        solve(point + 1, std::max(nodl, j + 1),
                              std::max(largchg, k));
                        seen.erase(temp);
                    }
                }
            }
        }
    } else {
        auto [nk1, cons] = calc();

        // Check if we already have a result with this nk1 value.
        auto it = std::find(nk1ans.begin(), nk1ans.end(), nk1);
        if (it == nk1ans.end()) {
            nk1ans.push_back(nk1);
            consans.push_back(cons);
            std::string exa;
            for (int i = 0; i < R; i++)
                exa += ver[i] + " ";
            ex.push_back(exa);
        } else {
            int poi = static_cast<int>(it - nk1ans.begin());
            if (consans[poi] < cons) {
                consans[poi] = cons;
                std::string exa;
                for (int i = 0; i < R; i++)
                    exa += ver[i] + " ";
                ex[poi] = exa;
            }
        }
    }
}

// ── Main ────────────────────────────────────────────────────────────────────
int main() {
    // Build the two initial vertices:
    //   ver[0] = "ABCDE"  (canonical)
    //   ver[1] = "FBCDE"  (first symbol replaced by symbol R, i.e. 'F')
    std::string a, b;
    a += static_cast<char>('A' + 0);
    b += static_cast<char>('A' + R);
    for (int i = 1; i < R; i++) {
        a += static_cast<char>('A' + i);
        b += static_cast<char>('A' + i);
    }
    ver[0] = a;
    ver[1] = b;

    solve(2, R + 1, 0);

    for (size_t i = 0; i < nk1ans.size(); i++) {
        std::cout << "(" << R << "nk-" << nk1ans[i] << ") (n-k)-"
                  << (nk1ans[i] + consans[i]) << ", EX: " << ex[i] << "\n";
    }

    return 0;
}
