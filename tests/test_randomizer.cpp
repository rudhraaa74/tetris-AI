#include <catch2/catch_test_macros.hpp>
#include "engine/Randomizer.h"

using namespace tetris;

TEST_CASE("Randomizer LFSR validation", "[randomizer]") {
    Randomizer rng(0x8988);
    rng.advance();
    REQUIRE(rng.getState() != 0x8988);
    
    PieceType p = rng.getNextPiece(PieceType::NONE);
    REQUIRE(p != PieceType::NONE);
}
