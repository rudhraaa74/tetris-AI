#include "Timing.h"
#include <algorithm>

namespace tetris {

int Timing::getGravityFrames(int level) {
    static constexpr int frames[] = {
        48, 43, 38, 33, 28, 23, 18, 13, 8, 6,
        5, 5, 5, 4, 4, 4, 3, 3, 3, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 1
    };
    if (level < 0) return 48;
    if (level >= 29) return 1;
    return frames[level];
}

int Timing::getAREFrames(int lockY) {
    // Height from bottom = 21 - lockY (assuming max Y is 21).
    int heightFromBottom = 21 - lockY;
    if (heightFromBottom <= 1) { // Bottom 2 rows
        return 10;
    } else {
        int groupsOfFour = (heightFromBottom - 2) / 4 + 1;
        return 10 + groupsOfFour * 2;
    }
}

} // namespace tetris
