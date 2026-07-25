#include <catch2/catch_test_macros.hpp>
#include "engine/Piece.h"

using namespace tetris;

TEST_CASE("Piece spawn orientations", "[piece]") {
    auto t_blocks = Piece::getBlocks(PieceType::T, 0);
    REQUIRE(t_blocks[0].x == -1); REQUIRE(t_blocks[0].y == 0);
    REQUIRE(t_blocks[1].x == 0);  REQUIRE(t_blocks[1].y == 0);
    REQUIRE(t_blocks[2].x == 1);  REQUIRE(t_blocks[2].y == 0);
    REQUIRE(t_blocks[3].x == 0);  REQUIRE(t_blocks[3].y == 1);
    
    auto t_cw = Piece::getBlocks(PieceType::T, 1);
    REQUIRE(t_cw[0].x == 0);  REQUIRE(t_cw[0].y == -1);
    REQUIRE(t_cw[1].x == 0);  REQUIRE(t_cw[1].y == 0);
    REQUIRE(t_cw[2].x == 0);  REQUIRE(t_cw[2].y == 1);
    REQUIRE(t_cw[3].x == -1); REQUIRE(t_cw[3].y == 0);
}
