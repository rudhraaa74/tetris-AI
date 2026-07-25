# Tetris AI — CMA-ES / BCTS Training Plan

**Depends on:** `plan.md` (engine core) and the `engine/MoveSearch` BFS reachability component.
This plan assumes both exist and are tested before Phase 0 starts.

**Goal:** train a linear weight vector over 9 board features that plays NES Tetris at a high
level via 1-step (later 2-step) lookahead. Structured in phases so each one produces something
you can independently run and sanity-check before moving to the next — no phase should require
you to "trust" that an earlier one is correct.

---

## Phase 0 — Bindings & Harness Skeleton

**Purpose:** get Python able to drive the C++ engine at all, with nothing intelligent happening yet.

- Add a `bindings/` target using pybind11, exposing:
  - `Game::reset(seed)`, `Game::tick(Input)`, `Game::getState()`, `Game::isGameOver()`,
    `Game::getScore()`, `Game::getLevel()`, `Game::getLines()`
  - `MoveSearch::getValidPlacements(board, piece, level)` returning the fixed-capacity placement
    list as a Python-friendly array
  - A way to apply a chosen placement back into the engine (drive the actual frame-by-frame
    `Input` sequence the search found, not a teleport — this matters, since teleporting would
    silently reintroduce the exact bug the BFS work was meant to fix)
- Do **not** implement feature extraction or CMA-ES yet.

**Checkpoint:** from Python, run a scripted loop that calls `getValidPlacements`, picks the
**first** valid placement every time (dumb, not evaluated), and plays a full game to either
game-over or a piece cap. Print final score/lines/level. This proves the binding layer,
placement-application, and game-over detection all work, independent of anything AI-related.

---

## Phase 1 — Feature Extraction (`engine/Features` or Python, your choice — see note)

**Purpose:** implement and independently verify all 9 BCTS features before any optimizer touches them.

**Note on where this lives:** the board already exposes fast native feature extraction per
`plan.md` Section 3.4 (heights, holes, bumpiness, aggregate height) computed in a single pass —
extend that pass rather than duplicating it in Python. Doing this in C++ also matters for speed:
this function runs once per candidate placement per piece, i.e. extremely often.

### The 9 features, explained

All 9 are computed on the **afterstate** — the board *after* the candidate piece has been placed
and any completed lines have been cleared. This matters: a placement that would complete a line
is evaluated post-clear, not on the board as it looks the instant before clearing. Getting this
ordering wrong (evaluating pre-clear) is a classic bug that quietly cripples eroded-cells and
hole-related features.

1. **Landing Height** — how high up the piece's center ended up when it locked, measured from
   the bottom of the board. Lower is better (negative weight): pieces that lock low keep the
   stack flat and leave more room above for maneuvering later pieces.

2. **Eroded Piece Cells** — `(lines cleared by this move) × (cells of the just-placed piece that
   were part of those cleared lines)`. This is the feature that actually rewards line-clearing,
   and it specifically rewards clears where the piece you just placed contributed a lot of the
   completed row(s) — not just any clear. A tetris where all 4 rows were mostly filled by the
   I-piece itself scores higher on this feature than a scrappy clear that happened to complete
   a row mostly filled by earlier pieces.

3. **Row Transitions** — count how many times, scanning left-to-right across each row, the
   cell state flips between filled and empty (walls count as filled). High = jagged, broken-up
   rows. Low = clean unbroken runs. Negative weight: fewer transitions is better.

4. **Column Transitions** — same idea, but scanning top-to-bottom down each column (floor counts
   as filled, the space above row 19 counts as empty). Measures vertical raggedness — lots of
   transitions means the surface has jagged up-down structure rather than smooth terrain.
   Negative weight.

5. **Number of Holes** — count of empty cells that have at least one filled cell somewhere above
   them in the same column. Holes are cells you can't fill without clearing the blocking row
   first, so they're one of the most damaging things a placement can create. Heavily negative
   weight — this should dominate the evaluation whenever a placement would create one.

6. **Cumulative Wells** — a "well" is an empty column segment with filled cells (or a wall) on
   both sides. For a well of depth `d`, its cost is `d(d+1)/2` (triangular, not linear) — so a
   4-deep well costs far more than four separate 1-deep wells. This isn't purely "wells are bad"
   (a single well is often intentional, reserved for an I-piece tetris); it's specifically
   discouraging *deep* wells, since deep wells are hard to fill with anything but an I-piece and
   risk becoming permanent liabilities.

7. **Hole Depth** — for every hole, sum up how many filled cells sit directly above it in that
   column, then sum that across all holes. This distinguishes a hole one row deep (annoying but
   fixable soon) from a hole buried under 8 rows of stack (effectively permanent until a big
   clear reaches it). Heavily negative — should track alongside Number of Holes but isn't
   redundant with it, since two boards can have equal hole counts and very different severity.

8. **Rows With Holes** — count of distinct rows that contain at least one hole, regardless of
   how many holes are in that row. This is subtly different from feature 5: it rewards
   *concentrating* holes into fewer rows rather than scattering them, because a row that already
   has a hole "isn't going anywhere" until cleared regardless of whether it has one hole or
   three, so spreading holes across more rows ties up more of the board.

9. **Pattern Diversity** — count adjacent-column pairs where the height difference is less than
   2 (i.e., roughly flat relative to each other). This is the one *positive*-weighted feature in
   the standard set: it rewards a flat, even skyline, which keeps the board flexible for
   whatever piece comes next, rather than a jagged surface that only accepts specific shapes.

**Checkpoint:** write unit tests with hand-constructed boards (the same style as the engine's
existing hand-verified board-feature tests) where you compute each of the 9 values by hand and
assert the extractor matches exactly. Do this **per feature**, not as one combined "does the
score come out right" test — a bug in one feature can be masked by others in an aggregate score
and won't surface until much later in training, when it's far harder to trace back to a specific
feature.

---

## Phase 2 — Fixed-Weight Sanity Runner (no optimizer yet)

**Purpose:** confirm the full evaluate → pick-best-placement → play loop works correctly using
a fixed, hand-chosen weight vector — before adding any optimization on top of an unverified loop.

- Use the standard reference initial weights (roughly: strongly reward eroded cells and pattern
  diversity, strongly penalize holes and hole depth, moderately penalize landing height, row/col
  transitions, and wells) as a fixed vector.
- For every piece: get valid placements from `MoveSearch`, extract features for each resulting
  afterstate, score with the dot product, execute the best-scoring placement.
- Run this for a handful of full games and just look at the outcome — this should already play
  *recognizably competently* (clearing lines, not obviously self-sabotaging) even without any
  tuning, since these are known-decent reference weights. If it doesn't, the bug is in
  placement-search, feature extraction, or scoring — not in CMA-ES, which doesn't exist yet.
  Debug here, not later.

**Checkpoint:** a handful of games with sane-looking scores/behavior, ideally with `tetris_render`
watchable so you can eyeball a few games rather than trusting numbers alone.

---

## Phase 3 — CMA-ES Loop, Single-Threaded, Small Scale

**Purpose:** get the optimizer running end-to-end before caring about speed.

- Implement `evaluate_candidate(weights)`: normalize weights, run a small number of trial games
  (e.g. 3–5) capped at a modest piece count (e.g. 2,000 pieces — smaller than the eventual cap,
  intentionally, to keep early iterations fast), return mean lines cleared.
- Wire up the `cma` library loop exactly as sketched earlier: initialize mean/sigma, `ask()` a
  population, evaluate each candidate serially, `tell()` results back.
- Use a small population size and low `maxiter` (e.g. `popsize=8`, `maxiter=20`) purely to prove
  the loop runs and the mean vector visibly moves generation to generation.

**Checkpoint:** log the best fitness per generation and eyeball that it's trending upward, even
if slowly/noisily at this scale. This is a correctness checkpoint, not a performance one — the
goal is "does the optimizer learn anything at all," not "is it good yet."

---

## Phase 4 — Parallelize Evaluation

**Purpose:** now that the loop is proven correct, make it fast enough to actually train.

- Swap serial candidate evaluation for `ProcessPoolExecutor` (or equivalent) across candidates.
- Confirm each worker process gets its own independent engine instance and seed — no shared
  state across processes (this should already hold given `plan.md`'s per-`Game`-instance RNG
  design, but verify it explicitly here rather than assuming the C++-side guarantee survived
  the Python binding layer).
- Increase population size and per-candidate trial count back up toward the real target now
  that wall-clock cost per generation is back under control.

**Checkpoint:** measure wall-clock time per generation before/after parallelization, and confirm
final fitness values for a fixed seed are consistent whether run serially or in parallel (a
mismatch here would indicate accidental shared/leaked state).

---

## Phase 5 — Real-Scale Training Run

**Purpose:** the actual training run, now that every layer under it has been independently
checked.

- Scale up: full population size (`popsize ≈ 16`), realistic piece cap (e.g. 10,000 pieces),
  more trials per candidate (e.g. 5–10) to reduce noise from piece-sequence variance, and a
  much higher `maxiter` (or run until fitness plateaus rather than a fixed generation count).
- Add checkpointing: save the current mean vector, sigma, and covariance matrix periodically
  (not just at the end) so a crashed or interrupted run doesn't lose progress.
- Add logging beyond just best-fitness-per-generation: track population fitness spread
  (best/median/worst) to get a feel for whether the search is still exploring or has converged.

**Checkpoint:** a saved best weight vector, plus a log/plot of fitness over generations showing
convergence (increasing then flattening, not still climbing steeply when you stop — if it's
still climbing steeply, you stopped too early).

---

## Phase 6 — Push Further (optional, after Phase 5 produces a working agent)

Only attempt after you have a working single-lookahead agent from Phase 5 and want to push score
higher:

- **2-piece lookahead**: evaluate current piece + known next piece jointly (the "$30 \times 30$"
  combination space mentioned in your research doc). This is a real jump in per-move search cost
  — budget for it rather than assuming it's a free upgrade to the existing loop.
- **Higher piece/line caps** for evaluation once the agent is good enough that games realistically
  run long — the original cap existed to stop early bad-but-not-hopeless candidates from
  freezing the loop, but a well-trained agent may need a much higher cap to actually distinguish
  "great" from "exceptional" candidates.
- Revisit weight normalization strategy if you notice the optimizer struggling to break past a
  score plateau — this is a known sensitivity point in linear-eval Tetris AIs.

---

## Cross-Phase Notes

- **Don't skip checkpoints to save time.** Each phase's checkpoint exists specifically to
  localize bugs to the layer that was just added — skipping from Phase 1 straight to Phase 5
  means a bad result gives you almost no information about which of five layers is at fault.
- **Determinism matters for debugging, not just correctness.** Since each `Game` is seeded
  (per `plan.md` Section 4), keep a way to re-run any specific candidate/seed combination that
  produced a surprising result (very high or very low fitness) so you can inspect it in
  `tetris_render` rather than only ever seeing aggregate numbers.