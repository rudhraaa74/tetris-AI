import os
import sys
import time

# Add the build directory to the Python path
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "build"))

import tetris_core

# Use the weights that reached ~4000 lines
best_weights = [
    -4.28190302,
    5.09973296,
    -6.20119369,
    -8.17077911,
    -3.86185653,
    -4.20967858,
    -3.00149556,
    -8.15636664,
    2.62364571
]

def main():
    print("Testing 2-piece lookahead engine in C++...")
    seed = int(time.time() * 1000) % 65536
    level = 19
    
    start_time = time.time()
    
    # Run a seeded game completely in C++ with depth 2 search
    score, lines, moves = tetris_core.run_seeded_game_depth2(seed, best_weights, level, max_moves=-1)
    
    end_time = time.time()
    elapsed = end_time - start_time
    
    print(f"Seed: {seed}")
    print(f"Level: {level}")
    print(f"Moves Played: {moves}")
    print(f"Lines Cleared: {lines}")
    print(f"Score: {score}")
    print(f"Time Taken: {elapsed:.2f} seconds")
    
    if lines > 0:
        print(f"Performance: {lines / elapsed:.2f} lines / second")

if __name__ == "__main__":
    main()
