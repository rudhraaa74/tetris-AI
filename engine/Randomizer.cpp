#include "Randomizer.h"

namespace tetris {

Randomizer::Randomizer(uint16_t seed) : m_lfsr(seed) {}

void Randomizer::advance() {
    uint16_t bit1 = (m_lfsr >> 1) & 1;
    uint16_t bit9 = (m_lfsr >> 9) & 1;
    uint16_t newBit = bit1 ^ bit9;
    
    m_lfsr = (newBit << 15) | (m_lfsr >> 1);
}

PieceType Randomizer::getNextPiece(PieceType lastPiece) {
    advance();
    int roll = (m_lfsr >> 8) % 8;
    
    if (roll == 7 || roll == static_cast<int>(lastPiece)) {
        advance();
        roll = (m_lfsr >> 8) % 7;
    }
    
    return static_cast<PieceType>(roll);
}

} // namespace tetris
