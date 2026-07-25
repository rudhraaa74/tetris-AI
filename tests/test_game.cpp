#include <catch2/catch_test_macros.hpp>
#include "engine/Game.h"

using namespace tetris;

TEST_CASE("Game initialization and entry delay", "[game]") {
    Game game(0, 0x8988);
    REQUIRE(game.getState() == GameState::ENTRY_DELAY);
    REQUIRE(game.getLevel() == 0);
    
    Input empty_input;
    for (int i=0; i<95; ++i) {
        game.tick(empty_input);
        REQUIRE(game.getState() == GameState::ENTRY_DELAY);
    }
    game.tick(empty_input);
    REQUIRE(game.getState() == GameState::PLAYING);
}

TEST_CASE("Opening entry delay cancellation", "[game]") {
    Game game(0, 0x8988);
    REQUIRE(game.getState() == GameState::ENTRY_DELAY);
    
    Input down_input; 
    down_input.softDrop = true;
    game.tick(down_input); // Fresh press of down should cancel ARE immediately
    REQUIRE(game.getState() == GameState::PLAYING);
}
