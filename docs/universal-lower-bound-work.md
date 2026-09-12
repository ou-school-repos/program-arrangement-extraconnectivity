# UniversalLowerBound: Math-First Work

## Exact Lean Target (ArrangementExtraconnectivity.lean:784–786)

```
∀ V' ⊆ A(n,k), |V'| = R, k ≤ n ⟹
  external_neighbors V' ≥ (R·k − E_seq(R))·(n−k) − C_constant(R)
```

**No connectedness hypothesis.** The Lean statement quantifies over all R-element subsets.

## Definitions

- `E_seq(R)`: `E_seq 0 = 0`; `E_seq (n+1) = E_seq n + popcount n` (A000788)
- `C_constant(R)`: `(R - 1) + sum_bit_length R - E_seq R`
- `external_neighbors V'`: total external boundary = union of coordinate boundaries minus V'
- `coord_boundary V' p`: vertices adjacent to V' at coordinate p only

## Deliverables (in order)

1. **Restate** all definitions and quantifiers precisely from Lean source
2. **Stress-test** computationally — emphasize disconnected/sparse subsets and small slack \(n-k \in \{1,2\}\). A counterexample is a success: report \(((n,k,R,V'),\) its boundary, claimed lower bound)
3. **Proof sketch** for the exact all-subsets statement; identify the lemma replacing the refuted support-projection inequality
4. **Recommendation**: should the Lean statement/paper proposition remain all-subsets or be restricted to connected subsets?

**Do not** claim a proof, change the capstone, or begin formalization unless step 2 survives and there is a concrete proof route.
