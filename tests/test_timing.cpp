#include <catch2/catch_test_macros.hpp>
#include "engine/Timing.h"

using namespace tetris;

TEST_CASE("Gravity frames are correct", "[timing]") {
    REQUIRE(Timing::getGravityFrames(0) == 48);
    REQUIRE(Timing::getGravityFrames(8) == 8);
    REQUIRE(Timing::getGravityFrames(9) == 6);
    REQUIRE(Timing::getGravityFrames(18) == 3);
    REQUIRE(Timing::getGravityFrames(19) == 2);
    REQUIRE(Timing::getGravityFrames(28) == 2);
    REQUIRE(Timing::getGravityFrames(29) == 1);
    REQUIRE(Timing::getGravityFrames(99) == 1);
}

TEST_CASE("ARE frames calculation", "[timing]") {
    REQUIRE(Timing::getAREFrames(21) == 10);
    REQUIRE(Timing::getAREFrames(20) == 10);
    REQUIRE(Timing::getAREFrames(19) == 12);
    REQUIRE(Timing::getAREFrames(16) == 12);
    REQUIRE(Timing::getAREFrames(15) == 14);
    REQUIRE(Timing::getAREFrames(0) == 20); // Top of buffer
}
