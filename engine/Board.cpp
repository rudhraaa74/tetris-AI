#include "Board.h"
#include <algorithm>

namespace tetris {

Board::Board() {
    clear();
}

void Board::clear() {
    for (auto& row : m_grid) {
        row.fill(PieceType::NONE);
    }
    m_bitboard.fill(0);
}

bool Board::isValidPlacement(PieceType type, int rotation, int x, int y) const {
    const auto& masks = Piece::getRowMasks(type, rotation, x);
    // getRowMasks handles horizontal bounds by returning 0xFFFF.
    if (masks[0] == 0xFFFF) {
        return false;
    }
    
    for (int i = 0; i < 4; ++i) {
        if (masks[i] == 0) continue; // Empty row in the piece's bounding box
        
        int by = y + (i - 2); // local Y maps from -2 to 1
        
        if (by < 0 || by >= HEIGHT) {
            return false; // Piece goes out of bounds vertically
        }
        
        if ((m_bitboard[by] & masks[i]) != 0) {
            return false; // Collision detected
        }
    }
    return true;
}

void Board::lockPiece(PieceType type, int rotation, int x, int y) {
    const auto& masks = Piece::getRowMasks(type, rotation, x);
    
    // Update rendering grid and bitboard
    for (const auto& block : Piece::getBlocks(type, rotation)) {
        int bx = x + block.x;
        int by = y + block.y;
        if (bx >= 0 && bx < WIDTH && by >= 0 && by < HEIGHT) {
            m_grid[by][bx] = type;
        }
    }
    
    for (int i = 0; i < 4; ++i) {
        if (masks[i] == 0) continue;
        int by = y + (i - 2);
        if (by >= 0 && by < HEIGHT) {
            m_bitboard[by] |= masks[i];
        }
    }
}

FixedCapacityVector<int, 4> Board::getFullLines() const {
    FixedCapacityVector<int, 4> fullLines;
    for (int y = 0; y < HEIGHT; ++y) {
        // A row is full if all 10 bits (0x3FF) are set.
        if (m_bitboard[y] == 0x3FF) {
            fullLines.push_back(y);
        }
    }
    return fullLines;
}

void Board::removeLines(const FixedCapacityVector<int, 4>& lines) {
    if (lines.empty()) return;

    std::array<std::array<PieceType, WIDTH>, HEIGHT> newGrid;
    for (auto& row : newGrid) row.fill(PieceType::NONE);
    std::array<uint16_t, HEIGHT> newBitboard;
    newBitboard.fill(0);

    int writeY = HEIGHT - 1;
    for (int readY = HEIGHT - 1; readY >= 0; --readY) {
        if (std::find(lines.begin(), lines.end(), readY) == lines.end()) {
            newGrid[writeY] = m_grid[readY];
            newBitboard[writeY] = m_bitboard[readY];
            writeY--;
        }
    }
    m_grid = newGrid;
    m_bitboard = newBitboard;
}

PieceType Board::getCell(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        return PieceType::NONE;
    }
    return m_grid[y][x];
}

int Board::getHighestLockedY() const {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (m_grid[y][x] != PieceType::NONE) {
                return y;
            }
        }
    }
    return HEIGHT; // empty board
}

void Board::testSetRow(int y, uint16_t mask) {
    if (y >= 0 && y < HEIGHT) {
        m_bitboard[y] = mask;
    }
}

} // namespace tetris
