#pragma once

namespace tetris {

class Scoring {
public:
    static int getLineClearScore(int linesCleared, int postClearLevel);
    static int getSoftDropScore(int cellsDropped);
    static int getNextLevelLines(int startLevel, int currentLevel, int currentLines);
};

} // namespace tetris
