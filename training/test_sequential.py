import sys
import os
import time
import numpy as np

sys.path.append(os.path.abspath('../build'))
import tetris_core

def evaluate_candidate(w_arr):
    game = tetris_core.Game(startLevel=0, seed=42)
    searcher = tetris_core.MoveSearch()
    pieces_placed = 0
    null_input = tetris_core.Input()
    
    while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
        game.tick(null_input)

    while not game.isGameOver() and pieces_placed < 500:
        print(f"  Evaluating piece {pieces_placed}...")
        placements = searcher.getValidPlacements(game)
        print(f"  Found {len(placements)} placements.")
        if not placements: break

        best_score = -float('inf')
        best_placement = None
        board = game.getBoard()
        piece_type = game.getActivePieceType()

        for p in placements:
            features = tetris_core.extract_features(board, piece_type, p.rotation, p.x, p.y)
            score = sum(w * f for w, f in zip(w_arr, features))
            if score > best_score:
                best_score = score
                best_placement = p

        if best_placement:
            for input_state in best_placement.inputs:
                game.tick(input_state)
            
            null_input.softDrop = True
            while game.getState() == tetris_core.GameState.PLAYING and not game.isGameOver():
                game.tick(null_input)
            
            null_input.softDrop = False
            while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
                game.tick(null_input)
            
            pieces_placed += 1
        else:
            break
    return pieces_placed

def main():
    np.random.seed(42)
    start_t = time.time()
    for i in range(160):
        print(f"Starting candidate {i}...")
        w = np.random.normal(0.0, 1.0, 9)
        pieces = evaluate_candidate(w)
        print(f"Candidate {i} survived {pieces} pieces!")
    print(f"Finished 160 games in {time.time() - start_t:.2f}s")

if __name__ == "__main__":
    main()
