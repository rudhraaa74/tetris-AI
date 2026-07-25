#pragma once

#include "engine/Board.h"
#include <array>
#include <cstdint>

namespace tetris {

struct FeatureWeights {
    float landing_height;
    float eroded_piece_cells;
    float row_transitions;
    float col_transitions;
    float holes;
    float cumulative_wells;
    float hole_depth;
    float rows_with_holes;
    float pattern_diversity;
};

class Features {
public:
    // Extract the 9 BCTS features from an afterstate board.
    // - afterstate: The board AFTER the piece has been locked and lines cleared.
    // - piece_y: The Y coordinate (row) where the piece locked.
    // - lines_cleared: The number of lines cleared by this placement.
    // - piece_cells_cleared: The number of cells of the placed piece that were in the cleared lines.
    static std::array<float, 9> extract(const Board& afterstate, int piece_y, int lines_cleared, int piece_cells_cleared);
};

} // namespace tetris
