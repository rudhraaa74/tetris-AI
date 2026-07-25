import sys
import os
import time

sys.path.append(os.path.abspath('../build'))
import tetris_core

def test_random_weights():
    game = tetris_core.Game(startLevel=0, seed=42)
    searcher = tetris_core.MoveSearch()
    weights = [0.0] * 9

    pieces_placed = 0
    null_input = tetris_core.Input()
    
    while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
        game.tick(null_input)

    while not game.isGameOver() and pieces_placed < 500:
        start_t = time.time()
        placements = searcher.getValidPlacements(game)
        
        if not placements:
            print("No valid placements found.")
            break

        best_score = -float('inf')
        best_placement = None
        
        board = game.getBoard()
        piece_type = game.getActivePieceType()

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
            print(f"Placed piece {pieces_placed}, Lines: {game.getLines()}, Placements Found: {len(placements)}, Time: {time.time() - start_t:.3f}s")
        else:
            break

    print(f"Game Over: {game.isGameOver()}, Pieces: {pieces_placed}")

if __name__ == "__main__":
    test_random_weights()
