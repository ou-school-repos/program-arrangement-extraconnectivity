# Review Notes: ideas-08.md

## Mathematically Sound

- The OEIS A000788 observation and the hypercube edge isoperimetry argument are correct — E(d) = d · 2^{d-1} is textbook, and the Lean proof verifies it cleanly.
- The table showing the divergence at R=8 and powers of 2 is compelling.

## Things to Double-Check

1. **A000788 interpretation (§1, line 22)** — A000788 is the _cumulative_ binary weight sequence (total 1-bits in 0..n-1). The document claims this equals the maximum internal edges for an optimal subgraph of size R. This is actually **Harper's theorem** applied to the hypercube — the edge isoperimetric optimal sets are initial segments of the binary-reflected Gray code, and the internal edge count for those initial segments does follow A000788. Worth citing Harper explicitly alongside OEIS to make the connection rigorous.

2. **R=9,10 predictions (§1, table)** — The table claims A000788(9)=13 and A000788(10)=15. Verified: A000788 values are 0,1,2,4,5,7,9,12,13,15,17,20,... ✓ correct.

3. **ShardedHashSet optimization (§3)** — The idea is sound in principle (shared dedup across threads), but the claim of "90% node reduction" is aggressive. The actual reduction depends on how much cross-branch symmetry overlap there is. Worth benchmarking before committing to the claim in a paper.

4. **R=10 coefficient prediction (§1, line 40)** — "OEIS A000788 guarantees the coefficient will be `(10k - 15)`" — this is a _prediction_ based on the hypercube optimality conjecture, not a guarantee until either proven or verified computationally. Worth flagging that distinction.

## Overall

The core insight (linear 2R-5 breaks at powers of 2 because of hypercube geometry) is solid. Reads well as working notes.
