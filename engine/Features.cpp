#include "Features.h"
#include <cmath>

namespace tetris {

std::array<float, 9> Features::extract(const Board& afterstate, int piece_y, int lines_cleared, int piece_cells_cleared) {
    const auto& bitboard = afterstate.getBitboard();
    
    int landing_height = 22 - piece_y;
    int eroded_piece_cells = lines_cleared * piece_cells_cleared;
    
    int row_transitions = 0;
    int col_transitions = 0;
    int holes = 0;
    int cumulative_wells = 0;
    int hole_depth = 0;
    int rows_with_holes = 0;
    
    uint16_t covered = 0;
    int well_depth[10] = {0};
    int filled_in_col[10] = {0};
    int col_height[10] = {0};
    
    uint16_t prev_r = 0;
    
    for (int y = 0; y < Board::HEIGHT; ++y) {
        uint16_t r = bitboard[y] & 0x3FF; // 10 bits
        
        // Row transitions
        int rt = __builtin_popcount((r ^ (r << 1)) & 0x3FE);
        rt += ((r & 1) == 0); // left wall transition
        rt += ((r & 512) == 0); // right wall transition
        row_transitions += rt;
        
        // Column transitions
        col_transitions += __builtin_popcount((prev_r ^ r) & 0x3FF);
        prev_r = r;
        
        // Holes
        uint16_t hole_cells = covered & (~r) & 0x3FF;
        if (hole_cells) {
            holes += __builtin_popcount(hole_cells);
            rows_with_holes++;
            
            // Hole depth
            for (int x = 0; x < 10; ++x) {
                if ((hole_cells >> x) & 1) {
                    hole_depth += filled_in_col[x];
                }
            }
        }
        
        // Cumulative Wells
        uint16_t left_walls = (r << 1) | 1;
        uint16_t right_walls = (r >> 1) | 512;
        uint16_t well_cells = left_walls & right_walls & (~r) & 0x3FF;
        
        for (int x = 0; x < 10; ++x) {
            if ((well_cells >> x) & 1) {
                well_depth[x]++;
                cumulative_wells += well_depth[x];
            } else {
                well_depth[x] = 0;
            }
            
            // Update filled_in_col
            if ((r >> x) & 1) {
                filled_in_col[x]++;
            }
        }
        
        // Pattern diversity heights
        uint16_t new_tops = r & (~covered) & 0x3FF;
        if (new_tops) {
            for (int x = 0; x < 10; ++x) {
                if ((new_tops >> x) & 1) {
                    col_height[x] = Board::HEIGHT - y;
                }
            }
        }
        
        covered |= r;
    }
    
    // Bottom boundary for column transitions
    col_transitions += __builtin_popcount((~prev_r) & 0x3FF);
    
    // Pattern Diversity
    int pattern_diversity = 0;
    for (int x = 0; x < 9; ++x) {
        if (std::abs(col_height[x] - col_height[x+1]) < 2) {
            pattern_diversity++;
        }
    }
    
    return {
        static_cast<float>(landing_height),
        static_cast<float>(eroded_piece_cells),
        static_cast<float>(row_transitions),
        static_cast<float>(col_transitions),
        static_cast<float>(holes),
        static_cast<float>(cumulative_wells),
        static_cast<float>(hole_depth),
        static_cast<float>(rows_with_holes),
        static_cast<float>(pattern_diversity)
    };
}

} // namespace tetris
