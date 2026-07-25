import sys
import os

# Add build directory to path to find tetris_core.so / .dylib
sys.path.append(os.path.abspath('../build'))

try:
    import tetris_core
except ImportError as e:
    print("Failed to import tetris_core. Ensure it is built in the 'build' directory.")
    print("Error:", e)
    sys.exit(1)

def run_phase0():
    # Initialize Game and MoveSearch
    game = tetris_core.Game(startLevel=0, seed=0x8988)
    searcher = tetris_core.MoveSearch()

    pieces_placed = 0
    max_pieces = 1000

    print("Starting Phase 0 harness loop...")

    while not game.isGameOver() and pieces_placed < max_pieces:
        # Get valid placements for the current piece
        placements = searcher.getValidPlacements(game)

        if not placements:
            print("No valid placements found. Game Over?")
            break

        # Phase 0 logic: dumbly pick the first valid placement every time
        best_placement = placements[0]

        # Apply the exact inputs discovered by the BFS
        for input_state in best_placement.inputs:
            game.tick(input_state)
            
        # Fast-forward until the piece locks
        null_input = tetris_core.Input()
        null_input.softDrop = True
        while game.getState() == tetris_core.GameState.PLAYING and not game.isGameOver():
            game.tick(null_input)
            
        # Fast-forward through delays
        null_input.softDrop = False
        while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
            game.tick(null_input)

        pieces_placed += 1

        if pieces_placed % 100 == 0:
            print(f"Placed {pieces_placed} pieces. Score: {game.getScore()}, Lines: {game.getLines()}")

    print("--- Simulation Complete ---")
    print(f"Pieces Placed: {pieces_placed}")
    print(f"Game Over: {game.isGameOver()}")
    print(f"Final Score: {game.getScore()}")
    print(f"Final Level: {game.getLevel()}")
    print(f"Final Lines: {game.getLines()}")

if __name__ == "__main__":
    run_phase0()
