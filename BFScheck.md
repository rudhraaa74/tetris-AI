Role: Senior C++ Systems Engineer & Retrogaming Physics Specialist

Task: Write a comprehensive, zero-allocation unit test suite and micro-benchmark harness for our NES Tetris `MoveSearch` BFS implementation (`MoveSearch.h` / `MoveSearch.cpp`). 

We need to verify that:
1. Zero dynamic heap allocations occur during search execution.
2. The Pareto dominance pruning (`best_frames`, `best_das`) in `VisitedMeta` never falsely prunes valid reachable placements.
3. The NES kinematic constraints (DAS charge, 0-frame lock delay, 1G gravity) are strictly respected.
4. The search executes within optimal microsecond-level latency boundaries.

---

### Test Suite Requirements (`tests/test_movesearch_stress.cpp`)

Please implement the following test categories:

#### 1. Zero-Allocation Verification
* Override global `operator new` and `operator delete` within the test file.
* Wrap calls to `MoveSearch::search()` in an allocation assertion block.
* Assert that **zero allocations** occur across 10,000 consecutive search iterations on varying board states.

#### 2. Kinematic & Reachability Sanity Tests
* **Open Board Placements:**
  * Test `O`-piece on an empty board: Verify exact count and unique placement positions.
  * Test `I`-piece on Level 0: Verify all vertical and horizontal orientations across all valid columns.
* **Level 29 (1G Gravity) Reachability Bounding:**
  * Set gravity to 1 frame per cell (Level 29).
  * Spawn an `I`-piece at the top center.
  * Assert that the piece **cannot physically reach** column 0 or column 9 before locking due to gravity pull rate and 16-frame DAS latency.
* **Overhang Tucks & Slides:**
  * Construct a board with a overhang (e.g., a 1x1 notch under a solid block on column 0, row 0).
  * Test a `J` or `L` piece sliding into the notch under Level 0 and Level 18 gravity.
  * Verify that the tuck is successfully discovered when DAS is charged, but correctly fails if gravity pulls the piece past the notch entrance before rotation/slide completes.

#### 3. Pareto Dominance Pruning Safety (`VisitedMeta`)
* Construct a test case where two paths reach position $(x, y, r)$:
  * **Path A:** Arrives in 10 frames with DAS charge = 0.
  * **Path B:** Arrives in 14 frames with DAS charge = 16 (fully charged).
* Verify that Path B is **NOT pruned** by Path A, because its higher DAS charge enables a subsequent horizontal slide that Path A cannot achieve.
* Assert that a path arriving *later* with *equal or worse* DAS charge is aggressively pruned.

#### 4. Micro-Benchmark Harness
* Implement a high-resolution timer (`std::chrono::high_resolution_clock`) measuring 100,000 search executions across a randomized distribution of complex board states.
* Output key performance metrics:
  * Average latency per search ($\mu s$).
  * P99 / Maximum latency ($\mu s$).
  * Searches per second (Target: $> 100,000$ searches/sec on a single CPU core).

#### 5. Code & Memory Optimization Audit
* Inspect `MoveSearch.h` and `MoveSearch.cpp` for potential performance bottlenecks:
  * Verify all structs (e.g., `KinematicState`, `Placement`) are cache-aligned and pack into small bit-fields or small primitive types if applicable.
  * Verify `VisitedMeta` lookup arrays fit inside L1 data cache ($< 32\text{ KB}$).
  * Ensure hot loops use `noexcept`, `constexpr`, or pass-by-reference where applicable.

Generate the complete C++ test file and update `CMakeLists.txt` accordingly. Run the tests and report the benchmark metrics and assertion results.