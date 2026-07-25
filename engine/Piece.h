#pragma once
#include <array>
#include <cstdint>

namespace tetris {

enum class PieceType : uint8_t {
    T = 0,
    J,
    Z,
    O,
    S,
    L,
    I,
    NONE
};

struct Position {
    int x;
    int y;
};

class Piece {
public:
    static constexpr int NUM_ROTATIONS = 4;
    static constexpr int BLOCKS_PER_PIECE = 4;

    // Get the localized coordinates for a given piece type and rotation state (0-3).
    // Rotation state: 0 = Spawn, 1 = CW, 2 = 180, 3 = CCW
    static const std::array<Position, BLOCKS_PER_PIECE>& getBlocks(PieceType type, int rotation);

    // Returns an array of 4 bitmasks (for Y offsets -2, -1, 0, +1 respectively).
    // If the piece placed at 'x' would be out of bounds horizontally, it returns masks of 0xFFFF (guaranteed collision).
    static const std::array<uint16_t, 4>& getRowMasks(PieceType type, int rotation, int x);
};

} // namespace tetris
