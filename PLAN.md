# NES-Accurate Tetris Engine — Plan

**Goal:** Build a standalone, headless C++17 engine that replicates real NES Tetris mechanics, timing, and scoring with byte-accurate fidelity — verifiable against documented ROM data and real recorded input replays. The engine itself contains no RL/reward logic and stays a pure, correct simulation — but it is being built specifically as the substrate for a future Tetris-playing AI trained for high scores. That downstream use drives two hard requirements on top of correctness: the engine must be **fast enough to run thousands of simulated games in parallel** (training throughput), and its state must be **trivially copyable/resettable** so many instances (or many rollouts of one instance) can be driven programmatically without per-episode overhead. This phase delivers a correct, well-tested, human-playable NES Tetris clone that is already shaped for that future use, not a rewrite away from it.

---

## 1. Tech Stack & Build System

- **Language**: C++17
- **Rendering** (demo/debug only): raylib, pulled via CMake `FetchContent`
- **Testing**: Catch2, pulled via `FetchContent`, **pinned to a specific tag** (e.g. `v3.4.0`) — never track `main`, so an upstream change can't silently break the build
- **Build system**: CMake, three targets:
  - `engine/` → static library `tetris_core` (**zero dependency on raylib** — this is deliberate, so the engine can later be embedded in any host, headless or not, without pulling in graphics, and so it can be linked into a training loop / parallel-rollout harness with no graphics stack at all)
  - `render/` → executable `tetris_render`, links `tetris_core` + raylib, for human-playable manual testing
  - `tests/` → executable `tetris_test`, links `tetris_core` + Catch2
- **Build type discipline**: always build `tetris_core` in `Release`/`-O3`. Since a core requirement is fast programmatic driving (many instances, many ticks/sec), also verify LTO is enabled for `tetris_core` in Release, and keep the library free of anything that would force a debug-speed fallback (asserts in the hot path should compile out in Release, not just be "usually fast").

---

## 2. Documentation-First: `docs/nes_rules_reference.md`

Before writing any simulation logic, write this document. Every constant used in the engine must trace to a primary/authoritative source — no hand-derived or approximated values. This becomes the spec the engine is built and tested against.

### 2.1 Gravity Table (frames per row, by level 0–29)
```
Level:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29
Frames:48 43 38 33 28 23 18 13  8  6  5  5  5  4  4  4  3  3  3  2  2  2  2  2  2  2  2  2  2  1
```
Source: reverse-engineered directly from the NES ROM (address `$898E`, offset `$099E` in iNES format) — this is ROM data, not just observed behavior; treat as ground truth. Levels beyond 29 hold at 1 frame/row (kill-screen speed).

### 2.2 DAS (Delayed Auto Shift)
- Initial delay: 16 frames before the first auto-repeat shift after a direction is held.
- Auto-repeat rate: every 6 frames thereafter.
- **Critical, easy-to-miss detail**: the counter does **not** reset to 0 after an auto-shift fires — it resets to **10**. So: hold → first shift at 16 frames → counter resets to 10 → next shift 6 frames later → repeats every 6 frames from then on. Full reset to 0 only happens on release-then-repress (a fresh tap).
- The counter is **frozen but retains its charge** during `ARE` and `LINE_CLEARING` — no new charge accumulates, but whatever charge existed carries over and becomes usable again once the next piece starts falling.
- **Wall Charge (Confirmed)**: If a shift attempt is blocked (wall or stack collision) while holding the direction button, the DAS counter instantly jumps to fully-charged (16) rather than stalling. This allows players to "pre-charge" the DAS against walls for the next piece. This is confirmed by MeatFighter disassembly analysis and must be implemented exactly this way.

### 2.3 ARE (Entry Delay)
- 10 frames if the previous piece locked in the bottom 2 rows.
- +2 frames for each group of 4 rows above that, up to ~18 frames for the highest lock positions.

### 2.4 Line Clear Delay
- An additional 17–20 frames beyond ARE when one or more lines clear, driven by a 5-step clear animation advancing every 4 frames — exact total depends on which frame within that cycle the piece locked.

### 2.5 Scoring
- Base values (Level 0): Single = 40, Double = 100, Triple = 300, Tetris = 1200.
- Multiplier: `(level + 1)`, scaling linearly (confirmed via the original NES instruction manual's printed score table).
- **Resolved edge case — level multiplier timing**: if a line clear triggers a level-up in the same event, the multiplier used is the **post-transition level**, not the level the piece locked at (e.g., a tetris that pushes level 5→6 scores at the level-6 multiplier). This is a one-frame timing detail that's easy to implement wrong — needs an explicit test case.
- Soft-drop points: 1 point per row actively fallen while soft-drop is held — **only the last continuous soft-drop press before lock counts**, not the sum of multiple separate presses on the same piece. Capped implicitly by board height (~20 max).
- **Resolved edge case — soft-drop BCD bug**: real NES scoring has a documented bug from miscoded BCD (binary-coded decimal) arithmetic that can cause soft-drop points to not always be exactly 1/cell in every case. Decision: implement the clean, intended behavior (exactly 1 point per cell of the last continuous soft-drop press) rather than replicating the ROM bug — document this choice explicitly as a deliberate accuracy-vs-effort tradeoff, not an oversight.

### 2.6 Level-Up Thresholds
- First transition: `min(startLevel × 10 + 10, max(100, startLevel × 10 − 50))` lines cleared.
- After the first transition, level advances every 10 additional lines.
- Verified worked examples to encode as literal test cases: start level 5 → level 6 at 60 lines → level 7 at 70; start level 12 → level 13 at 100 → level 14 at 110; start level 16 → level 17 at 110 → level 18 at 120.

### 2.7 Randomizer — must be the real NES LFSR, not a substitute generator
- Roll 1: uniform 0–7 (8 values; 7 is a "dummy").
- If roll 1 matches the previous piece, or hits the dummy value (7): reroll with a uniform 0–6 (7 values), dealt unconditionally (no further rerolling).
- This suppresses but does not eliminate immediate repeats, and is **not** the source of any specific per-piece-type bias (e.g., no special disadvantage for the I-piece specifically) — long-run frequency converges to uniform (~1/7) per piece; the real effect is high short-run variance (long droughts are normal), not a persistent bias toward or against any one shape.
- **Underlying generator (Confirmed)**: The NES uses a 16-bit Fibonacci LFSR, seeded at $8988. The taps are bits 1 and 9 (0-indexed). The new highest bit is `bit1 ^ bit9`. The register is updated via a right-shift: `value = (((value >> 9) & 1) ^ ((value >> 1) & 1)) << 15 | (value >> 1)`. The LFSR state must be updated at least once per frame (including during delays, line clears, and menus), as well as during piece generation rolls. Implement this exact LFSR behavior, bit-for-bit, as `std::mt19937` will silently break real-replay validation (Section 6.4).
- For training use, note that LFSR state is a single small integer — this is good for the speed/copyability goal (Section 4): trivial to snapshot, restore, or fork per rollout.

### 2.8 Piece Definitions & Rotation
- 7 tetrominoes: I, O, T, S, Z, J, L.
- **Right-handed Nintendo Rotation System (NRS)**: O has 1 rotation state (no rotation). I has 2 states (horizontal favors the lower half of its bounding box, vertical favors the right half). J/L/T have 4 states, centered on the piece's middle block.
- **No wall kicks** — a rotation that would collide is simply rejected; piece stays in its current position/orientation. This must be an explicit rejection, not a silent no-op that could mask a bug elsewhere. Make the rejection loud in code (e.g. a distinct return value/state, not just a doc comment) so a contributor instinctively reaching for modern SRS-style kick tables doesn't accidentally introduce one.
- **Exact Spawn Position (Confirmed)**: All pieces spawn at `X=5, Y=0`. The coordinate offsets for the 7 spawn orientations (relative to X=5, Y=0) are:
  - T (down): `{ {-1, 0}, {0, 0}, {1, 0}, {0, 1} }`
  - J (down): `{ {-1, 0}, {0, 0}, {1, 0}, {1, 1} }`
  - Z (horiz): `{ {-1, 0}, {0, 0}, {0, 1}, {1, 1} }`
  - O: `{ {-1, 0}, {0, 0}, {-1, 1}, {0, 1} }`
  - S (horiz): `{ {0, 0}, {1, 0}, {-1, 1}, {0, 1} }`
  - L (down): `{ {-1, 0}, {0, 0}, {1, 0}, {-1, 1} }`
  - I (horiz): `{ {-2, 0}, {-1, 0}, {0, 0}, {1, 0} }`

### 2.9 Drop & Lock Delay
- **Soft Drop**: Holding Down increments a counter. When it reaches 3, a soft drop occurs (1 cell) and the counter resets to 1. The initial soft drop requires 3 frames, but repeats every 2 frames thereafter.
- **Lock Delay**: The lock delay is equal to the current drop delay (gravity). When a piece attempts to drop via gravity but cannot due to collision, it immediately locks on that frame. There is no independent lock delay timer like in modern SRS.
- **Opening Entry Delay**: The very first piece spawned in the game has an entry delay of 96 frames. A soft drop will cancel it, but shifting and rotating will not.

### 2.10 Board
- 10 columns × 22 total rows (top 2 rows are hidden/buffer, 20 visible rows).
- Row/column indexing convention must be documented explicitly and used consistently everywhere: MeatFighter notes the NES hardware uses negative hidden rows (y=-1, y=-2). To represent this cleanly in C++, we will standardize on a 0-indexed 10x22 array where the hidden buffer rows are `y=0` and `y=1`, and the visible rows are `y=2` to `y=21`. Therefore, **y = 0 is at the very top of the hidden buffer, increasing downward**. Document this exact convention prominently to prevent confusion.

---

## 3. Core Engine Components — Build in This Order

Write Catch2 tests alongside each component as it's built, not deferred to the end.

### 3.1 `engine/Timing`
Pure lookup tables/formulas: gravity-frames-per-row by level, DAS constants (initial delay, repeat rate, reset-to-10 behavior, freeze-but-retain during ARE/line-clear, and the confirmed-or-not blocked-shift-jumps-to-16 rule), ARE duration by lock height, line-clear delay.

### 3.2 `engine/Scoring`
Pure formulas: line-clear score by size and level (with the post-clear-level-multiplier-timing rule correctly implemented), soft-drop point calculation (last-continuous-press-only, capped), level-up threshold logic. **Explicitly separate from any RL reward concept** — this class only ever computes the true NES score; it has no knowledge of holes, bumpiness, or any other shaping concept, since those don't exist in real NES scoring. Reward shaping for the AI, if any, belongs entirely outside this library, one layer up in the training harness.

### 3.3 `engine/Piece`
7 tetromino shapes, NRS rotation states (no wall kicks — collision-blocked rotation attempts explicitly rejected), exact spawn position/orientation per piece.

### 3.4 `engine/Board`
10×20 grid + buffer rows, collision detection, line-clear identification (row-completion check), row-shift-down-on-clear logic, and native feature extraction (column heights, hole count, bumpiness, aggregate height — computed in a single efficient pass; this is exactly the kind of per-tick computation an AI's state representation will want, so it belongs in the fast core, cheaply computable, even though the engine has no opinion on how those features get used).

**Given a specific issue observed during downstream use of an earlier engine version** (a board that filled to near-maximum height without a single row ever completing), line-clear detection deserves extra scrutiny here: add a dedicated test that fills a row to exactly one cell short of complete, places a piece to complete it, and asserts the clear is detected and the row is correctly removed/shifted — not just tested on empty or simple boards.

### 3.5 `engine/Randomizer`
Authentic NES reroll-on-repeat/dummy-value algorithm, driven by the real NES LFSR (Section 2.7) — not `std::mt19937` or any other substitute — seeded explicitly (see Section 5 for full RNG handling).

### 3.6 `engine/Game`
Explicit state machine, not an implicit falling-piece loop:
```
SPAWNING → FALLING → LOCKING → LINE_CLEARING → ARE → SPAWNING (loop)
SPAWNING → GAME_OVER (on spawn collision)
```
- **`Input` is a per-frame snapshot of independently-held buttons, not a single mutually-exclusive value.** An earlier draft of this plan specified `tick(Input)` accepting exactly one of `{left, right, rotateCW, rotateCCW, softDrop, none}` per call — that's wrong: the real NES controller reports the state of all buttons every frame, and real (and TAS-optimal) play depends on pressing multiple buttons on the same frame (e.g. rotate while mid-DAS, or rotate while soft-dropping). `Input` should instead be a small struct/bitmask of independent booleans — `{left, right, rotateCW, rotateCCW, softDrop}` — all sampled the same frame, matching the real controller-read routine. `tick(Input)` still advances exactly one NES frame per call.
- Correctly handles: DAS charge freezing-but-retained during `LINE_CLEARING`/`ARE`; piece non-existence during those states; state-appropriate input handling throughout; independent per-frame handling of movement, rotation, and soft-drop given the corrected `Input` struct above.
- Exposes: `reset(seed)`, `tick(Input)`, `getState()`, `isGameOver()`, `getScore()`, `getLevel()`, `getLines()`.
- **Speed/training shape**: `Game` should hold its entire state as plain fixed-size data (board grid, current piece, RNG/LFSR state, timers, score/level/lines) with no heap-owned members and no virtual dispatch in the hot path, so it is cheaply copyable and `reset(seed)` is cheap enough to call in a tight loop. This is what makes running many instances in parallel (or resetting one instance thousands of times per second) fast later — see Section 4.

---

## 4. RNG, Determinism, and Parallel-Instance Speed

- Each `Game` instance owns its own LFSR state (Section 2.7) — no global/shared RNG, no `std::rand()`, no `random_device` used as the actual generator (only acceptable as a seed source, never for the reproducible in-game sequence).
- `Game::reset(seed)` re-initializes the `Randomizer`'s LFSR deterministically — same seed must always produce the same piece sequence and same game outcome given the same inputs.
- No part of the engine should fall back to a non-seeded/time-based random source anywhere outside of explicit seeding.
- Because the eventual use case is training an AI across many simulated games, `Game` state should be small, flat, and trivially copyable (see 3.6) so a training harness can cheaply run many independent instances (e.g. one per CPU thread, or many per thread if games are small enough) and cheaply fork/snapshot state for search or rollout strategies — this engine doesn't implement any of that itself, but its data layout shouldn't get in the way of it later.

---

## 5. Zero-Allocation Discipline

- The engine's hot path (`tick()`, board operations, randomizer rolls) must use fixed-size arrays/stack allocation only — no dynamic containers (`std::vector`, `std::string`, heap `new`) in the simulation loop. This is not just a style preference here: allocation in the hot path is a direct throughput cost for a training setup that wants to run this loop as many times per second as possible.
- Verify this directly, don't assume it: build a test that overrides `operator new`/`operator delete` (or uses an allocation-counting allocator) during a large batch of `tick()` calls and asserts the allocation count is exactly 0 (or documents precisely where non-zero allocation is acceptable, e.g., during one-time setup in `reset()` but not during `tick()`).
- Once correctness is established, add a simple throughput benchmark (ticks/sec for a single instance, Release build) as a sanity check — not a formal target for this phase, but a number worth having on hand before this engine gets wrapped for parallel training use.

---

## 6. Verification Plan

### 6.1 Unit tests (Catch2, fast, run constantly during development)
- `Timing`: gravity table lookup exactness per level; DAS timing including the reset-to-10 quirk, freeze-but-retain behavior, and the blocked-shift-jumps-to-16 rule once confirmed against a primary source; ARE duration by lock height; line-clear delay range.
- `Scoring`: exact score output for every documented clear-size/level combination; the post-clear-level-multiplier-timing edge case as its own explicit test; soft-drop point calculation (last-press-only rule, capped correctly); level-up threshold exactness against the three worked examples in Section 2.6.
- `Piece`: correct rotation states per NRS for all 7 shapes; strict, explicit rejection (not silent no-op) of collision-causing rotations; correct spawn position/orientation per piece.
- `Board`: collision detection; line-clear identification including the near-full-board edge case described in Section 3.4; row shift-down correctness after a clear; hole/bumpiness/height feature extraction correctness against hand-verified board states.
- `Randomizer`: the *exact* LFSR sequence and reroll-on-repeat/dummy-value logic (not just aggregate distribution) — verify bit-for-bit LFSR output against known reference values from a primary source, and separately verify over a large sample (e.g. 1,000,000 rolls) that long-run frequency converges toward uniform per piece and immediate repeats are suppressed but not eliminated.
- `Game`: state machine transition correctness (`SPAWNING → FALLING → LOCKING → LINE_CLEARING → ARE → SPAWNING`, and `→ GAME_OVER`); correct DAS-charge carry-over behavior across ARE/line-clear windows; correct handling of multiple simultaneously-held inputs on one frame (e.g. rotate + soft-drop + DAS shift all active the same tick).

### 6.2 Integration tests
- Full scripted `Game` sequences (`reset()` → series of actions → assert exact final score/level/lines/board state) — catches bugs in how correctly-tested components interact, not just their individual correctness.

### 6.3 Golden/reference-value tests
- Hardcoded assertions pulled directly from `docs/nes_rules_reference.md` — e.g., "at level 18, gravity is X frames/row," "clearing a tetris at level 5 awards exactly Y points" — cheap, high-value, and this suite is what substantiates the "NES-accurate" claim specifically.

### 6.4 Real-replay validation (highest-value, pursue actively rather than deferring indefinitely)
- Find a real recorded NES Tetris input log (community TAS tools/tracking sites publish frame-perfect input logs with known final scores) and feed it through the engine, asserting the final score/board state matches. This now depends on the LFSR fix in Section 2.7/3.5 — without the real LFSR (seeded and advanced exactly as on hardware) the piece sequence won't match the log and this test cannot pass regardless of how correct the rest of the engine is. This is the strongest possible correctness signal available, since it validates against real hardware rather than only against documented rules.

### 6.5 Safety/robustness testing
- **ASan + LeakSanitizer**: run a large batch of frames (e.g., 1,000,000) through a single long-lived `Game` instance, and separately, a loop that repeatedly constructs/`reset()`s/destroys many `Game` instances (catches allocation-lifecycle bugs a single long-running session wouldn't) — this second pattern is also the closest single-threaded proxy to the many-resets-per-second usage a training harness will eventually put this through.
- **UBSan**: same runs, catches undefined behavior.
- **ThreadSanitizer**: a separate sanitizer build running multiple `Game` instances concurrently on separate threads — this is directly relevant to the eventual training use case (parallel simulated games), not just a stricter code-quality check; verify deterministic, isolated outcomes per instance with zero shared/stray global or static state.
- **Fuzz testing**: fire a large budget (e.g., 10M+) of randomized frame-inputs (using the corrected multi-button `Input` struct, including combinations no human would produce) across many randomized episodes at the engine under ASan/UBSan, checking for crashes, hangs, or out-of-bounds board state — this matters directly because an AI driving this engine will generate exactly these kinds of inputs.

### 6.6 Manual verification
- Build `tetris_render` (raylib) and playtest it directly via keyboard. Confirm DAS charge behavior, gravity pacing at various levels, and the absence of wall kicks all feel authentic to real NES Tetris. Don't skip this just because automated tests pass — unit tests only catch what you thought to assert.

---

## 7. Definition of Done

- Every constant in `docs/nes_rules_reference.md` is sourced/cited and mirrored exactly in `Timing`/`Scoring`, including the DAS blocked-shift-jumps-to-16 rule confirmed (not assumed) against a primary source.
- Full Catch2 suite passes: unit, integration, and golden/reference-value tests.
- At least one real recorded-replay validation completed and passing, using the real NES LFSR (not a substitute PRNG).
- `Input` is implemented as a multi-button per-frame snapshot, not a single mutually-exclusive value, and tests cover simultaneous-input frames.
- ASan/LSan/UBSan/TSan all clean; zero-heap-allocation in the hot path explicitly verified, not assumed.
- Fuzz test run completes with no crashes/hangs/out-of-bounds state.
- `tetris_render` manually playtested and confirmed to feel authentic.
- `Game` state is flat, fixed-size, and cheaply copyable/resettable, and a basic single-instance ticks/sec benchmark has been recorded as a baseline for future parallel-training use.
- The three deliberate/flagged decisions are documented explicitly, not left implicit: post-clear level-multiplier timing, soft-drop BCD-bug non-replication, and the LFSR-vs-substitute-PRNG choice (and why it matters for replay validation).