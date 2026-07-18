# Pre-Release Audit — 2026-07-18 bundle

Scope: README.md, paper.tex, predict.cpp, lean-proof-status.md,
collision-axiom-roadmap.md, current_state.md, ArrangementExtraconnectivity.lean,
git-log-head-20.patch. (`working-tree.patch` is listed in REQUEST.md but was
not in the bundle — resend it if there are uncommitted changes; this audit
assumes HEAD is the state of record.)

Verdict up front: the repo is materially better than two weeks ago and the
README/paper are now close to honest. But three docs actively contradict each
other about proof status, one status doc contains claims I could falsify from
the bundle itself, and `predict.cpp` has one real memory bug at large R. None
of this blocks building the paper; all of it should be fixed before calling
the documentation "in good order."

---

## 1. Build checks (done in this audit)

- **paper.tex builds.** `pdflatex` twice → exit 0, PDF produced, **zero
  undefined internal references**. The only failures were repo assets absent
  from the bundle (`../docs/complexity-curves.pdf`, `../assets/out/R{9,10}_graph.png`,
  `../src/predict.cpp` for embedfile, `predict_core_sample.cpp`, `references.bib`).
  Action: none needed if those exist in the repo; add a `make paper` target that
  fails loudly listing missing assets rather than emitting a broken PDF.
- **predict.cpp compiles clean** under
  `g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wstringop-overflow=4`: zero
  warnings. The earlier `-Wstringop-overflow` issue is genuinely gone.

## 2. predict.cpp — findings (REQUEST Q1)

**P1 (real bug): `named_nbrs.reserve(width * width * M)` massively
over-reserves.** Actual pushes are bounded by `width · width · (M − width)`
(only symbols _not_ in the vertex are tried), i.e. ~`R²·dims`. The reserve asks
for `R²·(R+dims)` elements of `sizeof(Vertex<K,SymT>)`:

| R   | K, SymT     | reserve request | actual need |
| --- | ----------- | --------------- | ----------- |
| 127 | 128, uint8  | ~0.3 GB         | ~15 MB      |
| 256 | 256, uint16 | ~8.9 GB         | ~0.3 GB     |
| 260 | 512, uint16 | ~18.6 GB        | ~0.6 GB     |
| 512 | 512, uint16 | ~137 GB         | ~2.4 GB     |

So `--verify` / `--verify-range` above ~R=128 will OOM (or thrash) on ordinary
machines even though the CLI advertises support to 512, and the docs claim
"verified up to R=260." Fix: `reserve(static_cast<size_t>(width) * width * (M - width))`
— one line — and while there, make the multiplication `size_t` to remove any
future int-overflow doubt. If the R≤260 verification claim was made with the
old `R*R*M` reserve, it had the same problem (same formula); re-run
`--verify-range 2 260` after the fix and record the machine/RAM in the doc.

**P2 (stale header comments):** the file header says "Usage: ./predict [R]
(R ≤ 64)" and "verification (R ≤ 40)", but the code accepts `--verify` to 512
and analytical mode unbounded. Same file, three different stated limits. Fix
the header to match `main`'s actual guards (verify ≤ 512, analytical any R ≥ 0).

**P3 (input validation):** `strtol` results are used unchecked; `./predict foo`
silently computes R=0. Check `endptr`/`errno` (or use `std::from_chars`) and
error out on non-numeric input. Also `./predict --csv --verify 20` silently
ignores `--verify`; either honor it or reject the combination.

**P4 (global `R`, REQUEST Q7 from the earlier writeup):** `run_verify` and the
R≤12 pretty-printer still read the global. The cleanest fix is small: pass `r`
as an explicit parameter to `run_verify`/`build_hamming_ball` call sites and
delete the global entirely (the only remaining consumers are in `main`). Not
urgent, but do it before anyone adds threading or a library entry point.

Otherwise the design is fine for its purpose: the tiered structure is clear,
the templated dispatch is a reasonable way to keep the hot path
allocation-free, and `std::abort()` on the internal invariant is the right
call after the throw/lint episode.

## 3. Documentation consistency matrix (REQUEST Q3, Q4)

Claims that currently disagree across the bundle, with the source of truth:

| Claim                                           | Says                                                                                                               | Reality (from bundle)                                                                                                                                                                                                                                                                            |
| ----------------------------------------------- | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `CrossDimStable` status                         | roadmap "Immediate Work Queue": still to finish; "Started: CrossBaseOne…"                                          | PROVEN (current_state.md, git log `proof: fill out sorry in cross_dim_stable`). Roadmap items 1–2 are stale.                                                                                                                                                                                     |
| unstable file sorry count                       | roadmap: "several … placeholders remain explicit sorrys"                                                           | Exactly one (`cross_recurrence`), per current_state.md and the file itself.                                                                                                                                                                                                                      |
| strong-induction driver                         | roadmap Step 4: "COMPLETED"; current_state implies proven                                                          | **TODO.txt in commit `proof: eliminate native_decide…` says a known omega gap remains in the driver assembly.** Nothing else in the bundle discloses this. Whichever is true, one doc is wrong; if the omega gap is real, "Step 4 COMPLETED" and current_state's framing are both overclaims.    |
| bit-rot blocking `CrossCollisionsResearch.lean` | current_state.md: private `embed_vertex_injective_cube`, renamed `image_map`, stale omegas still block clean build | In the uploaded sources these are already fixed (`embed_vertex_injective_cube` is public; `Finset.map_eq_image + Finset.image_image` in use). current_state.md is stale — or tracking a different branch.                                                                                        |
| dependency graph                                | lean-proof-status.md still shows `hb_cross_collisions [AXIOM]`, `lower_bound_all_embeddings [AXIOM]`               | Axioms were converted to hypothesis `Prop`s (`HBCrossCollisions`, `UniversalLowerBound`) many commits ago. Update the graph.                                                                                                                                                                     |
| `hamming_ball_eval` row                         | lean-proof-status table: "PROVEN" (no asterisk)                                                                    | Proven **conditional** on an `HBCrossCollisions` argument. Give it the same `PROVEN*` marker used for the capstone, or the table's own convention is violated.                                                                                                                                   |
| `nat_popcount_eq_card_filter` row               | lean-proof-status: PROVEN                                                                                          | **Not present in ArrangementExtraconnectivity.lean.** If it lives in `HypercubeEdges.lean`, say so in the table (file column); if it doesn't exist anywhere, delete the row. (If it does exist, note: it is exactly the `popcount_eq_card_testBit` sorry in `CrossTop_v2.lean` — free progress.) |
| verification range                              | lean-proof-status: "predict --verify R … for all R ≤ 260"                                                          | See P1: current code cannot comfortably verify R=260 on a normal machine. Either document the machine or re-verify after the reserve fix.                                                                                                                                                        |
| `CollisionAdjustedBound`                        | lean-proof-status §2 presents it as a live hypothesis with a "justification", TODO'd                               | It is **provably false**: by the exact identity `ext + X + Rk = U(n−k+1)` (from `total_coord_edges_eq`), it is equivalent to `X + D ≤ C(R)`, refuted by the star graph. Stop listing it as a hypothesis interface; see §5 below for the replacement.                                             |

Pattern behind most rows: **three documents claim to describe proof status
(lean-proof-status.md, collision-axiom-roadmap.md, current_state.md), plus
README, plus paper Appendix, plus docs/architecture.md and
docs/project-summary.md (touched in the latest commit, not in this bundle).**
That is six narrators. README already declares "docs/lean-proof-status.md is
the source of truth" — enforce it: the roadmap should contain only the plan
(no status claims), current_state.md should be generated or dated-and-scoped,
and the others should link rather than restate.

The TODO(review) markers are also past their usefulness: collision-axiom-roadmap
still _ends_ on "allowing us to mechanize the complete proof … !" two lines
after a TODO saying the claim is unsupported. Rewrite or excise the
support-projection and Step-3 sections (move the invalid narrative to an
explicitly labeled `docs/archive/` file); annotating false prose in place has
now failed twice as a containment strategy.

## 4. README (updated) — assessment

Good. The status language ("intentionally parameterized by outstanding
extremal-combinatorics hypotheses", the Asymptotic Penalty caveat, uniqueness
as open) matches the Lean sources. Three nits: (a) "Collision Factor
$C=\binom{R}{2}$" for the star graph is the combined waste $X+D$, and the
symbol $C$ collides with $C_{const}$ — rename to avoid a reader concluding the
refuted combined-waste bound is being asserted; (b) the sample output block is
from `arrangement` (the search engine) but sits under a `run/predict`
quickstart — label which binary produced it; (c) once PenaltyExact.lean lands
(§5), the "Asymptotic Penalty Status" paragraph can be upgraded from a caveat
to a positive statement.

## 5. The penalty theorem: from false-hypothesis to unconditional (open issue #1/#2)

Delivered alongside this audit as `PenaltyExact.lean`. The key realization:
`total_coord_edges_eq` (proven, stable) already yields the **exact,
unconditional** identity

    external_neighbors V' + cross_collisions V' + R·k = U(V')·(n−k) + U(V')

and from two instances of it, the paper's eq:penalty-expansion verbatim:

    |∂V₁| + (X₁+D₁) = |∂V₂| + (X₂+D₂) + ΔD·(n−k)

with no hypotheses beyond equal cardinality and `k ≤ n`. Both statements were
verified on 500 random subset pairs of A(7,3). The four proofs use only
`total_coord_edges_eq`, `external_neighbors_le_total_coord`,
`sum_unique_roots_le_rk` (signature checked against the current file), and
omega; they should close as written. Consequences once compiled:

- delete/archive `CollisionAdjustedBound` and old `sub_optimal_penalty`;
- Asymptotic Penalty becomes **PROVEN, unconditional** in lean-proof-status;
- paper Theorem `them:penalty` gains a mechanized citation for its central
  equation (its `O_R(1)` prose framing already matches `penalty_exact` exactly).

## 6. Answers to REQUEST's five questions

1. **predict.cpp acceptable?** Yes after the one-line reserve fix (P1) and the
   header/parsing cleanups (P2–P3). The aggressive refactor to context objects
   (P4) is worth ~an hour, not a rewrite.
2. **Stable/unstable/computational separation well designed?** The three-way
   split is right; the failure mode is _narration_, not structure. Add the two
   mechanical guards: a scheduled non-gating `lake build` over `unstable/`
   (reports rot without blocking), and a pre-commit check that fails when a
   previously sorry-free theorem gains a `sorry` or disappears.
3. **Right level of caution now?** README and paper: yes, essentially. The
   status docs: no — the matrix in §3 lists nine concrete contradictions, one
   of which (driver omega gap disclosed only in TODO.txt) is exactly the class
   of silent overclaim this project keeps re-catching by hand.
4. **Biggest maintainability risk?** Six overlapping status narrators with no
   mechanical reconciliation. Every incident in WRITEUP.md and every row in §3
   is this one risk expressing itself. Second place: `unstable/` invisibility
   (already half-solved by the scheduled-build idea).
5. **Priority order:** (i) resolve the driver-omega-gap question and make
   lean-proof-status.md true (half a day, unblocks every other claim);
   (ii) land PenaltyExact.lean and delete the false-hypothesis pair (small,
   removes the worst remaining "misleading theorem name");
   (iii) predict.cpp P1–P3; (iv) collapse the status narrators per §3;
   (v) then return to the real math (`cross_recurrence` — or better, the
   CrossTop route, which also retires the driver and its omega gap entirely).

## 7. Ready-to-build checklist

- [ ] Confirm/resolve the driver "omega gap" from TODO.txt; sync roadmap +
      current_state + lean-proof-status to whatever is true.
- [ ] Apply predict.cpp P1 (reserve), P2 (header), P3 (strtol).
- [ ] Re-run `predict --verify-range 2 260`; record result + machine in docs.
- [ ] Compile PenaltyExact.lean; delete `CollisionAdjustedBound` +
      `sub_optimal_penalty`; update status table + README paragraph + paper
      appendix sentence.
- [ ] Fix lean-proof-status: dependency graph [AXIOM]→hypothesis params,
      `hamming_ball_eval` asterisk, `nat_popcount_eq_card_filter` file column
      or deletion.
- [ ] Rewrite (not TODO-annotate) roadmap Steps 1–3 + support-projection
      section; move refuted narrative to docs/archive/.
- [ ] `make paper` target with asset existence checks; commit `references.bib`
      presence check.
- [ ] Bundle nit: include `working-tree.patch` next time or note it was empty.
