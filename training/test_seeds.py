import sys
import os
import time
import numpy as np

sys.path.append(os.path.abspath('../build'))
import tetris_core

FIXED_SEEDS = [0x1234, 0x5678, 0x9ABC, 0xDEF0, 0x1357, 0x2468, 0x1111, 0x2222, 0x3333, 0x4444]

def evaluate_seed(seed):
    print(f"  Testing seed {hex(seed)}...")
    game = tetris_core.Game(startLevel=0, seed=seed)
    searcher = tetris_core.MoveSearch()
    pieces_placed = 0
    null_input = tetris_core.Input()
    
    while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
        game.tick(null_input)

    while not game.isGameOver() and pieces_placed < 1000:
        print(f"    Piece {pieces_placed}...")
        placements = searcher.getValidPlacements(game)
        if not placements: break

        best_score = -float('inf')
        best_placement = None
        board = game.getBoard()
        piece_type = game.getActivePieceType()

        # Random weights
        weights = [0.0] * 9

        for p in placements:
            features = tetris_core.extract_features(board, piece_type, p.rotation, p.x, p.y)
            score = sum(w * f for w, f in zip(weights, features))
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
    print(f"  Finished seed {hex(seed)} after {pieces_placed} pieces.")

def main():
    start_t = time.time()
    for seed in FIXED_SEEDS:
        evaluate_seed(seed)
    print(f"Finished in {time.time() - start_t:.2f}s")

if __name__ == "__main__":
    main()
