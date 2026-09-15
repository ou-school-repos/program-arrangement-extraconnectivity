# Archived: Single-Vertex Hole-Filling Induction (Bridge 1, Abandoned)

**Status: FALSIFIED by a minimal witness, not just a corpus sweep.** This
document preserves the "fill the hole" induction strategy explored for
`UniversalLowerBound` / the strictness conjecture (`slack > 0` whenever
`L_c > 0`; see `docs/proof-sketch-weighted-potential.md`), kept here for
historical record. It is **not** part of the active roadmap. Do not resume
this direction without a genuinely different move (see "What would revive
this" below).

## The strategy

Prove `Slack(S) = ΔC + m·ΔE − (m+1)·conc − L_c ≥ 0` for arbitrary `S` by
induction on `L_c`, walking from `S` up to a saturated, collision-free
state `S_final` (`L_c = 0`, `Slack ≥ 0` trivially) one vertex at a time:
pick a swallowed slot `(i, x)` — `x ∈ P_i \ Q_i` with `x` in the local
boundary of `Q_i` — and add `v = (i, x)` to get `S' = S ∪ {v}`.

For this to prove `Slack(S) ≥ 0`, slack must be **non-increasing** along
the walk: `Slack(S') ≤ Slack(S)` at every step. Then
`Slack(S) ≥ Slack(S₁) ≥ … ≥ Slack(S_final) ≥ 0`. (The other polarity —
slack non-decreasing on fill — only bounds `Slack(S)` from *above* and is
useless for this goal.) Two variants were tested:

- **Universal**: every hole-fill step satisfies `ΔSlack ≤ 0`.
- **Existential** (weaker, still sufficient): *some* hole at each stage
  satisfies `ΔSlack ≤ 0`, i.e. there is always at least one safe move.

## Computational result

Implemented in `scripts/hole_filling_check.py`, run against the same
3,796-set adversarial corpus used for the strictness conjecture
(`scripts/coverage_avoidance.py`), producing 7,836 individual hole-fill
steps.

- **Universal fails hard**: `ΔSlack > 0` on 1,736/7,836 steps (22%),
  worst case `ΔSlack = +6` in a single fill (`crumb-6-16-10`, filling
  `(0,3,2)` on `R=15, Lc=14, slack=16 → R=16, Lc=13, slack=22`).
- **`α` (the collision mass a fill destroys) is not bounded below by 1,
  or even by 0**: histogram over all steps was
  `{-2: 33, -1: 531, 0: 1737, 1: 5535}`. Filling one hole can *create*
  new swallowed slots elsewhere — the newly-occupied vertex projects its
  own local boundary, which can reach neighboring empty slots that were
  previously safe. This ripple effect kills any argument assuming a
  fixed per-hole geometric refund.
- **`|π|` invariance holds exactly**: filling a swallowed slot never
  changes the projection base `π` (0 violations across all 7,836 steps).
  This sub-claim — the one piece of the original hole-filling proposal
  that simplified the accounting — is correct and worth keeping if this
  direction is ever revisited.
- **Existential also fails**: 212 of 2,318 sets with `L_c > 0` have *no*
  safe hole at all — every available fill strictly increases slack.
  Ruled out by minimal witness, not just aggregate rate (see below).

## The minimal witness: `A4r-R4-2`

`S = {(0,1,3), (1,3,2), (2,3,0), (3,0,1)} ⊂ A(4,3)`, sweep coordinate 0.

- Slices: `R_0=R_1=R_2=R_3=1`, `π=4`, `conc=0`, **exactly one** swallowed
  slot: `(1, (3,0))` — slice 1 is missing `(1,3,0)`, adjacent to the
  occupied projection `(3,2)`.
- Before: `R=4, ΔC=4, ΔE=4, m=1, Slack = 4+4−0−1 = 7`.
- Filling `(1,3,0)` (the *only* available move): `R=5, π=4, conc=1,
  L_c: 1→0` (clean, `α=1`, no ripple — this is the minimal case).
  `ΔC=6, ΔE=4` (`β = ΔC(S')+m·ΔE(S') − ΔC(S)−m·ΔE(S) = 2`).
  `ΔSlack = β − (m+1) + α = 2 − 2 + 1 = +1`. New `Slack = 8`.

**This one set has one hole and one possible move, and that move strictly
increases slack.** There is no greedy alternative to try — batch-filling
is identical to single-vertex filling here since there's only one hole to
batch. No reordering or hole-selection heuristic can route around it.

### Why: binary-digit phase misalignment

`ΔC = C(R) − Σ C(R_i)` jumped by 2 not because of anything dramatic in
the local geometry, but because **global `R` crossed a power-of-two
boundary (`4→5`, `d: 2→3`) that the local slice `R_1` did not**
(`R_1: 1→2`, `C(1)=0 → C(2)=1`, a mild jump of 1, vs. `C(4)=4 → C(5)=7`,
a jump of 3). `C(R)` and `E(R)` are step functions of `⌈log₂ R⌉`; a
single-vertex edit moves `R` and `R_i` past independent thresholds at
different times, and there is no way for a local, single-vertex operation
to respect two decoupled binary-digit boundaries simultaneously. This is
the general mechanism, not a coincidence of this witness — it's also
consistent with the 22% universal-failure rate and the negative-`α`
ripple failures above.

## Verdict

**Single-vertex induction (either direction — tearing down from a
saturated state, or filling up toward one) is structurally the wrong
induction variable for this bound.** `L_c` is not monotone/submodular
enough under single-substitution moves, and the arithmetic budget
(`ΔC`, `ΔE`) is driven by a step function that a single-vertex edit
cannot control the phase of. This closes Bridge 1 from
`docs/proof-sketch-weighted-potential.md` (or wherever the three-bridge
writeup for the strictness conjecture is filed).

## What would revive this

Not batch-filling on this witness — it has one hole, so batching is a
no-op. A revival would need a genuinely nonlocal move that changes
several `R_i` (and possibly `R`) at once in a way that keeps the local
and global binary-digit boundaries in phase, or a different potential
function whose discrete derivative isn't driven by `⌈log₂⌉` steps. No
such operator has been proposed or checked.

## Reproducing

```
python3 scripts/hole_filling_check.py
```

reruns the full corpus check (universal + existential) and prints the
worst single step and the existential-failure list, including
`A4r-R4-2`.
