#pragma once

namespace tetris {

struct Input {
    bool left = false;
    bool right = false;
    bool rotateCW = false;
    bool rotateCCW = false;
    bool softDrop = false;
};

} // namespace tetris
