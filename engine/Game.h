#pragma once

#include "Input.h"
#include "Piece.h"
#include "Board.h"
#include "Randomizer.h"
#include "FixedCapacityVector.h"
#include <cstdint>

namespace tetris {

enum class GameState {
    PLAYING,
    ENTRY_DELAY, // ARE
    LINE_CLEAR,
    GAME_OVER
};

class Game {
public:
    Game(int startLevel = 0, uint16_t seed = 0x8988);

    // Advances the game state by one frame
    void tick(const Input& input);

    // Resets the game to start conditions
    void reset(int startLevel, uint16_t seed = 0x8988);

    // Accessors for state
    GameState getState() const { return m_state; }
    int getLevel() const { return m_level; }
    int getScore() const { return m_score; }
    int getLines() const { return m_lines; }
    int getTetrises() const { return m_tetrises; }
    
    PieceType getActivePieceType() const { return m_activePiece; }
    PieceType getNextPieceType() const { return m_nextPiece; }
    int getActiveX() const { return m_activeX; }
    int getActiveY() const { return m_activeY; }
    int getActiveRotation() const { return m_activeRotation; }
    
    const Board& getBoard() const { return m_board; }
    
    // Timing / DAS Accessors
    int getGravityCounter() const { return m_gravityCounter; }
    int getDasCounter() const { return m_dasCounter; }
    int getDasDirection() const { 
        if (m_dasLeftHeld) return 1;
        if (m_dasRightHeld) return 2;
        return 0;
    }
    int getSoftDropCounter() const { return m_softDropCounter; }
    bool getSoftDropHeld() const { return m_softDropHeld; }
    
    bool getPrevRotateCW() const { return m_prevRotateCW; }
    bool getPrevRotateCCW() const { return m_prevRotateCCW; }

private:
    void spawnPiece();
    void processLock();

    int m_startLevel;
    int m_level;
    int m_lines;
    int m_score;
    int m_tetrises;
    
    Board m_board;
    Randomizer m_randomizer;
    
    GameState m_state;
    
    PieceType m_activePiece;
    PieceType m_nextPiece;
    int m_activeX;
    int m_activeY;
    int m_activeRotation;
    
    // Timing counters
    int m_gravityCounter;
    int m_areCounter;
    int m_lineClearCounter;
    
    // DAS state
    int m_dasCounter;
    bool m_dasLeftHeld;
    bool m_dasRightHeld;
    
    // Soft drop state
    int m_softDropCounter;
    bool m_softDropHeld;
    int m_softDropScoreTracker;
    
    // History
    bool m_prevLeft;
    bool m_prevRight;
    bool m_prevRotateCW;
    bool m_prevRotateCCW;
    bool m_prevDown;
    
    FixedCapacityVector<int, 4> m_clearingLines;
};

} // namespace tetris
