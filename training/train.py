import sys
import os
import cma
import numpy as np
import pickle
import csv
from concurrent.futures import ProcessPoolExecutor
import time

sys.path.append(os.path.abspath('../build'))

try:
    import tetris_core
except ImportError as e:
    print("Failed to import tetris_core. Ensure it is built in the 'build' directory.")
    print("Error:", e)
    sys.exit(1)

# CRN: Use 10 fixed seeds for all candidates in a generation to reduce variance
FIXED_SEEDS = [0x1234, 0x5678, 0x9ABC, 0xDEF0, 0x1357, 0x2468, 0x1111, 0x2222, 0x3333, 0x4444]
MAX_PIECES_PER_GAME = 10000

CHECKPOINT_FILE = 'checkpoint.pkl'
LOG_FILE = 'training_log.csv'

def run_seeded_game(weights, seed, max_pieces):
    game = tetris_core.Game(startLevel=0, seed=seed)
    searcher = tetris_core.MoveSearch()

    pieces_placed = 0

    # Fast-forward opening delay
    null_input = tetris_core.Input()
    while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
        game.tick(null_input)

    while not game.isGameOver() and pieces_placed < max_pieces:
        placements = searcher.getValidPlacements(game)
        if not placements:
            break

        best_score = -float('inf')
        best_placement = None
        
        board = game.getBoard()
        piece_type = game.getActivePieceType()

        for p in placements:
            features = tetris_core.extract_features(board, piece_type, p.rotation, p.x, p.y)
            # Dot product
            score = sum(w * f for w, f in zip(weights, features))
            if score > best_score:
                best_score = score
                best_placement = p

        if best_placement:
            for input_state in best_placement.inputs:
                game.tick(input_state)
            
            # Fast-forward until the piece locks
            null_input.softDrop = True
            while game.getState() == tetris_core.GameState.PLAYING and not game.isGameOver():
                game.tick(null_input)
            
            # Fast-forward through delays
            null_input.softDrop = False
            while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
                game.tick(null_input)
            
            pieces_placed += 1
        else:
            break

    # Fitness: We want to MAXIMIZE lines cleared, so we return NEGATIVE lines_cleared for CMA-ES (minimizer)
    return game.getLines()

def evaluate_candidate(candidate_weights):
    # Normalize weights as per user request to prevent magnitude explosion
    w_arr = np.array(candidate_weights)
    norm = np.linalg.norm(w_arr)
    if norm > 0:
        w_arr = w_arr / norm
    
    total_lines = 0
    print(f"  Evaluating candidate {candidate_weights[:3]}...")
    for seed in FIXED_SEEDS:
        # print(f"    Seed {hex(seed)}...")
        total_lines += run_seeded_game(w_arr.tolist(), seed, MAX_PIECES_PER_GAME)
        
    avg_lines = total_lines / len(FIXED_SEEDS)
    print(f"  Candidate finished. Avg lines: {avg_lines}")
    return -avg_lines

def main():
    print("Starting Phase 3 CMA-ES Training Loop (Learn from Scratch)")
    
    generation = 1
    es = None
    
    if os.path.exists(CHECKPOINT_FILE):
        print(f"Resuming from {CHECKPOINT_FILE}...")
        with open(CHECKPOINT_FILE, 'rb') as f:
            es = pickle.load(f)
        
        # Figure out the generation count from the log file
        if os.path.exists(LOG_FILE):
            with open(LOG_FILE, 'r') as f:
                lines = f.readlines()
                if len(lines) > 1:
                    last_line = lines[-1].strip().split(',')
                    generation = int(last_line[0]) + 1
    else:
        print("Starting Phase 3 fresh tuning run from scratch ([0.0]*9)...")
        initial_mean = [0.0] * 9
        initial_sigma = 1.0
        
        es = cma.CMAEvolutionStrategy(initial_mean, initial_sigma, {
            'popsize': 16,
            'maxiter': 50  # 50 generations for overnight run
        })
        
        # Initialize log file
        with open(LOG_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Generation', 'BestFitness', 'MedianFitness', 'WorstFitness', 'Time'])

    # Use max_workers=8 since M1 has 8 cores
    with ProcessPoolExecutor(max_workers=8) as executor:
        while not es.stop():
            print(f"--- Generation {generation} ---", flush=True)
            gen_start_time = time.time()
            
            # Ask for candidates
            candidates = es.ask()
            
            # Evaluate in parallel
            fitnesses = list(executor.map(evaluate_candidate, candidates))
            
            gen_time = time.time() - gen_start_time
            
            # Tell the optimizer the results
            es.tell(candidates, fitnesses)
            
            # Log the best
            best_fitness = np.min(fitnesses)
            median_fitness = np.median(fitnesses)
            worst_fitness = np.max(fitnesses)
            
            best_lines = -best_fitness
            mean_lines = -median_fitness
            worst_lines = -worst_fitness
            
            print(f"=> Generation {generation} | Best: {best_lines:.2f} | Median: {mean_lines:.2f} | Time: {gen_time:.2f}s", flush=True)
            
            # Save to CSV
            with open(LOG_FILE, 'a', newline='') as f:
                writer = csv.writer(f)
                writer.writerow([generation, best_lines, mean_lines, worst_lines, gen_time])
                
            # Checkpoint
            with open(CHECKPOINT_FILE, 'wb') as f:
                pickle.dump(es, f)
            
            generation += 1

    print("Phase 5 tuning complete.")

if __name__ == "__main__":
    main()
