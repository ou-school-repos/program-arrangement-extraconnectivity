# Lean 4 proof status

Status reviewed 2026-09-19. This file distinguishes proved boundary statements
from the still-open exact profile and extra-connectivity questions.

## Current capstone

`Arrangement/Capstone.lean` proves `arrangement_boundary_sandwich`. For every
volume `R` and every `(n,k)` satisfying the Boolean-cube embedding gate, it
states

\[ \Phi*{n,k}(R) \le H(R,n,k) \le \Phi*{n,k}(R)+E(R)+R(R-1). \]

and supplies an `R`-vertex set attaining `H`. Here `\Phi` is the minimum
external vertex boundary over all `R`-subsets, `H` is the exact boundary of the
embedded Hamming-ball witness, and `E` is the cumulative-popcount term. The
per-set lower bound needs only `k ≤ n`; the exact witness requires the embedding
gate. This is not an exact-isoperimetric theorem and makes no claim about `κ_g`.

The additive error is not removable as a universal zero-error assertion: Star
and other finite witnesses beat the Hamming value in some arrangement graphs.
The theorem is designed to remain valid in those cases.

## Main proved components

| Component                          | Lean declaration                                                            | Status                                                                |
| ---------------------------------- | --------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| Popcount partition inequality      | `E_add_min_le`, `E_seq_list_sum_le`                                         | Proved                                                                |
| Arrangement vertices and adjacency | `ArrVertex`, `arr_adjacent`                                                 | Proved definitions/instances                                          |
| Defect bound                       | `sum_unique_roots_lower_bound`                                              | Proved                                                                |
| Coordinate-fiber identity          | `total_coord_edges_eq`                                                      | Proved                                                                |
| Collision charging bound           | `cross_collision_bound_factor_one` and `restricted_lower_bound_up_to_error` | Proved                                                                |
| Hamming witness and evaluation     | `hamming_ball_subset`, `hamming_ball_eval`, `hb_cross_collisions_closed`    | Proved                                                                |
| Fixed-volume boundary capstone     | `arrangement_boundary_sandwich`                                             | Proved, additive-error sandwich                                       |
| Exact penalty identities           | `Arrangement/PenaltyExact.lean`                                             | Proved                                                                |
| Pairwise phase comparisons         | `Arrangement/Phase.lean`                                                    | Proved from exact penalty identity; family signatures remain separate |

Build with `make lean` from the repository root. The legacy conditional
exact-minimum interfaces and the disproved universal/restricted zero-error
claims are historical only; they are not premises of
`arrangement_boundary_sandwich`. Refuted statements belong under
`proofs/Arrangement/refuted/` and should not be described as active capstone
hypotheses.

## What is not proved

- No general exact formula for `\Phi_{n,k}(R)` is known from this development.
- No theorem says the Hamming ball minimizes boundary for every feasible
  `(n,k,R)`; computational counterexamples rule out that blanket claim.
- The connected-pattern catalogue in `src/arrangement.cpp` is computational, and
  does not certify the unrestricted profile. The small-profile enumerator
  results are also computational certificates, not Lean theorems.
- No general formula for `g`-extra-connectivity `κ_g` is proved. A boundary
  profile theorem alone does not automatically give an extra-connectivity
  theorem; component-size and valid-cut conditions must also be handled.
- The Star-cut connectivity theorem and the full characterization of
  intermediate `(D,X,budget)` phases are not formalized in Lean.

## Remaining research targets

1. Prove or refute the budgeted pattern-envelope characterization, including
   completeness of pattern enumeration and the alphabet/coordinate embedding
   criteria.
2. Extend exact unrestricted profile computations beyond the current small
   cells, with reproducible witnesses and disconnected-set split checks.
3. Classify the phase transitions among Hamming, folded-box, Star, and hybrid
   patterns. Pairwise crossings are affine in `n-k` once a family's defect and
   collision counts are known, but these observed families are not exhaustive.
4. Bridge fixed-volume boundary minima to `g`-extra-connectivity with explicit
   component-size hypotheses, then determine where the resulting bounds are
   sharp.

## Reproducibility

The key public theorem is in `proofs/Arrangement/Capstone.lean`; its imported
boundary and charging lemmas are in
`Arrangement/ArrangementExtraconnectivity.lean` and `Arrangement/CrossTop.lean`.
Refuted historical propositions are isolated in the `refuted` subtree. Do not
cite a successful build as evidence for any computational profile value: those
require the enumerator output and its independent witness/boundary checks.
