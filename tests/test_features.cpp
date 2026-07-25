#include <catch2/catch_test_macros.hpp>
#include "engine/Board.h"
#include "engine/Features.h"

using namespace tetris;

Board createBoardFromStrings(const std::vector<std::string>& layout) {
    Board b;
    int y = Board::HEIGHT - layout.size();
    for (const auto& rowStr : layout) {
        uint16_t mask = 0;
        for (int x = 0; x < 10; ++x) {
            if (rowStr[x] == '#') {
                mask |= (1 << x);
            }
        }
        b.testSetRow(y, mask);
        y++;
    }
    return b;
}

TEST_CASE("Feature Extraction: Landing Height", "[features]") {
    Board b;
    // piece_y = 10, meaning landing height is 22 - 10 = 12
    auto f = Features::extract(b, 10, 0, 0);
    REQUIRE(f[0] == 12.0f);
}

TEST_CASE("Feature Extraction: Eroded Piece Cells", "[features]") {
    Board b;
    auto f = Features::extract(b, 20, 2, 4); // 2 lines cleared, 4 piece cells in those lines
    REQUIRE(f[1] == 8.0f); // 2 * 4 = 8
}

TEST_CASE("Feature Extraction: Row and Column Transitions", "[features]") {
    std::vector<std::string> layout = {
        "..........",
        "..........",
        "##..##....", // Row Transitions: wall-to-# (0), #-to-. (1), .-to-# (1), #-to-. (1), .-to-wall (0) -> Wait:
        // let's trace exactly:
        // wall(filled) -> # (0)
        // # -> . (1)
        // . -> . (0)
        // . -> # (1)
        // # -> . (1)
        // . -> . (0)...
        // . -> wall(filled) (1)
        // Total = 4.
        "#........#"  // wall -> # (0), # -> . (1), . -> # (1), # -> wall (0)
        // Total = 2.
    };
    
    Board b = createBoardFromStrings(layout);
    auto f = Features::extract(b, 0, 0, 0);
    
    // empty rows have 2 transitions (wall -> . -> wall)
    // 20 empty rows = 40 transitions
    // "##..##...." = 4
    // "#........#" = 2
    // Total row transitions = 46.
    REQUIRE(f[2] == 46.0f);
}

TEST_CASE("Feature Extraction: Holes and Wells", "[features]") {
    std::vector<std::string> layout = {
        "..........",
        "..........",
        "###.######", // 1 empty cell, bounded by #. That's a well of depth 1, and also covers below.
        "#.#.######", // x=1 is a hole. x=3 is a well of depth 2 (and hole).
        "#.########"  // x=1 is a hole.
    };
    Board b = createBoardFromStrings(layout);
    auto f = Features::extract(b, 0, 0, 0);
    
    // Holes: 
    // row 21: x=3 is empty but covered? Yes, by row 19 top space? No, row 19 is open!
    // Wait, x=3 is empty all the way up. So it's NOT a hole.
    // x=1 in row 20 is a hole (covered by # in row 19).
    // x=1 in row 21 is a hole (covered by # in row 19).
    // Total holes = 2.
    REQUIRE(f[4] == 2.0f); // Number of Holes
    
    // Wells:
    // x=3 is empty all the way down.
    // row 19: well depth 1
    // row 20: well depth 2
    // row 21: well depth 3
    // Cumulative = 1 + 2 + 3 = 6.
    // x=1 in row 20: empty, bounded by #. Well depth 1.
    // x=1 in row 21: empty, bounded by #. Well depth 2.
    // Cumulative = 3 + 3 = 6.
    REQUIRE(f[5] == 6.0f); // Cumulative Wells
    
    // Rows with holes: row 20 and row 21. Total = 2.
    REQUIRE(f[7] == 2.0f);
    
    // Hole Depth
    // x=1 hole in row 20 is covered by 1 block (row 19).
    // x=1 hole in row 21 is covered by 1 block (row 19).
    // Total Hole Depth = 2.
    REQUIRE(f[6] == 2.0f);
    
    // Pattern Diversity
    // Heights:
    // x=0: 3 (rows 19-21)
    // x=1: 1 (row 19 #) - wait, x=1 has # at row 19, so height is 22 - 19 = 3!
    // x=2: 3 (rows 19-21)
    // x=3: 1 (row 21 #) - wait, x=3 has # at row 21, so height is 22 - 21 = 1.
    // x=4..9: 3 (rows 19-21)
    // Diff(x=0, x=1) = abs(3-3) = 0 < 2 -> ++
    // Diff(x=1, x=2) = abs(3-3) = 0 < 2 -> ++
    // Diff(x=2, x=3) = abs(3-1) = 2 -> NO
    // Diff(x=3, x=4) = abs(1-3) = 2 -> NO
    // Diff(x=4..9) = 0 < 2 -> 5 matches.
    // Total = 1 + 1 + 5 = 7.
    REQUIRE(f[8] == 7.0f);
}
