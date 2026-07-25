import sys
import os

sys.path.append(os.path.abspath('../build'))
import tetris_core

def test_input_replay():
    game = tetris_core.Game(startLevel=0, seed=42)
    searcher = tetris_core.MoveSearch()
    
    null_input = tetris_core.Input()
    while game.getState() != tetris_core.GameState.PLAYING and not game.isGameOver():
        game.tick(null_input)
        
    placements = searcher.getValidPlacements(game)
    
    # Pick a placement that is far away (e.g. x=0 or x=9)
    target_p = None
    for p in placements:
        if p.x == 0 or p.x == 8:  # 8 is max x for some pieces
            target_p = p
            break
            
    if not target_p:
        print("Could not find a wide placement.")
        return
        
    print(f"Target Placement: x={target_p.x}, y={target_p.y}, rot={target_p.rotation}")
    
    # Execute inputs
    for i, input_state in enumerate(target_p.inputs):
        game.tick(input_state)
        
    # See if it locked
    print(f"After inputs, state is: {game.getState()}")
    if game.getState() == tetris_core.GameState.PLAYING:
        print(f"Piece active at: x={game.getActiveX()}, y={game.getActiveY()}, rot={game.getActiveRotation()}")
        if game.getActiveX() != target_p.x or game.getActiveRotation() != target_p.rotation:
            print("ERROR: Piece did not reach target X/Rotation!")
    else:
        print("Piece locked.")
        
    # Print the board
    print("Board:")
    grid = game.getBoard().get_grid()
    for row in grid:
        print("".join("." if c == 7 else "#" for c in row))

if __name__ == "__main__":
    test_input_replay()
