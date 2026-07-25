#include <catch2/catch_test_macros.hpp>
#include "engine/MoveSearch.h"
#include <memory>
#include <iostream>

using namespace tetris;

extern int allocation_count;
extern bool tracking_allocations;


TEST_CASE("Zero allocations in hot path", "[movesearch]") {
    auto searcher = std::make_unique<MoveSearch>();
    Placement placements[MoveSearch::MAX_PLACEMENTS];
    
    Board board;
    KinematicState init{5, 0, 0, 0, 0, 0, 0, false, 0};
    
    tracking_allocations = true;
    allocation_count = 0;
    int count = searcher->search(board, PieceType::T, 18, init, placements);
    tracking_allocations = false;
    
    REQUIRE(allocation_count == 0);
    REQUIRE(count > 0);
}

TEST_CASE("Open board placements", "[movesearch]") {
    auto searcher = std::make_unique<MoveSearch>();
    Placement placements[MoveSearch::MAX_PLACEMENTS];
    
    Board board;
    KinematicState init{5, 0, 0, 0, 0, 0, 0, false, 0};
    
    int count = searcher->search(board, PieceType::O, 0, init, placements);
    // On level 0 (slow gravity), an O piece on an empty board can reach every horizontal position at the bottom.
    // O piece takes 2 columns, so x can be 0 through 8. (9 positions)
    // Rotation is mathematically tracked as 0, 1, 2, 3 in the state, so it generates 9 * 4 = 36 placements.
    REQUIRE(count == 36);
    for (int i=0; i<count; i++) {
        REQUIRE(placements[i].y == 20); // bottom row for O piece (it extends to y+1, so root is at 20)
    }
}

TEST_CASE("Tuck under overhang", "[movesearch]") {
    auto searcher = std::make_unique<MoveSearch>();
    Placement placements[MoveSearch::MAX_PLACEMENTS];
    
    Board board;
    // Build an overhang:
    // ...XX.....
    // ...XX.....
    // ..........
    board.lockPiece(PieceType::O, 0, 3, 19); 
    
    // So columns 3,4 are blocked at rows 19, 20. But row 21 is empty underneath!
    // We want to see if the BFS can slide an I-piece under the overhang.
    KinematicState init{5, 0, 0, 0, 0, 0, 0, false, 0};
    
    int count = searcher->search(board, PieceType::I, 0, init, placements);
    
    // Check if there is a placement with y=21 and x=3 (under the overhang)
    bool found_tuck = false;
    for (int i=0; i<count; i++) {
        if (placements[i].y == 21 && (placements[i].x == 3 || placements[i].x == 2)) {
            found_tuck = true;
        }
    }
    REQUIRE(found_tuck == true);
}

TEST_CASE("High gravity limit (Level 29)", "[movesearch]") {
    auto searcher = std::make_unique<MoveSearch>();
    Placement placements[MoveSearch::MAX_PLACEMENTS];
    
    Board board;
    KinematicState init{5, 0, 0, 0, 0, 0, 0, false, 0};
    
    // Level 29 is 1G (1 drop per frame)
    // At 1G, the piece drops 1 row every frame.
    // We want to test that DAS/Hypertapping speed limits reachability.
    // Hypertapping takes 2 frames per shift (press, release, press, ...).
    // Start X is 5. To reach X=0, it takes 5 shifts = 10 frames.
    // If we place a column at X=0 from Y=10 downwards, a naive search
    // would think we can rest a piece on top of it at X=0, Y=9.
    // But since it takes 10 frames to reach X=0, and gravity pulls it to Y=10
    // by frame 10, it will be at Y=9 at frame 9, where it is only at X=1!
    // So it cannot physically reach X=0, Y=9 before falling past it.
    
    for (int y = 10; y < 22; y += 2) {
        board.lockPiece(PieceType::O, 0, 1, y);
    }
    
    int count = searcher->search(board, PieceType::O, 29, init, placements);
    
    bool reached_impossible_edge = false;
    for (int i=0; i<count; i++) {
        if (placements[i].x == 0 && placements[i].y == 9) {
            reached_impossible_edge = true;
        }
    }
    REQUIRE(reached_impossible_edge == false);
}
