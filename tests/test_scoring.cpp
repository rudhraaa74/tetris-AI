#include <catch2/catch_test_macros.hpp>
#include "engine/Scoring.h"

using namespace tetris;

TEST_CASE("Line clear scores", "[scoring]") {
    REQUIRE(Scoring::getLineClearScore(1, 0) == 40);
    REQUIRE(Scoring::getLineClearScore(4, 0) == 1200);
    REQUIRE(Scoring::getLineClearScore(4, 9) == 12000);
}

TEST_CASE("Next level thresholds", "[scoring]") {
    REQUIRE(Scoring::getNextLevelLines(0, 0, 0) == 10);
    REQUIRE(Scoring::getNextLevelLines(0, 0, 9) == 10);
    REQUIRE(Scoring::getNextLevelLines(0, 1, 15) == 20);
    
    REQUIRE(Scoring::getNextLevelLines(9, 9, 0) == 100);
    REQUIRE(Scoring::getNextLevelLines(9, 9, 99) == 100);
    REQUIRE(Scoring::getNextLevelLines(9, 10, 105) == 110);
    
    REQUIRE(Scoring::getNextLevelLines(15, 15, 0) == 100);
    REQUIRE(Scoring::getNextLevelLines(16, 16, 0) == 110);
}
