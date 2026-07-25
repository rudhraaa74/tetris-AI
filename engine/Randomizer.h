#pragma once
#include <cstdint>
#include "Piece.h"

namespace tetris {

class Randomizer {
public:
    Randomizer(uint16_t seed = 0x8988);

    void advance();
    PieceType getNextPiece(PieceType lastPiece);
    
    uint16_t getState() const { return m_lfsr; }
    void setState(uint16_t state) { m_lfsr = state; }

private:
    uint16_t m_lfsr;
};

} // namespace tetris
