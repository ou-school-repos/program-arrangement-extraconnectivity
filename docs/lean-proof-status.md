# Lean 4 Formal Verification Status

Formal verification of the Arrangement Graph extraconnectivity theorem
using Lean 4 and Mathlib.

## Build

```bash
make lean          # Build and verify proofs
make lean/cache    # Download pre-built Mathlib cache (first time)
```

Current status: **909 jobs, 0 errors, 1 sorry** (capstone theorem only).

## Architecture

The proof is organized in three layers:

| Layer                | Theorem                                               | Status       |
| -------------------- | ----------------------------------------------------- | ------------ |
| 1 - Combinatorics    | `E_add_min_le`: E(x)+E(y)+min(x,y) <= E(x+y)          | DONE         |
| 2 - Harper's Theorem | `harpers_edge_isoperimetry`: cubeEdges(S) <= E(\|S\|) | DONE         |
| 3 - Embedding        | `embed_cube`, `embedding_is_injective`                | DONE         |
| 3 - Capstone         | `arrangement_extraconnectivity_minimum`               | TODO (sorry) |

### Layer 1: Subadditivity of A000788

The sequence $E(n) = \sum_{i=0}^{n-1} s(i)$ (OEIS A000788), where $s(i)$ is the
binary digit sum (popcount), satisfies the **subadditive inequality**:

$$E(x) + E(y) + \min(x,y) \le E(x+y)$$

This is proven by strong induction on $x+y$ with a four-way parity case split
(Even/Even, Even/Odd, Odd/Even, Odd/Odd). Each case reduces to the inductive
hypothesis via the recurrences:

- $E(2m) = 2 E(m) + m$
- $E(2m+1) = E(m) + E(m+1) + m$

All arithmetic obligations are closed by `omega`.

### Layer 2: Harper's Edge Isoperimetric Inequality

**Theorem.** For any subset $S$ of the $d$-dimensional hypercube $Q_d$,
the number of edges within $S$ is at most $E(|S|)$.

The hypercube is represented as `Fin d → Bool` (bit-vectors), avoiding
recursive sum types and eliminating all `Classical.choice` dependencies.
The proof proceeds by induction on $d$:

1. **Partition** $S$ by the last coordinate into $S_0$ (last bit = 0) and
   $S_1$ (last bit = 1), then project via `dropLast`.
2. **Card split**: $|S| = |S_0| + |S_1|$ — proven via filter partition
   exhaustiveness and injectivity of `dropLast` on each half.
3. **Edge decomposition**: edges within $S$ = edges in $S_0$ + edges in
   $S_1$ + crossing edges, where crossings $\le |S_0 \cap S_1| \le \min(|S_0|, |S_1|)$.
4. **Squeeze**: By the inductive hypothesis and `E_add_min_le`:

$$\text{cubeEdges}(S) \le E(|S_0|) + E(|S_1|) + \min(|S_0|,|S_1|) \le E(|S_0|+|S_1|) = E(|S|)$$

### Layer 3: Arrangement Graph Embedding

An arrangement graph vertex $A(n,k)$ is an injective function `Fin k → Fin n`.
The embedding maps a hypercube vertex $v \in Q_d$ to an arrangement vertex by:

- Position $p < d$: use **fresh symbol** $k+p$ if $v_p = 1$, else **base symbol** $p$
- Position $p \ge d$: use base symbol $p$ (unchanged)

**Injectivity** is proven by showing fresh symbols ($\ge k$) never collide
with base symbols ($< k$), and within each class the position index uniquely
determines the output. All bounds are closed by `omega`.

### Capstone (remaining sorry)

The final theorem states that for any $R$ vertices in $A(n,k)$, the number of
external neighbors is minimized by the Hamming ball embedding, with the
minimum given by:

$$\kappa = (Rk - E(R))(n-k) - C(R)$$

where $C(R) = (R-1) + \sum_{i=0}^{R-1} \lfloor\log_2 i\rfloor + 1 - E(R)$.

#### Required definitions

1. **Adjacency**: Two arrangement vertices are adjacent iff they differ at
   exactly one position:

   ```lean
   def ArrAdj (u v : ArrVertex n k) : Prop :=
     exists! (p : Fin k), u.val p != v.val p
   ```

2. **External neighbors** (vertex boundary): vertices outside V' adjacent
   to at least one member of V':

   ```lean
   def external_neighbors (V' : Finset (ArrVertex n k)) : Nat :=
     (Finset.univ.filter (fun w => w not in V' and exists v in V', ArrAdj v w)).card
   ```

3. **Fintype instance**: `Fintype (ArrVertex n k)` -- the type of injective
   functions `Fin k -> Fin n` is finite. Required for `Finset.univ`.

#### Proof roadmap

The capstone splits into two halves:

**Upper bound (existence)**: Construct the Hamming ball embedding via
`embed_cube` and compute `external_neighbors` exactly. This requires:

- `embed_cube_adjacent`: hypercube adjacency implies arrangement adjacency.
  If `u, v : Cube d` differ in exactly bit `p`, then `embed_cube u` and
  `embed_cube v` differ in exactly position `p`.
- Counting the distinct neighbors of the embedded Hamming ball. Each
  embedded vertex has `k(n-k)` arrangement neighbors total. The key
  decomposition is by **ray groups**:
  - At each position `p`, vertices in V' that agree at all OTHER positions
    form a "group". Group members share some neighbors (the "named"
    neighbors within the used symbol set M) and have anonymous neighbors
    (symbols outside M, which are always unique per vertex).
  - Anonymous neighbors contribute `anon_coeff * (n - M)` where `anon_coeff`
    is the total number of (vertex, position) pairs.
  - Named neighbors require deduplication (same swap from different vertices
    in the same group yields the same neighbor).
  - The collision/overlap count is `C_constant(R)`.

**Lower bound (minimality)**: Show any set of R vertices has at least as
many external neighbors. This uses:

- Each vertex has degree `k(n-k)` in A(n,k).
- Total edge-endpoints from V': `R * k * (n-k) = 2 * internal_edges + boundary_edges`.
- Harper's theorem gives `internal_edges <= E_seq(R)` (maximized by the
  Hamming ball).
- More boundary edges means more external neighbors (modulo collisions).
- The collision bound requires showing the Hamming ball also minimizes the
  number of shared external neighbors.

## File Map

| File                                       | Contents                             |
| ------------------------------------------ | ------------------------------------ |
| `proofs/ArrangementExtraconnectivity.lean` | Main proof: Layers 1–3               |
| `proofs/HypercubeEdges.lean`               | Supporting popcount/A000788 lemmas   |
| `proofs/PredictorComplexity.lean`          | Complexity analysis of the predictor |
| `proofs/lakefile.lean`                     | Lake build configuration             |

## Dependencies

- **Lean**: v4.30.0-rc2
- **Mathlib**: Current master (pinned in `lake-manifest.json`)
- Key imports: `Mathlib.Data.Fintype.Pi`, `Mathlib.Data.Finset.Card`
