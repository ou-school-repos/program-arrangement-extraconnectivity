# UniversalLowerBound: Math-First Work

## Exact Lean Target (ArrangementExtraconnectivity.lean:784–786)

```text
∀ V' ⊆ A(n,k), |V'| = R, k ≤ n ⟹
  external_neighbors V' ≥ (R·k − E_seq(R))·(n−k) − C_constant(R)
```

**No connectedness hypothesis.** The Lean statement quantifies over all
R-element subsets.

## Definitions

- `E_seq(R)`: `E_seq 0 = 0`; `E_seq (n+1) = E_seq n + popcount n`
  (A000788)
- `C_constant(R)`: `(R - 1) + sum_bit_length R - E_seq R`
- `external_neighbors V'`: total external boundary = union of coordinate
  boundaries minus V'
- `coord_boundary V' p`: vertices adjacent to V' at coordinate p only

## Deliverables (in order)

1. **Restate** all definitions and quantifiers precisely from Lean source
2. **Stress-test** computationally — emphasize disconnected/sparse subsets and
   small slack \(n-k \in \{1,2\}\). A counterexample is a success: report
   \(((n,k,R,V'),\) its boundary, claimed lower bound)
3. **Proof sketch** for the exact all-subsets statement; identify the lemma
   replacing the refuted support-projection inequality
4. **Recommendation**: should the Lean statement/paper proposition remain
   all-subsets or be restricted to connected subsets?

**Do not** claim a proof, change the capstone, or begin formalization unless
step 2 survives and there is a concrete proof route.

## First Math Audit (2026-09-12)

### What existing computation does _not_ establish

`predict.cpp` evaluates the constructed Hamming ball. `arrangement.cpp`
starts from an adjacent pair and grows connected configurations. Neither
program exhaustively tests the all-subsets quantifier in `UniversalLowerBound`.

`scripts/check_universal_lower_bound.py` was added specifically for this
purpose. It enumerates literal subsets of the finite vertex set, computes the
Lean definition of `external_neighbors`, and uses the natural-number-truncated
right-hand side of the proposition.

### Exhaustive small-slack checks

No counterexample was found in the following exhaustive tests:

- every subset of \(A(3,2)\) and \(A(4,2)\);
- every \(R\)-set of \(A(4,3)\) for \(R\le6\);
- every \(R\)-set of \(A(5,3)\) for \(R\le4\), including
  \(\binom{60}{4}=487{,}635\) four-sets;
- all \(280{,}840\) three-sets of \(A(5,4)\), all \(64{,}620\) pairs of
  \(A(6,4)\), and all \(258{,}840\) pairs of \(A(6,5)\).

In every tested row, a boundary minimizer was connected; the best disconnected
set had no smaller boundary. This is evidence only, not a reduction from
all subsets to connected subsets.

### Corrected candidate lemma

Write \(m=n-k\), \(D(V)=Rk-\mathrm{sum_unique_roots}(V)\), and
\(X(V)=\mathrm{cross_collisions}(V)\). The proved defect theorem gives
\(D(V)\le E(R)\), and the proved fiber identity gives

\[
|\partial V|=(Rk-D(V))m-D(V)-X(V).
\]

Thus, whenever the stated lower-bound right-hand side is positive, the target
is equivalent to the following **deficit-compensated collision inequality**:

\[
X(V)\le\bigl(C(R)-E(R)\bigr)+(m+1)\bigl(E(R)-D(V)\bigr).
\]

For the Hamming ball this is equality, since
\(D(\mathrm{HB}\_R)=E(R)\) and
\(X(\mathrm{HB}\_R)=C(R)-E(R)\). This is strictly weaker than the refuted
dimension-independent inequality \(X(V)+D(V)\le C(R)\). It is a candidate
for the missing mathematical lemma, not an established claim; the next task is
to seek either a proof or a counterexample to it.
