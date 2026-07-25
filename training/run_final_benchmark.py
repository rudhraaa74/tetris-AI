import os
os.environ["OMP_NUM_THREADS"] = "1"
os.environ["OPENBLAS_NUM_THREADS"] = "1"
os.environ["MKL_NUM_THREADS"] = "1"
os.environ["VECLIB_MAXIMUM_THREADS"] = "1"
os.environ["NUMEXPR_NUM_THREADS"] = "1"
os.environ["PYTHONUNBUFFERED"] = "1"

import sys
import json
import time
import pickle
import numpy as np
from concurrent.futures import ProcessPoolExecutor

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "build"))
import tetris_core

MAX_PIECES = -1  # -1 means absolutely no cap
SEEDS = [42, 101, 777, 99999]
LEVEL = 0  # Starting from the beginning of the game

def load_weights():
    # Attempt to load from JSON if it exists
    if os.path.exists("best_weights_depth2.json"):
        with open("best_weights_depth2.json", "r") as f:
            return json.load(f)
    # Attempt to load from the recent checkpoint
    if os.path.exists("output/checkpoint_depth2.pkl"):
        with open("output/checkpoint_depth2.pkl", "rb") as f:
            es, _ = pickle.load(f)
            return es.result.xbest.tolist()
    # Fallback to depth 1
    if os.path.exists("best_weights.json"):
        with open("best_weights.json", "r") as f:
            return json.load(f)
    
    # Absolute fallback
    return [-4.28, 5.10, -6.20, -8.17, -3.86, -4.21, -3.00, -8.16, 2.62]

def run_game(args):
    seed, weights, core_id = args
    print(f"-> Launching Core {core_id} on Seed {seed}...", flush=True)
    
    t0 = time.time()
    # C++ takes (seed, weights, level, max_moves, core_id)
    score, lines, tetrises, moves = tetris_core.run_seeded_game_depth2(
        seed, weights, LEVEL, MAX_PIECES, core_id
    )
    t1 = time.time()
    
    return {
        "seed": seed,
        "score": score,
        "lines": lines,
        "tetrises": tetrises,
        "moves": moves,
        "duration": t1 - t0,
        "core_id": core_id
    }

def main():
    print("=== FINAL 4-CORE GRANDMASTER BENCHMARK ===", flush=True)
    weights = load_weights()
    
    w_arr = np.array(weights)
    norm = np.linalg.norm(w_arr)
    if norm > 0:
        w_arr = w_arr / norm
    norm_weights = w_arr.tolist()
    
    args = [(seed, norm_weights, idx + 1) for idx, seed in enumerate(SEEDS)]
    
    global_start = time.time()
    
    results = []
    with ProcessPoolExecutor(max_workers=4) as executor:
        for res in executor.map(run_game, args):
            results.append(res)
            
    global_duration = time.time() - global_start
    
    # Aggregation
    best_score = max(r['score'] for r in results)
    best_lines = max(r['lines'] for r in results)
    avg_score = np.mean([r['score'] for r in results])
    avg_lines = np.mean([r['lines'] for r in results])
    
    total_tetrises = sum(r['tetrises'] for r in results)
    total_lines = sum(r['lines'] for r in results)
    total_pieces = sum(r['moves'] for r in results)
    
    overall_trt = (total_tetrises * 4) / total_lines * 100 if total_lines > 0 else 0
    overall_throughput = total_pieces / global_duration if global_duration > 0 else 0
    
    print("\n" + "="*50, flush=True)
    print(" FINAL BENCHMARK SUMMARY ", flush=True)
    print("="*50, flush=True)
    for r in results:
        print(f" Core {r['core_id']} (Seed {r['seed']}): {r['score']} pts | {r['lines']} lines | {r['moves']} pieces", flush=True)
    print("-" * 50, flush=True)
    print(f" Best Score      : {best_score}", flush=True)
    print(f" Highest Lines   : {best_lines}", flush=True)
    print(f" Average Score   : {avg_score:.1f}", flush=True)
    print(f" Average Lines   : {avg_lines:.1f}", flush=True)
    print(f" Overall TRT     : {overall_trt:.2f}%", flush=True)
    print(f" Total Pieces    : {total_pieces}", flush=True)
    print(f" Total Lines     : {total_lines}", flush=True)
    print(f" Engine Speed    : {overall_throughput:.1f} moves/sec (combined)", flush=True)
    print("="*50, flush=True)

if __name__ == "__main__":
    main()
