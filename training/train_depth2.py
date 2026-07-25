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
import csv
from concurrent.futures import ProcessPoolExecutor
import time
import json

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "build"))

try:
    import tetris_core
except ImportError as e:
    print("Failed to import tetris_core. Ensure it is built in the 'build' directory.")
    print("Error:", e)
    sys.exit(1)

# Depth-2 Fine-Tuning Parameters
MAX_PIECES_PER_GAME = 500
FIXED_SEEDS = [101, 202]
LEVEL = 19
POPSIZE = 10
MAXITER = 10
SIGMA0 = 0.1

CHECKPOINT_FILE = 'checkpoint_depth2.pkl'
LOG_FILE = 'training_log_depth2.csv'
FINAL_WEIGHTS_FILE = 'best_weights_depth2.json'

# Default to 1-piece master weights if json doesn't exist
DEFAULT_WEIGHTS = [
    -4.28190302, 5.09973296, -6.20119369, -8.17077911,
    -3.86185653, -4.20967858, -3.00149556, -8.15636664, 2.62364571
]

def load_initial_weights():
    if os.path.exists('best_weights.json'):
        with open('best_weights.json', 'r') as f:
            return json.load(f)
    return DEFAULT_WEIGHTS

def evaluate_candidate(args):
    candidate_weights, gen, cand_idx = args
    
    print(f"[Gen {gen}/10] [Candidate {cand_idx}/10] Starting seeds {FIXED_SEEDS}...", flush=True)
    start_t = time.time()
    
    # Normalize weights to prevent magnitude explosion
    w_arr = np.array(candidate_weights)
    norm = np.linalg.norm(w_arr)
    if norm > 0:
        w_arr = w_arr / norm
    
    total_score = 0
    total_lines = 0
    total_tetrises = 0
    
    for seed in FIXED_SEEDS:
        score, lines, tetrises, moves = tetris_core.run_seeded_game_depth2(
            seed, w_arr.tolist(), LEVEL, max_moves=MAX_PIECES_PER_GAME
        )
        total_score += score
        total_lines += lines
        total_tetrises += tetrises
        
    avg_score = total_score / len(FIXED_SEEDS)
    avg_lines = total_lines / len(FIXED_SEEDS)
    
    # Calculate Tetris Rate: (Tetrises * 4) / Lines
    tetris_rate = (total_tetrises * 4) / total_lines if total_lines > 0 else 0
    
    duration = time.time() - start_t
    print(f"[Gen {gen}/10] [Candidate {cand_idx}/10] Finished in {duration:.1f}s | Avg Score: {avg_score:.1f} | Avg Lines: {avg_lines:.1f}", flush=True)
    
    # Return (fitness_for_cmaes, actual_avg_score, actual_avg_lines, tetris_rate)
    return -avg_score, avg_score, avg_lines, tetris_rate

def main():
    print("Starting Phase 6 CMA-ES Depth-2 Fine-Tuning", flush=True)
    
    initial_weights = load_initial_weights()
    print("Initial weights:", initial_weights, flush=True)
    
    generation = 1
    es = None
    
    if os.path.exists(CHECKPOINT_FILE):
        print(f"Resuming from {CHECKPOINT_FILE}...", flush=True)
        with open(CHECKPOINT_FILE, 'rb') as f:
            es, generation = pickle.load(f)
    else:
        print("Starting fresh CMA-ES...", flush=True)
        es = cma.CMAEvolutionStrategy(initial_weights, SIGMA0, {'popsize': POPSIZE})
        
        # Initialize log
        with open(LOG_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["Generation", "BestScore", "MedianScore", "WorstScore", "AvgLines", "TetrisRate", "TimeTaken"])
            
    while not es.stop() and generation <= MAXITER:
        gen_start_time = time.time()
        print(f"\n=== STARTING GENERATION {generation} / 10 ===", flush=True)
        
        candidates = es.ask()
        
        scores_for_cma = []
        actual_scores = []
        actual_lines = []
        
        # Prepare arguments for multiprocessing
        eval_args = [(c, generation, idx + 1) for idx, c in enumerate(candidates)]
        
        with ProcessPoolExecutor(max_workers=4) as executor:
            results = list(executor.map(evaluate_candidate, eval_args))
            
        for cma_score, real_score, real_lines, tetris_rate in results:
            scores_for_cma.append(cma_score)
            actual_scores.append(real_score)
            actual_lines.append(real_lines)
            
        es.tell(candidates, scores_for_cma)
        
        best_idx = np.argmin(scores_for_cma)
        best_score = actual_scores[best_idx]
        best_lines = actual_lines[best_idx]
        best_tetris_rate = results[best_idx][3]
        
        median_score = np.median(actual_scores)
        worst_score = np.min(actual_scores)
        
        gen_time = time.time() - gen_start_time
        
        print(f"Gen {generation} Stats:", flush=True)
        print(f"  Best Score: {best_score:.2f} (Avg Lines: {best_lines:.2f}, TRT: {best_tetris_rate*100:.1f}%)", flush=True)
        print(f"  Median Score: {median_score:.2f}", flush=True)
        print(f"  Worst Score: {worst_score:.2f}", flush=True)
        print(f"  Time: {gen_time:.2f}s", flush=True)
        
        # Log to CSV
        with open(LOG_FILE, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([generation, best_score, median_score, worst_score, np.mean(actual_lines), best_tetris_rate, gen_time])
            
        # Checkpoint
        with open(CHECKPOINT_FILE, 'wb') as f:
            pickle.dump((es, generation + 1), f)
            
        generation += 1

    print("\nFine-tuning complete!", flush=True)
    best_final_weights = es.result.xbest.tolist()
    
    # Normalize final weights before saving
    w_arr = np.array(best_final_weights)
    norm = np.linalg.norm(w_arr)
    if norm > 0:
        w_arr = w_arr / norm
    
    with open(FINAL_WEIGHTS_FILE, 'w') as f:
        json.dump(w_arr.tolist(), f)
        
    print(f"Saved optimal Depth-2 weights to {FINAL_WEIGHTS_FILE}")

if __name__ == "__main__":
    main()
