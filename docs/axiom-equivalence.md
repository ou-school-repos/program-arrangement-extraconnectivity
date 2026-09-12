# Universal Lower Bound and Hamming-ball Evaluation

`UniversalLowerBound` remains an explicit Lean `Prop` parameter rather than a
raw `axiom` declaration. The direct `CrossTop` proof supplies the
Hamming-ball collision evaluation used by the public capstone theorem.

## The Two Hypothesis Interfaces

### Hypothesis 1: `UniversalLowerBound` (Lower Bound Engine)

```lean
def UniversalLowerBound (R n k : ℕ) : Prop :=
  ∀ (V' : Finset (ArrVertex n k)), V'.card = R → k ≤ n →
    external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R
```

**Role:** Supplies the _universally quantified lower bound_. For **every**
R-vertex subset V', the external boundary is bounded below by the predicted
boundary. Still unproven in Lean (no formalization strategy currently active;
see `docs/collision-axiom-roadmap.md`).

### Lemma: `hamming_ball_eval` (Upper Bound Witness)

```lean
lemma hamming_ball_eval {R n k d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (hd : d = bit_length (R - 1)) (h_cross : HBCrossCollisions R n k d hk hnk) :
    external_neighbors (hamming_ball_subset R n k d hk hnk) =
      (R * k - E_seq R) * (n - k) - C_constant R
```

**Role:** Supplies the _existential upper bound_. There **exists** a specific
subset (the Hamming Ball) that achieves the formula exactly. This lemma is
**proven** — the public capstone supplies the Hamming-ball collision evaluation
from `hb_cross_collisions_closed`; only the universal lower-bound hypothesis
remains external.

## The Mathematical Correction and Duality

In previous versions of the framework, `UniversalLowerBound` was formulated as a dimension-independent bound on the combined waste: `cross_collisions V' + (R * k - sum_unique_roots V') ≤ C_constant R`. However, this statement is **provably false** for sub-optimal topologies.
TODO(review): keep this historical note, but do not let later prose drift back
into the same combined-waste formulation.

### The Star Graph Counterexample

For the Star Graph $K_{1, R-1}$ at $R=8$ in $A(15, 7)$ (where $n-k = 8$), the defect is $D = 7$ (shortfall of 5 from the optimal $E(8) = 12$). Since the star graph leaves are placed along coordinates with distinct symbols, they can share an external neighbor across every pair, yielding:
$$X = \binom{7}{2} = 21 \text{ cross-collisions}$$
$$X + D = 21 + 7 = 28$$
But $C_{constant}(8) = 12$. Since $28 \not\le 12$, the combined waste bound is violated.
TODO(review): this counterexample is the reason the support-projection and
collision-adjusted bridge should stay marked as abandoned until repaired.

### Why the Hamming Ball Still Wins ("Tug-of-War" Scaling)

Although the Star Graph can achieve higher collision savings, it requires a larger $n-k$ dimension factor. The penalty for missing 5 defect links is:
$$\Delta D \cdot (n-k) = 5 \cdot 7 = 35$$
This linear dimensional penalty of 35 easily outpaces the 16 additional cross-collisions ($28 - 12 = 16$). As $n-k \to \infty$, the dimensional penalty completely crushes any non-standard collision savings.

Therefore, the true universal bound must be stated as the final boundary inequality directly.

Together, these two hypothesis interfaces form the "sandwich" that pins the isoperimetric profile to a single value:

```
∀ V', |N(V')| ≥ formula(R)      ← from UniversalLowerBound
∃ V*, |N(V*)| = formula(R)      ← from hamming_ball_eval
────────────────────────────────
∴ min_{|V'|=R} |N(V')| = formula(R)
```

### The Information Flow

```
Kruskal-Katona Shadow Theorem + Tug-of-War Scaling
  │
  ├──► "Hamming Ball maximizes internal shielding/minimizes boundary"
  │        │
  │        ├──► UniversalLowerBound
  │        │      (boundary ≥ HB boundary)
  │        │
  │        └──► hamming_ball_eval
  │               (HB achieves exact formula)
  │
  └──► Both follow from: initial colex segment
       minimizes shadow, which maximizes overlap,
       which maximizes collisions.
```

## Computational Verification

The Hamming-ball formula has finite computational consistency checks at two
independent levels. These checks provide bounded evidence only; they do not
establish the universally quantified lower bound.

### Level 1: Formula Engine (`predict.cpp`)

The predictor computes (for any R):

- `E_seq(R) = A000788(R)` — the coefficient (cumulative binary weight)
- `C_constant(R) = (R-1) + Σ bit_length(x) - E_seq(R)` — the collision constant
- `formula(R) = (R·k - E_seq(R))·(n-k) - C_constant(R)` — the predicted boundary

The formula is cross-validated against brute-force neighbor enumeration over
the recorded finite range, confirming exact agreement on those instances.

### Level 2: Exhaustive Topology Search (`arrangement.cpp`)

For small R, the search engine:

1. Generates ALL R-vertex subsets of A(n,k) up to `nauty` automorphism
2. Computes `external_neighbors(V')` for each canonical representative
3. Confirms the **minimum** equals the formula value
4. Records the **unique** minimizer topology (Hamming Ball)

This provides bounded finite-instance evidence that:

- No enumerated R-vertex subset has fewer external neighbors than the formula.
- The enumerated Hamming Ball subsets achieve the formula value; this is a
  finite check of `hamming_ball_eval`, not a replacement for its Lean proof.

## C_constant(R) and cross_collisions(HB(R)) Values

`C_constant(R)` is defined in Lean as `(R - 1) + sum_bit_length R - E_seq R`.
The Hamming Ball's cross-collision count is `cross_collisions(HB(R)) = C_constant(R) - E_seq(R)`.

| R   | E_seq(R) | C_constant(R) | cross_collisions(HB(R)) | Meaning                                     |
| --- | -------- | ------------- | ----------------------- | ------------------------------------------- |
| 2   | 1        | 1             | 0                       | No shielding cycles (two adjacent vertices) |
| 3   | 2        | 3             | 1                       | One external rectangle                      |
| 4   | 4        | 4             | 0                       | Full Q₂: all boundary shared internally     |
| 5   | 5        | 7             | 2                       | One external rectangle from the extra leaf  |
| 6   | 7        | 9             | 2                       | Two independent external rectangles         |
| 7   | 9        | 11            | 2                       | Two rectangles plus one extra leaf          |
| 8   | 12       | 12            | 0                       | Full Q₃: all boundary shared internally     |

**Note:** Powers of two (`R = 2^d`) always have `cross_collisions(HB(R)) = 0`,
because the full hypercube `Q_d` has no external rectangles — every
distance-2 pair in `Q_d` has both completions inside `Q_d`.

The pattern: `C_constant(R)` equals `E_seq(R)` at each power of two.
Between powers, `cross_collisions(HB(R)) = C_constant(R) - E_seq(R) ≥ 0`
counts the shielding 4-cycles: each eliminates one external neighbor
that would otherwise be counted twice.

## Why They Cannot Be Merged

Although a future proof may connect both hypothesis interfaces to KK and Tug-of-War scaling, they serve structurally different roles
in the proof:

1. **`UniversalLowerBound`** is a ∀-statement over all V'.
   It provides the universal lower bound.

2. **`hamming_ball_eval`** is an ∃-statement about a specific V\*.
   It flows into `exists_optimal_embedding`.

The public capstone theorem takes `h_lower : ∀ R n k, UniversalLowerBound R n k`
and supplies the Hamming-ball evaluation internally. Its internal
`..._of_cross` composition lemma remains available when a caller already has a
fixed-size collision evaluation.

Merging them into a single hypothesis would obscure the proof architecture and
lose the clean separation between the universal bound and the constructive
witness.

## Formalization Path (if pursued)

The remaining live formalization effort is `UniversalLowerBound`. An earlier
attempt via support-projection into the Boolean hypercube (to reuse
Mathlib's `Mathlib.Combinatorics.SetFamily.KruskalKatona`, which does
exist in Mathlib) was abandoned after a counterexample refuted its central
inequality; see
`docs/archive/collision-axiom-support-projection-abandoned.md`. A working
approach would need, at minimum, a corrected shadow-count inequality, a
proven colex correspondence, and a proven pullback to
`sum_unique_roots`/`external_neighbors` — none of which currently exist.

See [collision-axiom-roadmap.md](collision-axiom-roadmap.md) for the
detailed step-by-step plan.
