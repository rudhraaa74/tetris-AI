#pragma once
#include <cstdint>

namespace tetris {

class Timing {
public:
    // Gravity (frames per drop) given a level
    static int getGravityFrames(int level);

    // ARE (Entry Delay) frames given the lock height (y coordinate of highest locked block)
    // Board uses 0-indexed y coordinates where y=2 is the top visible row and y=21 is the bottom.
    static int getAREFrames(int lockY);

    // Constants for DAS
    static constexpr int DAS_INITIAL_DELAY = 16;
    static constexpr int DAS_REPEAT_RATE = 6;
    static constexpr int DAS_RESET_VALUE = 10;
    static constexpr int DAS_MAX_CHARGE = 16;

    // Constants for Line Clear
    static constexpr int LINE_CLEAR_ANIMATION_FRAMES = 20;
    
    // Drop / Lock Delay constants
    static constexpr int SOFT_DROP_INITIAL_DELAY = 3;
    static constexpr int SOFT_DROP_REPEAT_RATE = 2;
    static constexpr int OPENING_ENTRY_DELAY = 96;
};

} // namespace tetris
