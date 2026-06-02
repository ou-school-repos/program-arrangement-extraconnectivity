# Axiom Equivalence: The Duality of lower_bound_all_embeddings and hamming_ball_eval

## The Two Axioms

### Axiom 1: `lower_bound_all_embeddings` (Lower Bound Engine)

```lean
axiom lower_bound_all_embeddings (R n k : ℕ) (V' : Finset (ArrVertex n k)) (hR : V'.card = R) (hnk : k ≤ n) :
    external_neighbors V' ≥ (R * k - E_seq R) * (n - k) - C_constant R
```

**Role:** Supplies the _universally quantified lower bound_. For **every**
R-vertex subset V', the external boundary is bounded below by the predicted boundary.

### Axiom 2: `hamming_ball_eval` (Upper Bound Witness)

```lean
axiom hamming_ball_eval {R n k d : ℕ} (hk : d ≤ k) (hnk : k + d ≤ n)
    (hd : d = bit_length (R - 1)) :
    external_neighbors (hamming_ball_subset R n k d hk hnk) =
      (R * k - E_seq R) * (n - k) - C_constant R
```

**Role:** Supplies the _existential upper bound_. There **exists** a specific
subset (the Hamming Ball) that achieves the formula exactly.

## The Mathematical Correction and Duality

In previous versions of the framework, Axiom 1 was formulated as a dimension-independent bound on the combined waste: `cross_collisions V' + (R * k - sum_unique_roots V') ≤ C_constant R`. However, this statement is **provably false** for sub-optimal topologies.

### The Star Graph Counterexample

For the Star Graph $K_{1, R-1}$ at $R=8$ in $A(15, 7)$ (where $n-k = 8$), the defect is $D = 7$ (shortfall of 5 from the optimal $E(8) = 12$). Since the star graph leaves are placed along coordinates with distinct symbols, they can share an external neighbor across every pair, yielding:
$$X = \binom{7}{2} = 21 \text{ cross-collisions}$$
$$X + D = 21 + 7 = 28$$
But $C_{constant}(8) = 12$. Since $28 \not\le 12$, the combined waste bound is violated.

### Why the Hamming Ball Still Wins ("Tug-of-War" Scaling)

Although the Star Graph can achieve higher collision savings, it requires a larger $n-k$ dimension factor. The penalty for missing 5 defect links is:
$$\Delta D \cdot (n-k) = 5 \cdot 7 = 35$$
This linear dimensional penalty of 35 easily outpaces the 16 additional cross-collisions ($28 - 12 = 16$). As $n-k \to \infty$, the dimensional penalty completely crushes any non-standard collision savings.

Therefore, the true universal bound must be stated as the final boundary inequality directly.

Together, these two axioms form the "sandwich" that pins the isoperimetric profile to a single value:

```
∀ V', |N(V')| ≥ formula(R)      ← from lower_bound_all_embeddings
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
  │        ├──► lower_bound_all_embeddings
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

Both axioms have been verified computationally at two independent levels:

### Level 1: Formula Engine (`predict.cpp`)

The predictor computes (for any R):

- `E_seq(R) = A000788(R)` — the coefficient (cumulative binary weight)
- `C_constant(R) = (R-1) + Σ bit_length(x) - E_seq(R)` — the collision constant
- `formula(R) = (R·k - E_seq(R))·(n-k) - C_constant(R)` — the predicted boundary

The formula is cross-validated against brute-force neighbor enumeration for
small R, confirming exact agreement.

### Level 2: Exhaustive Topology Search (`arrangement.cpp`)

For small R, the search engine:

1. Generates ALL R-vertex subsets of A(n,k) up to `nauty` automorphism
2. Computes `external_neighbors(V')` for each canonical representative
3. Confirms the **minimum** equals the formula value
4. Records the **unique** minimizer topology (Hamming Ball)

This provides independent verification that:

- No R-vertex subset has fewer external neighbors than the formula (validates Axiom 1)
- The Hamming Ball subset achieves exactly the formula value (validates Axiom 2)

## C_constant(R) Values

| R   | E_seq(R) | C_constant(R) | Meaning                                     |
| --- | -------- | ------------- | ------------------------------------------- |
| 2   | 1        | 0             | No collisions possible (2 vertices)         |
| 3   | 2        | 0             | No collisions (triangle has none in A(n,k)) |
| 4   | 4        | 1             | First collision at the 4-cycle (square)     |
| 5   | 5        | 1             | Same square, one extra leaf                 |
| 6   | 7        | 2             | Two independent squares                     |
| 7   | 8        | 2             | Two squares, one extra leaf                 |
| 8   | 11       | 4             | Three-dimensional cube: four squares        |

The pattern: C_constant(R) counts the maximum number of "shielding 4-cycles"
the Hamming Ball creates at size R. Each 4-cycle eliminates one external
neighbor that would otherwise be counted twice.

## Why They Cannot Be Merged

Although both axioms follow from KK and Tug-of-War scaling, they serve structurally different roles
in the proof:

1. **`lower_bound_all_embeddings`** is a ∀-statement over all V'.
   It provides the universal lower bound.

2. **`hamming_ball_eval`** is an ∃-statement about a specific V\*.
   It flows into `exists_optimal_embedding`.

The capstone theorem (`arrangement_extraconnectivity_minimum`) combines both
via a conjunction: `⟨exists_optimal_embedding, lower_bound_all_embeddings⟩`.

Merging them into a single axiom would obscure the proof architecture and
lose the clean separation between the universal bound and the constructive
witness.

## Formalization Path (if pursued)

Both axioms would be closed by a single formalization effort:

1. **Define sequence compression** on `ArrVertex n k` (~50 lines)
2. **Prove compression preserves cardinality** (~100 lines)
3. **Prove compression does not increase boundary** (~200 lines)
4. **Prove convergence to Hamming Ball** (~100 lines)
5. **Evaluate C_constant at the Hamming Ball** (~100 lines)

Total: ~550 lines of Lean 4, requiring Kruskal-Katona shadow operators
that are not yet in Mathlib for sequence (ordered, injective) families.

See [collision-axiom-roadmap.md](collision-axiom-roadmap.md) for the
detailed step-by-step plan.
