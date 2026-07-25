#pragma once
#include <array>
#include "FixedCapacityVector.h"
#include <cstdint>
#include "Piece.h"

namespace tetris {

class Board {
public:
    static constexpr int WIDTH = 10;
    static constexpr int HEIGHT = 22;
    static constexpr int BUFFER_ROWS = 2;

    Board();

    bool isValidPlacement(PieceType type, int rotation, int x, int y) const;

    // Writes the piece into the board.
    void lockPiece(PieceType type, int rotation, int x, int y);

    // Identifies full lines on the board.
    FixedCapacityVector<int, 4> getFullLines() const;

    // Removes the specified lines and shifts everything above them down.
    void removeLines(const FixedCapacityVector<int, 4>& lines);

    PieceType getCell(int x, int y) const;
    void clear();
    int getHighestLockedY() const;
    
    const std::array<uint16_t, HEIGHT>& getBitboard() const { return m_bitboard; }
    
    // For unit testing only
    void testSetRow(int y, uint16_t mask);

private:
    std::array<std::array<PieceType, WIDTH>, HEIGHT> m_grid;
    std::array<uint16_t, HEIGHT> m_bitboard;
};

} // namespace tetris
