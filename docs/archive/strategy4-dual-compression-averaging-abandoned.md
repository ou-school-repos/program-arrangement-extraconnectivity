# Archived: Pinto-Style Dual Root-Compression Averaging (Strategy 4, Abandoned)

**Status: FALSIFIED for the tested operator pair, confirmed at scale.** This
document preserves the dual-compression averaging strategy explored for the
weighted-potential target `X(V') + (m+1)D(V') ≤ C(R) + m·E(R)` (see
`docs/proof-sketch-weighted-potential.md`, "Strategy 4 sketch"). Kept for
historical record only; not part of the active roadmap.

## The strategy

Following Pinto's proof of the Bollobás–Leader directed-path conjecture
(arXiv:1504.07079), define a pair of cardinality-preserving operators on
`A(n,k)`, keyed to a symbol pair `(a,b)`:

- `C(V')`: guarded compression — for each `v` containing `b`, replace `b→a`;
  skip (leave unchanged) if `v` already contains `a`.
- `D(V')`: push-up repair — same as `C`, except when guard-blocked (`v` contains
  both `a` and `b`), apply the full transposition `(a b)` to the whole vertex
  instead of skipping.

Goal: show `Φ(V') = X(V') + (m+1)·D_defect(V')` is non-decreasing along at least
one of `C`, `D` for every `V'` — an _existence_ claim, not a universal one
(Pinto's own averaging inequality is one mechanism for guaranteeing existence,
but existence could in principle hold without the literal averaging bound; both
were checked).

## First result (small graphs, `A(4,2)`–`A(6,3)`, `R≤9`): looked promising

`scripts/dual_compression_check.py`, 1,052 non-degenerate trials:

- Literal averaging inequality `Φ(V') ≤ avg(Φ(C),Φ(D))` holds only **672/1052
  (64%)**.
- Existence for a single random `(a,b)` pair (pick the better of `C`/`D`):
  **878/1052 (83.5%)**. This is the honest headline number — not the rescue rate
  below.
- Existence over the _best_ choice of `(a,b)` and operator, among the 174 cases
  that failed for their originally-sampled pair: **171/174 (98%) rescued**,
  leaving 3 residual witnesses, all in `A(4,3)` (`m = n−k = 1`).

This 3-witness residual was initially (incorrectly) read as a near-total success
confined to a degenerate `m=1` corner, worth patching with a base-case lemma.
Two follow-ups refuted that reading.

### Autopsy of the 3 small residuals (real, but not the whole story)

- **Witness 1** (`A(4,3)`, `R=7`): `Φ₀=12` (`D=5,X=2`), best achievable `Φ=11`
  (`D=4,X=3`) — one defect unit traded for one collision unit, net `−1`. 8 of 12
  ordered symbol pairs are invalid outright (guarded replacement collides with
  an existing vertex).
- **Witness 2** (`A(4,3)`, `R=7`): `Φ₀=13` (`D=6,X=1`) → best `Φ=11`
  (`D=4,X=3`). Only 2 of the possible ordered pairs are valid at all; both give
  the same exchange.
- **Witness 3** (`A(4,3)`, `R=8`): `Φ₀=12` (`D=3,X=6`) → best `Φ=11`, reached
  via either `(D,X): (3,6)→(2,7)` or `(3,6)→(3,5)` — different paths, same net
  `−1`.

Diagnosis (holds up): two distinct obstructions, not one patchable corner —

1. **Injectivity blockage**: most `(a,b)` pairs are invalid because the guarded
   replacement collides with a vertex already in `V'`.
2. **Exact defect–collision exchange**: when a move _is_ valid, it commonly
   converts `+1 D` into `+1 X` one-for-one. Since the target weights defect at
   `(m+1)` and collision at `1`, a clean `1-for-1` trade nets `Φ` down by
   `(m+1) − 1 ≥ 1` every time — a conservation law the operator pair has no
   mechanism to beat, not a tie-breaking gap.

### Second result (`m≥2`, larger `R`, density stress test): the `m=1` reading was wrong

`scripts/dual_compression_m_stress.py`: `A(5,3)` (`m=2`) and `A(6,3)` (`m=3`) at
`R ∈ {15,20,30}`, random and Swiss-cheese (dense ball with holes) constructions,
existence checked over _every_ valid `(a,b,op)` combination (the full criterion,
not a single sampled pair).

**193 of 270 trials (71.5%) fail existence** — far worse than the small-`R`
`m=1` sample. Several failures have **`best=None`**: at `R=15` in a 60-vertex
graph, no move remains after the candidate's non-degeneracy and validity
filters. In the reported examples this arises from guarded replacements
colliding with existing image vertices. Thus `best=None` means ``no admissible
non-degenerate move for this candidate,'' not an unqualified claim that no
set-preserving operation of any kind exists.

So the `m=1` framing was an artifact of testing only small, sparse sets. Both
obstruction mechanisms diagnosed above get _worse_, not better, as `R` grows
relative to the graph: more vertices means more guard-blocking (mechanism 1),
and the fixed `1`-for-`1` exchange rate (mechanism 2) is unaffected by `m`. High
density — exactly the near-extremal regime this proof actually needs to cover —
is where the operator has the least room to maneuver, not the most.

## Verdict

**This `(C,D)` operator pair is dead**, confirmed two independent ways
(small-graph all-pairs autopsy, and large-`R` density stress test). Not a
patchable corner and not an `m=1` carve-out. The objective has the wrong local
exchange rate: a clean defect-for-collision swap is exactly what compression
tends to produce, and `Φ`'s fixed `(m+1):1` weighting never rewards it.

## What would revive this

A fundamentally different operator, not a better `(a,b)` selection rule or a
base-case exception. Two directions not yet tried:

- An operator whose guard condition has a fallback when both `a` and `b` are
  already saturated across `V'` (the injectivity-blockage mechanism), rather
  than simply skipping.
- A potential function that doesn't weight defect and collision at a fixed ratio
  — since the natural moves here trade the two 1-for-1, a potential with a
  matching (not `(m+1):1`) local exchange rate might be non-decreasing where `Φ`
  isn't. This would need to still imply the original target inequality, which
  has not been checked.

Pinto's own `C_i`/`D_i` change `|S|` and lean on set-complement symmetry between
the two operators; neither property has a clean analogue under `A(n,k)`'s
fixed-cardinality, no-repeated-symbol constraint, which may be the deeper reason
no cardinality-preserving substitute has worked so far.

## Reproducing

```
python3 scripts/dual_compression_check.py       # small graphs, all-pairs autopsy
python3 scripts/dual_compression_m_stress.py     # m>=2, larger R, density stress test
```
