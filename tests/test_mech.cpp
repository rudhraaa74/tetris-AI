#include <catch2/catch_test_macros.hpp>
#include "engine/Game.h"
#include "engine/Timing.h"
#include <map>
#include <iostream>

using namespace tetris;

TEST_CASE("Randomizer RNG Distribution", "[mech]") {
    Randomizer rng(0x8988);
    std::map<PieceType, int> counts;
    PieceType lastPiece = PieceType::NONE;
    
    for (int i = 0; i < 10000; ++i) {
        PieceType p = rng.getNextPiece(lastPiece);
        counts[p]++;
        lastPiece = p;
    }
    
    REQUIRE(counts.size() == 7);
    for (int i = 0; i < 7; ++i) {
        REQUIRE(counts[static_cast<PieceType>(i)] > 1000); // Fairly balanced
    }
}

TEST_CASE("DAS wall charge during ARE", "[mech]") {
    Game game(0, 0x8988);
    REQUIRE(game.getState() == GameState::ENTRY_DELAY);
    
    Input left_input; 
    left_input.left = true;
    
    // Hold left for 20 frames during opening ARE
    for (int i = 0; i < 20; ++i) {
        game.tick(left_input);
    }
    
    // Complete the rest of the ARE (96 - 20 = 76 frames)
    for (int i = 0; i < 76; ++i) {
        game.tick(left_input);
    }
    REQUIRE(game.getState() == GameState::PLAYING);
    
    int startX = game.getActiveX();
    
    // On the very next tick, if DAS carried over properly, it will shift immediately 
    // because the counter charged to DAS_MAX_CHARGE (16) during ARE.
    game.tick(left_input);
    
    int nextX = game.getActiveX();
    REQUIRE(nextX < startX); // Should shift left immediately
}

TEST_CASE("Rotation Wall Blocking (No Wall Kicks)", "[mech]") {
    Board board;
    
    // T piece state 1 has a block at (-1, 0)
    // If placed at x=0, it will hit the left wall and be invalid
    REQUIRE(board.isValidPlacement(PieceType::T, 1, 0, 10) == false);
    
    // T piece state 3 has a block at (1, 0)
    // If placed at x=9 (rightmost), it will hit the right wall
    REQUIRE(board.isValidPlacement(PieceType::T, 3, 9, 10) == false);
}
