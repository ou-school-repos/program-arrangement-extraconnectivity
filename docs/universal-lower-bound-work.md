# UniversalLowerBound: Math-First Work

## Exact Lean Target (ArrangementExtraconnectivity.lean:784–786)

```text
∀ V' ⊆ A(n,k), |V'| = R, k ≤ n ⟹
  external_neighbors V' ≥ (R·k − E_seq(R))·(n−k) − C_constant(R)
```

**No connectedness hypothesis.** The Lean statement quantifies over all
R-element subsets.

## Definitions

- `E_seq(R)`: `E_seq 0 = 0`; `E_seq (n+1) = E_seq n + popcount n` (A000788)
- `C_constant(R)`: `(R - 1) + sum_bit_length R - E_seq R`
- `external_neighbors V'`: total external boundary = union of coordinate
  boundaries minus V'
- `coord_boundary V' p`: vertices adjacent to V' at coordinate p only

## Deliverables (in order)

1. **Restate** all definitions and quantifiers precisely from Lean source
2. **Stress-test** computationally — emphasize disconnected/sparse subsets and
   small slack $n-k \in \{1,2\}$. A counterexample is a success: report
   $((n,k,R,V'),$ its boundary, claimed lower bound)
3. **Proof sketch** for the exact all-subsets statement; identify the lemma
   replacing the refuted support-projection inequality
4. **Recommendation**: should the Lean statement/paper proposition remain
   all-subsets or be restricted to connected subsets?

**Do not** claim a proof, change the capstone, or begin formalization unless
step 2 survives and there is a concrete proof route.

## First Math Audit (2026-09-12)

### What existing computation does _not_ establish

`predict.cpp` evaluates the constructed Hamming ball. `arrangement.cpp` starts
from an adjacent pair and grows connected configurations. Neither program
exhaustively tests the all-subsets quantifier in `UniversalLowerBound`.

`scripts/check_universal_lower_bound.py` was added specifically for this
purpose. It enumerates literal subsets of the finite vertex set, computes the
Lean definition of `external_neighbors`, and uses the natural-number-truncated
right-hand side of the proposition. `src/universal_lower_bound.cpp` provides a
separate C++ implementation of the same exhaustive fiber calculation for larger
bounded sweeps; small rows must agree with the Python oracle before its output
is treated as evidence.

### Exhaustive small-slack checks

No counterexample was found in the following exhaustive tests:

- every subset of $A(3,2)$ and $A(4,2)$;
- every $R$-set of $A(4,3)$ for $R\le6$;
- every $R$-set of $A(5,3)$ for $R\le4$, including $\binom{60}{4}=487{,}635$
  four-sets;
- all $280{,}840$ three-sets of $A(5,4)$, all $64{,}620$ pairs of $A(6,4)$, and
  all $258{,}840$ pairs of $A(6,5)$.

In every tested row, a boundary minimizer was connected; the best disconnected
set had no smaller boundary. This is evidence only, not a reduction from all
subsets to connected subsets.

### Corrected candidate lemma

Write $m=n-k$, $D(V)=Rk-\mathrm{sum_unique_roots}(V)$, and
$X(V)=\mathrm{cross_collisions}(V)$. The proved defect theorem gives
$D(V)\le E(R)$, and the proved fiber identity gives

$$
|\partial V|=(Rk-D(V))m-D(V)-X(V).
$$

Thus, whenever the stated lower-bound right-hand side is positive, the target is
equivalent to the following **deficit-compensated collision inequality**:

$$
X(V)\le\bigl(C(R)-E(R)\bigr)+(m+1)\bigl(E(R)-D(V)\bigr).
$$

For the Hamming ball this is equality, since $D(\mathrm{HB}_R)=E(R)$ and
$X(\mathrm{HB}_R)=C(R)-E(R)$. This is strictly weaker than the refuted
dimension-independent inequality $X(V)+D(V)\le C(R)$. It is a candidate for the
missing mathematical lemma, not an established claim; the next task is to seek
either a proof or a counterexample to it.

## Second Math Audit (2026-09-12)

The exhaustive checker now computes $D(V)$ and $X(V)$ from the same
coordinate-root fibers as the Lean definitions. In particular, it does **not**
mistake the number of outward graph edges for `total_coord_edges`: several
vertices in one fiber contribute only once to a coordinate boundary.

For every prior exhaustive case, and additionally all $\binom{30}{5}=142{,}506$
five-sets of $A(6,2)$ and all $\binom{42}{r}$ subsets of $A(7,2)$ for $r=3,4,5$,
both the stated boundary inequality and the compensated inequality passed
whenever the Lean target's right-hand side was positive. In the new $A(7,2)$
rows, the minimum boundary was connected, while the best disconnected boundary
was strictly larger. No counterexample is presently known from this search. The
reproducible commands are
`python3 scripts/check_universal_lower_bound.py --profile small` and
`python3 scripts/check_universal_lower_bound.py --profile extended`. This
remains finite evidence, not a proof.

The C++ cross-check is built with `make bin/universal_lower_bound` and invoked
as, for example, `bin/universal_lower_bound 7 2 5`. It has a 250-million-subset
default safety limit; a deliberate larger run can supply `--max-subsets LIMIT`.
The purpose is faster exhaustive evidence, not an unbounded claim.

The candidate is most naturally stated as the following weighted extremal claim,
with $m=n-k$:

$$
X(V)+(m+1)D(V)\le C(R)+mE(R).
$$

It is algebraically identical to the displayed compensated inequality, and the
Hamming ball attains equality. This is the precise replacement sought for the
false, dimension-independent combined-waste bound. A viable proof must show that
the Hamming ball maximizes this **weighted** fiber-overlap potential; a proof
merely maximizing collisions or merely maximizing defect is insufficient.

## Third Math Audit (2026-09-12, extended sweep)

The exhaustive C++ verifier (`bin/universal_lower_bound`) now covers 30
parameter rows, including the 3,652,745,460-subset A(6,3) R=6 case, the
3,244,032,792-subset A(7,3) R=5 case, and the 61,949,040-subset A(6,5) R=3 case.
17 rows were cross-checked against the Python oracle
(`check_universal_lower_bound.py`); 10 C++-only rows exceeded the Python
1-million-subset safety cap. No counterexample was found in any row.

### Observed tightness pattern

The weighted inequality is **tight** (minimum compensated slack = 0) in every
tested row where R is small relative to the vertex set size. Tightness has been
observed in the following regimes:

| n−k | Tight rows                 | Non-tight rows                                       |
| --- | -------------------------- | ---------------------------------------------------- |
| 1   | A(3,2) R=2                 | A(3,2) R=3; A(4,3) R=3–6; A(5,4) R=3–4; A(6,5) R=2–3 |
| 2   | A(5,3) R=3–4; A(6,4) R=2–3 | A(5,3) R=5                                           |
| 3   | A(6,3) R=2–6               | —                                                    |
| 4   | A(6,2) R=2–4; A(7,3) R=2–5 | A(6,2) R=5–6                                         |
| 5   | A(7,2) R=3–4               | A(7,2) R=5–6                                         |
| 6   | A(8,2) R=2–4               | A(8,2) R=5–6                                         |

A(6,3) R=6 was subsequently run to completion with `--max-subsets 4000000000`
(C(120,6) = 3,652,745,460 subsets, ~18 minutes at ~3.3M subsets/sec):
`min=24 (connected), min_disconnected=29, min_compensated_slack=0: PASS` —
tight, so the row is now A(6,3) R=2–6, all tight.

TODO(review): the A(6,3) row's "—" non-tight entry only reflects R=2–6; R=7+ was
never tested and may still be non-tight, as seen in the analogous A(6,2) row
(n−k=4).

A(8,2) R=2, R=5, and R=6 were run to fill the previous gap:

```
A(8,2) R=2: PASS, min_compensated_slack=0 (tight)
A(8,2) R=5: PASS, min_compensated_slack=1
A(8,2) R=6: PASS, min_compensated_slack=2 (C(56,6)=32,468,436 subsets)
```

This is a new tight case at n−k=6 (R=2) and matches the pattern of growing slack
with R seen elsewhere.

A(7,3) R=5 was subsequently run to completion (3,244,032,792 subsets, ~50
minutes):
`min=33 (connected), min_disconnected=36, min_compensated_slack=0: PASS` —
tight, consistent with the rest of the n−k=4 row.

Commands used for the n−k=4 rows:

```bash
make bin/universal_lower_bound
bin/universal_lower_bound 6 2 2
bin/universal_lower_bound 6 2 3
bin/universal_lower_bound 6 2 4
bin/universal_lower_bound 6 2 5
bin/universal_lower_bound 6 2 6
bin/universal_lower_bound 7 3 2
bin/universal_lower_bound 7 3 3
bin/universal_lower_bound 7 3 4
```

A(7,3) R=5 was run with `--max-subsets 4000000000` to lift the default 250M
safety cap:

```bash
bin/universal_lower_bound 7 3 5 --max-subsets 4000000000
```

In the non-tight rows, positive slack is observed and grows with R for fixed
n−k. This pattern is consistent with the conjecture that the Hamming ball is the
unique boundary minimizer at small R, with alternative topologies becoming
competitive but never beating the bound at larger R.

**Important caveat:** Tightness means some enumerated subset achieves equality.
It does **not** establish that the Hamming ball is the unique minimizer —
several non-isomorphic subsets can attain the same boundary value. The current
checker does not classify minimizers up to arrangement-graph automorphism.

In every tested row, a boundary minimizer was connected; the best disconnected
set had no smaller boundary. This is evidence only, not a reduction from all
subsets to connected subsets.

### Current recommendation

Keep `UniversalLowerBound` quantified over **all** subsets. The new exhaustive
checks deliberately include disconnected subsets, and none has improved the
minimum boundary or violated the weighted candidate. Restricting the theorem to
connected subsets would therefore discard the desired statement without solving
the missing extremal lemma. Revisit that decision only if a genuine all-subsets
counterexample is found.

### Compression audit

For fixed $(R,n,k)$, the proved fiber identity rewrites the boundary exactly as

$$
|\partial V|=Rk(n-k)-\bigl(X(V)+(n-k+1)D(V)\bigr).
$$

Consequently, a compression is monotone for the weighted potential exactly when
it does not increase boundary. The guarded symbol compression already defined as
`compressSet` in the unstable Lean scaffold fails this test: in $A(4,2)$,
shifting $3\mathbin{\to}1$ in $\{[4,3],[1,3]\}$ produces $\{[4,1],[1,3]\}$ and
raises the boundary from $5$ to $7$. Its weighted potential therefore falls from
$3$ to $1$. This rules out a monotonicity proof for that operation, not the
weighted candidate itself.

The next proof search must use a different, coordinate-aware symmetrization or a
global arrangement-graph vertex-isoperimetric theorem. Do not resume the
ordinary symbol-compression route.
