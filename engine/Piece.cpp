#include "Piece.h"

namespace tetris {

static const std::array<std::array<std::array<Position, 4>, 4>, 7> PIECE_BLOCKS = {{
    // T
    {
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {1, 0}, {0, 1}}}, // state 0
        std::array<Position, 4>{{{0, -1}, {0, 0}, {0, 1}, {-1, 0}}}, // state 1
        std::array<Position, 4>{{{1, 0}, {0, 0}, {-1, 0}, {0, -1}}}, // state 2
        std::array<Position, 4>{{{0, 1}, {0, 0}, {0, -1}, {1, 0}}}, // state 3
    },
    // J
    {
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {1, 0}, {1, 1}}}, // state 0
        std::array<Position, 4>{{{0, -1}, {0, 0}, {0, 1}, {-1, 1}}}, // state 1
        std::array<Position, 4>{{{1, 0}, {0, 0}, {-1, 0}, {-1, -1}}}, // state 2
        std::array<Position, 4>{{{0, 1}, {0, 0}, {0, -1}, {1, -1}}}, // state 3
    },
    // Z
    {
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}}, // state 0
        std::array<Position, 4>{{{0, -1}, {0, 0}, {-1, 0}, {-1, 1}}}, // state 1
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}}, // state 2
        std::array<Position, 4>{{{0, -1}, {0, 0}, {-1, 0}, {-1, 1}}}, // state 3
    },
    // O
    {
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {-1, 1}, {0, 1}}}, // state 0
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {-1, 1}, {0, 1}}}, // state 1
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {-1, 1}, {0, 1}}}, // state 2
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {-1, 1}, {0, 1}}}, // state 3
    },
    // S
    {
        std::array<Position, 4>{{{0, 0}, {1, 0}, {-1, 1}, {0, 1}}}, // state 0
        std::array<Position, 4>{{{0, 0}, {0, 1}, {-1, -1}, {-1, 0}}}, // state 1
        std::array<Position, 4>{{{0, 0}, {1, 0}, {-1, 1}, {0, 1}}}, // state 2
        std::array<Position, 4>{{{0, 0}, {0, 1}, {-1, -1}, {-1, 0}}}, // state 3
    },
    // L
    {
        std::array<Position, 4>{{{-1, 0}, {0, 0}, {1, 0}, {-1, 1}}}, // state 0
        std::array<Position, 4>{{{0, -1}, {0, 0}, {0, 1}, {-1, -1}}}, // state 1
        std::array<Position, 4>{{{1, 0}, {0, 0}, {-1, 0}, {1, -1}}}, // state 2
        std::array<Position, 4>{{{0, 1}, {0, 0}, {0, -1}, {1, 1}}}, // state 3
    },
    // I
    {
        std::array<Position, 4>{{{-2, 0}, {-1, 0}, {0, 0}, {1, 0}}}, // state 0
        std::array<Position, 4>{{{0, -2}, {0, -1}, {0, 0}, {0, 1}}}, // state 1
        std::array<Position, 4>{{{-2, 0}, {-1, 0}, {0, 0}, {1, 0}}}, // state 2
        std::array<Position, 4>{{{0, -2}, {0, -1}, {0, 0}, {0, 1}}}, // state 3
    },
}};

const std::array<Position, 4>& Piece::getBlocks(PieceType type, int rotation) {
    int typeIdx = static_cast<int>(type);
    if (typeIdx < 0 || typeIdx >= 7) {
        typeIdx = 0;
    }
    return PIECE_BLOCKS[typeIdx][rotation % NUM_ROTATIONS];
}

static const auto PIECE_MASKS = []() {
    std::array<std::array<std::array<std::array<uint16_t, 4>, 10>, 4>, 7> masks{};
    for (int t = 0; t < 7; ++t) {
        for (int r = 0; r < 4; ++r) {
            for (int x = 0; x < 10; ++x) {
                bool out_of_bounds = false;
                std::array<uint16_t, 4> m = {0, 0, 0, 0};
                for (const auto& block : PIECE_BLOCKS[t][r]) {
                    int bx = x + block.x;
                    if (bx < 0 || bx >= 10) {
                        out_of_bounds = true;
                        break;
                    }
                    int y_idx = block.y + 2; // -2 -> 0, -1 -> 1, 0 -> 2, 1 -> 3
                    if (y_idx >= 0 && y_idx < 4) {
                        m[y_idx] |= (1 << bx);
                    }
                }
                if (out_of_bounds) {
                    m = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
                }
                masks[t][r][x] = m;
            }
        }
    }
    return masks;
}();

const std::array<uint16_t, 4>& Piece::getRowMasks(PieceType type, int rotation, int x) {
    int typeIdx = static_cast<int>(type);
    if (typeIdx < 0 || typeIdx >= 7) typeIdx = 0;
    
    if (x < 0 || x >= 10) {
        static const std::array<uint16_t, 4> OOB = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
        return OOB;
    }
    
    return PIECE_MASKS[typeIdx][rotation % NUM_ROTATIONS][x];
}

} // namespace tetris
