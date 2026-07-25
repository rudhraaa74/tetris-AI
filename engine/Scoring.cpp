#include "Scoring.h"
#include <algorithm>

namespace tetris {

int Scoring::getLineClearScore(int linesCleared, int postClearLevel) {
    int baseScore = 0;
    switch (linesCleared) {
        case 1: baseScore = 40; break;
        case 2: baseScore = 100; break;
        case 3: baseScore = 300; break;
        case 4: baseScore = 1200; break;
        default: return 0;
    }
    return baseScore * (postClearLevel + 1);
}

int Scoring::getSoftDropScore(int cellsDropped) {
    return std::min(cellsDropped, 20);
}

int Scoring::getNextLevelLines(int startLevel, int currentLevel, int currentLines) {
    int firstTransition = std::min(startLevel * 10 + 10, std::max(100, startLevel * 10 - 50));
    
    if (currentLines < firstTransition) {
        return firstTransition;
    }
    
    int linesPastTransition = currentLines - firstTransition;
    return firstTransition + ((linesPastTransition / 10) + 1) * 10;
}

} // namespace tetris
