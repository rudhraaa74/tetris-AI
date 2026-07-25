import sys
import os
import time
import cProfile
import pstats
import numpy as np

sys.path.append(os.path.abspath('../build'))
import tetris_core

def evaluate_candidate_profile():
    # Provide a decent set of weights so it plays for a while
    weights = [-4.5, 3.2, -3.2, -9.3, -7.9, -3.3, -4.8, -3.3, 2.2]
    w_arr = np.array(weights)
    norm = np.linalg.norm(w_arr)
    w_arr = (w_arr / norm).tolist()

    seed = 42
    max_pieces = 1000
    
    game = tetris_core.Game(startLevel=0, seed=seed)
    searcher = tetris_core.MoveSearch()
    pieces_placed = 0

    null_input = tetris_core.Input()
    while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
        game.tick(null_input)

    while not game.isGameOver() and pieces_placed < max_pieces:
        placements = searcher.getValidPlacements(game)
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

def main():
    start_t = time.time()
    evaluate_candidate_profile()
    end_t = time.time()
    
    
    print(f"Total time for 1000 pieces: {end_t - start_t:.3f} seconds")

if __name__ == "__main__":
    main()
