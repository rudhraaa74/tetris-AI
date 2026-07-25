#include <catch2/catch_test_macros.hpp>
#include "engine/Board.h"

using namespace tetris;

TEST_CASE("Board placement and clearing", "[board]") {
    Board board;
    REQUIRE(board.isValidPlacement(PieceType::T, 0, 5, 10) == true);
    REQUIRE(board.isValidPlacement(PieceType::T, 0, -1, 10) == false); // out of bounds left
    
    // Fill the whole bottom row piece by piece
    for(int x = 0; x < 10; ++x) {
        // T piece at x, 21. T blocks are (-1,0), (0,0), (1,0), (0,1)
        // If we put 1x1 blocks, but we can only lock pieces.
        // Let's just lock I piece state 1 (vertical)
        // I vert: (0,-2), (0,-1), (0,0), (0,1)
        board.lockPiece(PieceType::I, 1, x, 20);
        // This fills (x,18), (x,19), (x,20), (x,21)
    }
    
    auto full = board.getFullLines();
    REQUIRE(full.size() == 4);
    REQUIRE(full[0] == 18);
    REQUIRE(full[1] == 19);
    REQUIRE(full[2] == 20);
    REQUIRE(full[3] == 21);
    
    board.removeLines(full);
    REQUIRE(board.getFullLines().empty());
}
