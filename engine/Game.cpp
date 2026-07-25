#include "Game.h"
#include "Timing.h"
#include "Scoring.h"
#include "FixedCapacityVector.h"
#include <algorithm>

namespace tetris {

Game::Game(int startLevel, uint16_t seed) {
    reset(startLevel, seed);
}

void Game::reset(int startLevel, uint16_t seed) {
    m_startLevel = startLevel;
    m_level = startLevel;
    m_lines = 0;
    m_score = 0;
    m_tetrises = 0;
    
    m_board.clear();
    m_randomizer.setState(seed);
    
    m_state = GameState::ENTRY_DELAY;
    
    m_activePiece = PieceType::NONE;
    m_nextPiece = PieceType::NONE;
    m_activeX = 5;
    m_activeY = 0;
    m_activeRotation = 0;
    
    m_gravityCounter = 0;
    m_areCounter = Timing::OPENING_ENTRY_DELAY;
    m_lineClearCounter = 0;
    
    m_dasCounter = 0;
    m_dasLeftHeld = false;
    m_dasRightHeld = false;
    
    m_softDropCounter = 0;
    m_softDropHeld = false;
    m_softDropScoreTracker = 0;
    
    m_prevLeft = false;
    m_prevRight = false;
    m_prevRotateCW = false;
    m_prevRotateCCW = false;
    m_prevDown = false;
    
    m_clearingLines.clear();
    
    m_nextPiece = m_randomizer.getNextPiece(PieceType::NONE);
}

void Game::tick(const Input& input) {
    if (m_state == GameState::GAME_OVER) {
        return;
    }
    
    m_randomizer.advance();
    
    bool leftPress = input.left && !m_prevLeft;
    bool rightPress = input.right && !m_prevRight;
    bool cwPress = input.rotateCW && !m_prevRotateCW;
    bool ccwPress = input.rotateCCW && !m_prevRotateCCW;
    bool downPress = input.softDrop && !m_prevDown;
    
    if (input.left || input.right) {
        if (leftPress || rightPress || (input.left && !m_dasLeftHeld && !input.right) || (input.right && !m_dasRightHeld && !input.left)) {
            m_dasCounter = 0;
        }
        m_dasLeftHeld = input.left;
        m_dasRightHeld = input.right;
    } else {
        m_dasCounter = 0;
        m_dasLeftHeld = false;
        m_dasRightHeld = false;
    }
    
    if (input.softDrop) {
        if (downPress) {
            m_softDropCounter = 0;
            m_softDropScoreTracker = 0;
        }
        m_softDropHeld = true;
    } else {
        m_softDropCounter = 0;
        m_softDropHeld = false;
    }

    if (m_state != GameState::PLAYING) {
        if (m_dasLeftHeld || m_dasRightHeld) {
            if (m_dasCounter < Timing::DAS_MAX_CHARGE) {
                m_dasCounter++;
            }
        }
    }

    if (m_state == GameState::ENTRY_DELAY) {
        if (m_areCounter == Timing::OPENING_ENTRY_DELAY && downPress) {
            m_areCounter = 0;
        }
        
        if (m_areCounter > 0) {
            m_areCounter--;
        }
        if (m_areCounter == 0) {
            spawnPiece();
        }
    } else if (m_state == GameState::LINE_CLEAR) {
        if (m_lineClearCounter > 0) {
            m_lineClearCounter--;
        }
        if (m_lineClearCounter == 0) {
            m_board.removeLines(m_clearingLines);
            m_clearingLines.clear();
            spawnPiece(); // ARE is overridden by line clear animation
        }
    } else if (m_state == GameState::PLAYING) {
        if (m_dasLeftHeld || m_dasRightHeld) {
            if (m_dasCounter == 0 || m_dasCounter == Timing::DAS_INITIAL_DELAY) {
                int dx = m_dasLeftHeld ? -1 : 1;
                if (m_board.isValidPlacement(m_activePiece, m_activeRotation, m_activeX + dx, m_activeY)) {
                    m_activeX += dx;
                    m_dasCounter = (m_dasCounter == 0) ? 1 : Timing::DAS_RESET_VALUE;
                } else {
                    m_dasCounter = Timing::DAS_MAX_CHARGE;
                }
            } else {
                m_dasCounter++;
            }
        }
        
        if (cwPress || ccwPress) {
            int newRot = m_activeRotation;
            if (cwPress) newRot = (m_activeRotation + 1) % 4;
            else if (ccwPress) newRot = (m_activeRotation + 3) % 4;
            
            if (m_board.isValidPlacement(m_activePiece, newRot, m_activeX, m_activeY)) {
                m_activeRotation = newRot;
            }
        }
        
        bool softDropActive = false;
        if (m_softDropHeld) {
            m_softDropCounter++;
            if (m_softDropCounter == Timing::SOFT_DROP_INITIAL_DELAY) {
                softDropActive = true;
                m_softDropCounter = 1;
            }
        }
        
        m_gravityCounter++;
        int gravityThreshold = Timing::getGravityFrames(m_level);
        
        if (m_gravityCounter >= gravityThreshold || softDropActive) {
            if (m_board.isValidPlacement(m_activePiece, m_activeRotation, m_activeX, m_activeY + 1)) {
                m_activeY++;
                if (m_gravityCounter >= gravityThreshold) m_gravityCounter = 0;
                
                if (softDropActive) {
                    m_softDropScoreTracker++;
                }
            } else {
                processLock();
            }
        }
    }
    
    m_prevLeft = input.left;
    m_prevRight = input.right;
    m_prevRotateCW = input.rotateCW;
    m_prevRotateCCW = input.rotateCCW;
    m_prevDown = input.softDrop;
}

void Game::spawnPiece() {
    m_activePiece = m_nextPiece;
    m_nextPiece = m_randomizer.getNextPiece(m_activePiece);
    m_activeX = 5;
    m_activeY = 0;
    m_activeRotation = 0;
    
    m_gravityCounter = 0;
    m_softDropScoreTracker = 0;
    
    if (!m_board.isValidPlacement(m_activePiece, m_activeRotation, m_activeX, m_activeY)) {
        m_state = GameState::GAME_OVER;
    } else {
        m_state = GameState::PLAYING;
    }
}

void Game::processLock() {
    m_score += Scoring::getSoftDropScore(m_softDropScoreTracker);
    m_softDropScoreTracker = 0;
    
    m_board.lockPiece(m_activePiece, m_activeRotation, m_activeX, m_activeY);
    m_clearingLines = m_board.getFullLines();
    
    if (!m_clearingLines.empty()) {
        m_state = GameState::LINE_CLEAR;
        m_lineClearCounter = Timing::LINE_CLEAR_ANIMATION_FRAMES;
        
        m_lines += m_clearingLines.size();
        if (m_clearingLines.size() == 4) {
            m_tetrises++;
        }
        
        int nextLevelThreshold = Scoring::getNextLevelLines(m_startLevel, m_level, m_lines - m_clearingLines.size());
        if (m_lines >= nextLevelThreshold) {
            m_level++;
        }
        
        m_score += Scoring::getLineClearScore(m_clearingLines.size(), m_level);
        
    } else {
        m_state = GameState::ENTRY_DELAY;
        m_areCounter = Timing::getAREFrames(m_activeY);
    }
}

} // namespace tetris
