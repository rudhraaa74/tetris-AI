import os
os.environ["OMP_NUM_THREADS"] = "1"
os.environ["OPENBLAS_NUM_THREADS"] = "1"
os.environ["MKL_NUM_THREADS"] = "1"
os.environ["VECLIB_MAXIMUM_THREADS"] = "1"
os.environ["NUMEXPR_NUM_THREADS"] = "1"

import sys
import cma
import numpy as np
import pickle
import time

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "build"))
import tetris_core

MAX_PIECES = 500
LEVEL = 19
OLD_SEEDS = [101, 202]
NEW_SEEDS = [404, 505]
CHECKPOINT_FILE = 'checkpoint_depth2.pkl'

def evaluate(weights, seeds):
    w_arr = np.array(weights)
    norm = np.linalg.norm(w_arr)
    if norm > 0:
        w_arr = w_arr / norm
        
    scores = []
    lines_list = []
    
    for seed in seeds:
        print(f"Running seed {seed}...", flush=True)
        t0 = time.time()
        score, lines, tetrises, moves = tetris_core.run_seeded_game_depth2(
            seed, w_arr.tolist(), LEVEL, max_moves=MAX_PIECES
        )
        t1 = time.time()
        trt = (tetrises * 4) / lines * 100 if lines > 0 else 0
        print(f"  -> Score: {score}, Lines: {lines}, TRT: {trt:.1f}%, Time: {t1-t0:.1f}s", flush=True)
        scores.append(score)
        lines_list.append(lines)
        
    return np.mean(scores), np.mean(lines_list)

def main():
    if not os.path.exists(CHECKPOINT_FILE):
        print("No checkpoint found.")
        return
        
    with open(CHECKPOINT_FILE, 'rb') as f:
        es, gen = pickle.load(f)
        
    best_weights = es.result.xbest
    print(f"Loaded best candidate weights from Generation {gen-1} checkpoint.")
    
    print("\n=== Evaluating on TRAINING Seeds [101, 202] ===")
    old_score, old_lines = evaluate(best_weights, OLD_SEEDS)
    print(f"Avg Training Score: {old_score:.1f}, Avg Lines: {old_lines:.1f}")
    
    print("\n=== Evaluating on UNSEEN Seeds [404, 505] ===")
    new_score, new_lines = evaluate(best_weights, NEW_SEEDS)
    print(f"Avg Unseen Score: {new_score:.1f}, Avg Lines: {new_lines:.1f}")
    
if __name__ == "__main__":
    main()
