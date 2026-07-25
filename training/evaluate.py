import sys
import os

sys.path.append(os.path.abspath('../build'))

try:
    import tetris_core
except ImportError as e:
    print("Failed to import tetris_core. Ensure it is built in the 'build' directory.")
    print("Error:", e)
    sys.exit(1)

import argparse

# Correct BCTS weights aligned with Features.cpp
WEIGHTS = [-4.5, 3.2, -3.2, -9.3, -7.9, -3.3, -4.8, -3.3, 2.2]

def run_phase2():
    parser = argparse.ArgumentParser()
    parser.add_argument('--depth', type=int, default=1)
    parser.add_argument('--seed', type=int, default=42)
    args = parser.parse_args()

    game = tetris_core.Game(startLevel=0, seed=args.seed)
    searcher = tetris_core.MoveSearch()

    pieces_placed = 0
    max_pieces = 50000

    print(f"Starting Sanity Runner (Seed: {args.seed})...")

    # Fast-forward opening delay
    null_input = tetris_core.Input()
    while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
        game.tick(null_input)

    while not game.isGameOver() and pieces_placed < max_pieces:
        placements = searcher.getValidPlacements(game)

        if not placements:
            print("No valid placements found. Game Over!")
            break

        best_score = -float('inf')
        best_placement = None
        
        board = game.getBoard()
        piece_type = game.getActivePieceType()

        # Evaluate all candidate placements
        for p in placements:
            features = tetris_core.extract_features(board, piece_type, p.rotation, p.x, p.y)
            score = sum(w * f for w, f in zip(WEIGHTS, features))
            
            if pieces_placed == 1:
                print(f"Cand: x={p.x}, y={p.y}, r={p.rotation}, sc={score}, f={features}")

            if score > best_score:
                best_score = score
                best_placement = p

        # Execute best placement
        if best_placement:
            print(f"Best score: {best_score} for placement x={best_placement.x}, y={best_placement.y}")
            
            inputs_str = []
            for i in best_placement.inputs:
                s = []
                if i.left: s.append("L")
                if i.right: s.append("R")
                if i.softDrop: s.append("D")
                if i.rotateCW: s.append("cw")
                if i.rotateCCW: s.append("ccw")
                inputs_str.append("+".join(s) if s else "None")
            print(f"Frames: {best_placement.required_frames}, Num inputs: {len(best_placement.inputs)}")
            print("Inputs:", ", ".join(inputs_str))
                
            for input_state in best_placement.inputs:
                game.tick(input_state)
            
            # Fast-forward until the piece locks (state changes to ENTRY_DELAY or LINE_CLEAR)
            null_input = tetris_core.Input()
            null_input.softDrop = True # hold soft drop to speed up locking
            extra_frames = 0
            while game.getState() == tetris_core.GameState.PLAYING and not game.isGameOver():
                game.tick(null_input)
                extra_frames += 1
            if extra_frames > 0:
                print(f"WARNING: Ran {extra_frames} extra frames to lock!")
            
            # Fast-forward through delays (Line Clear, ARE) until the next piece spawns
            null_input.softDrop = False
            while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
                game.tick(null_input)
            
            pieces_placed += 1
            
            # Print board every piece
            board_grid = game.getBoard().get_grid()
            # print(f"--- Board after piece {pieces_placed} ---")
            # for row in board_grid:
            #     print("".join("." if cell == 7 else "#" for cell in row))
            
            if pieces_placed % 10 == 0:
                print(f"Placed {pieces_placed} pieces. Score: {game.getScore()}, Lines: {game.getLines()}")
        else:
            print("Failed to find a best placement.")
            break

    print("--- Simulation Complete ---")
    
    # Print the final board
    board_grid = game.getBoard().get_grid()
    for row in board_grid:
        line = ""
        for cell in row:
            # 0 is PieceType.NONE in the enum backing
            if cell == 0:
                line += "."
            else:
                line += "#"
        print(line)
        
    print(f"Pieces Placed: {pieces_placed}")
    print(f"Game Over: {game.isGameOver()}")
    print(f"Final Score: {game.getScore()}")
    print(f"Final Level: {game.getLevel()}")
    print(f"Final Lines: {game.getLines()}")

if __name__ == "__main__":
    run_phase2()
