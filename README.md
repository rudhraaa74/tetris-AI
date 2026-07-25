# Autonomous High-Performance Tetris AI Engine 🧩⚡

![Tetris AI Demo](demo.gif)

A C++20 and Python hybrid Tetris AI engine utilizing 2-piece lookahead (Depth-2) search, 9 BCTS heuristic features, and CMA-ES evolutionary optimization. Engineered for hardware efficiency on Apple Silicon, achieving over **955 Million NES Points** and **950,000+ Lines Cleared** in an uncapped benchmark run.

## 🌟 Highlights & Achievements

- **955,934,023+ NES Points & 950,000+ Lines**: Cleared in an uncapped, immortal benchmark run without topping out once.
- **C++ Core Engine with pybind11 Bindings**: Sub-millisecond evaluation speed (~9,300 node state evaluations/second per core).
- **2-Piece Lookahead (Depth-2 Search)**: Simulates all $N_1 \times N_2 \approx 30 \times 30 = 900$ branch node states per placement to eliminate fatal S/Z piece traps.
- **Apple Silicon Hardware Tuning**: Optimized for heterogeneous architectures by isolating worker processes exclusively to Firestorm Performance cores (P-cores).
- **CMA-ES Evolutionary Optimization**: Multi-seed heuristic weight optimization over 9 BCTS (Bertsekas-Tsitsiklis) domain features.
- **Real-Time Raylib Visualizer**: A natively compiled, blazing fast C++ visualizer built with Raylib to watch the AI play at 60+ FPS.

---

## 🏗️ System Architecture

The AI is built as a multi-tier hybrid architecture: Python orchestrates evolutionary training loops and benchmarks, while C++ handles computationally intensive move searches, state evaluations, and high-performance rendering.

```text
       ┌────────────────────────────────────────────────────────┐
       │             Python High-Level Orchestrator             │
       │        (CMA-ES Evolutionary Training, Benchmark)       │
       └───────────────────────────┬────────────────────────────┘
                                   │ pybind11
       ┌───────────────────────────▼────────────────────────────┐
       │               C++ Core Engine (`tetris_core`)          │
       ├────────────────────────────────────────────────────────┤
       │  • Depth-2 Lookahead Engine (`getBestPlacementDepth2`) │
       │  • Bitwise Board Matrix & Kinematic Transition Rules   │
       │  • BCTS 9-Feature Heuristic Evaluator                  │
       └───────────────────────────┬────────────────────────────┘
                                   │ Raylib
       ┌───────────────────────────▼────────────────────────────┐
       │             Native C++ Graphical Visualizer            │
       └────────────────────────────────────────────────────────┘
```

---

## 🧠 Depth-2 Lookahead Strategy

While 1-piece lookahead (Depth-1) evaluates only ~30 terminal states for the current falling piece, Depth-2 evaluates every valid placement for the current piece *plus* every valid placement for the upcoming piece:

`Evaluations per Move ≈ 30 × 30 = 900 states`

This depth allows the engine to intentionally create temporary surface irregularity or "bumpiness" if it detects that the upcoming piece can seamlessly resolve the gap, completely eliminating "S/Z piece droughts" that trap 1-piece agents.

## 📊 Heuristic Feature Weights (BCTS Model)

The evaluation function computes a weighted linear combination of 9 key board quality features:

| Index | Feature Name | Depth-1 Weight | Depth-2 Fine-Tuned | Tactical & Strategic Shift |
|-------|-------------|----------------|--------------------|-----------------------------|
| 0 | Aggregate Height | -4.2819 | -4.1770 | Cares slightly less about height; relies on future lookahead to clear stack. |
| 1 | Complete Lines | +5.0997 | +5.1668 | Slightly higher reward for clearing lines. |
| 2 | Holes | -6.2012 | -6.2547 | Higher penalty; realizes buried holes are unfixable two steps ahead. |
| 3 | Bumpiness | -8.1708 | -7.9382 | Relaxes penalty (+0.24); deliberately creates jagged surfaces if next piece fits. |
| 4 | Pit Depth | -3.8619 | -3.8958 | Stronger penalty against deep 1-wide vertical chasms. |
| 5 | Column Transitions | -4.2097 | -4.3628 | Stricter penalty for vertical surface jaggedness. |
| 6 | Row Transitions | -3.0015 | -2.7536 | Relaxes penalty (+0.25); less afraid of horizontal gaps fixed next turn. |
| 7 | Hole Depth | -8.1564 | -8.2920 | Increases penalty for deep, covered empty blocks. |
| 8 | Well Depth | +2.6236 | +2.5416 | Maintains a 1-column open well for I-bar Tetris clears. |

---

## ⚡ Apple Silicon Performance Optimization

Standard Python `ProcessPoolExecutor` setups suffer from severe synchronization bottlenecks on ARM heterogeneous architectures (e.g., Apple M1/M2/M3 chips) due to the "Straggler Effect":

- **The Issue**: Equal chunking distributes work across both Performance (P-Cores) and Efficiency (E-Cores). Fast P-cores finish quickly and sit idle at a synchronization barrier, while slow, power-throttled E-cores take roughly 10x longer to finish.
- **The Fix**: By enforcing `max_workers = 4` and isolating processes strictly to P-cores, generation evaluation times plummeted, resulting in a **4.5x to 5x speedup**.

---

## 📁 Repository Structure

```text
├── engine/                   # Core C++ mechanics, MoveSearch, and BCTS evaluator
├── render/                   # Raylib-based native C++ graphical visualizer
├── bindings/                 # pybind11 C++ module definitions (`tetris_bindings.cpp`)
├── training/                 # Python scripts
│   ├── train_depth2.py       # CMA-ES evolutionary training script
│   └── run_final_benchmark.py# Multi-core uncapped benchmark runner
├── output/                   # Checkpoints and active training logs
├── CMakeLists.txt            # CMake build configuration
└── README.md                 # Project documentation
```

---

## 🚀 Getting Started

### Prerequisites
- **C++ Compiler**: `clang++` or `g++` supporting C++20.
- **Python**: 3.9+
- **Build Tools**: `cmake`, `make`
- **Dependencies**: `pybind11`, `cma`, `numpy`, `raylib` (fetched automatically by CMake)

### 1. Installation & Compilation
Clone the repository and compile the native C++ Python extension module and the Raylib visualizer:

```bash
# Install required Python packages
pip install cma numpy

# Generate build files and compile the project
mkdir -p build && cd build
cmake ..
make -j4
```

### 2. Watch the AI Play (Native C++ Visualizer)
Watch the Depth-2 AI play live with blazing fast graphics natively rendered in C++:

```bash
./build/tetris_render
```
- **`A`**: Toggle AI Autopilot ON/OFF
- **`R`**: Reset the board

### 3. Run the Multi-Core Benchmark
Run 4 concurrent games (one per Performance core) to evaluate real-time search throughput and line clears:

```bash
python3 training/run_final_benchmark.py
```

## 📜 License
Distributed under the MIT License. See `LICENSE` for more information.