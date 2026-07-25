#pragma once

#include "engine/Board.h"
#include "engine/Piece.h"
#include "engine/Input.h"
#include "engine/Timing.h"
#include <cstdint>
#include <array>
#include <vector>
#include "FixedCapacityVector.h"

namespace tetris {

struct KinematicState {
    int8_t x;
    int8_t y;
    uint8_t rotation;
    uint8_t das_charge;
    uint8_t das_direction; // 0 = None, 1 = Left, 2 = Right
    uint8_t gravity_counter; // frames since last drop
    uint8_t soft_drop_counter;
    bool soft_drop_held;
    bool prev_cw;
    bool prev_ccw;
    uint16_t frames; // depth in search tree
};

struct Placement {
    int8_t x;
    int8_t y;
    uint8_t rotation;
    uint16_t required_frames;
    uint16_t num_inputs = 0;
    std::array<uint8_t, 1500> input_masks{};

    bool operator==(const Placement& o) const {
        return x == o.x && y == o.y && rotation == o.rotation;
    }
};

class MoveSearch {
public:
    static constexpr size_t MAX_QUEUE_SIZE = 500000;
    static constexpr size_t MAX_PLACEMENTS = 2000;

    MoveSearch();

    void search(const Board& board, PieceType piece, int level, const KinematicState& initialState, FixedCapacityVector<Placement, 128>* out_placements);

private:
    uint32_t m_current_search_id = 0;
    struct QueueItem {
        KinematicState state;
        uint32_t history_idx;
    };
    std::vector<QueueItem> m_queue_arr;
    
    // To prevent state explosion, we store the best state reached at each (x, y, rotation).
    // A state is "better" if it arrives in fewer frames or has more DAS charge.
    struct VisitedMeta {
        uint32_t search_id;
        int16_t best_frames;
        uint32_t parent_index;
        uint8_t input_mask;
        uint8_t max_das_charge;
        uint8_t max_sd_counter;
    };
    // Indexed by: x (12) * y (23) * rotation (4) * das_direction (3) = 3312
    std::array<VisitedMeta, 3312> m_visited;
    std::array<uint32_t, 12 * 23 * 4> m_placements_found;

    struct HistoryNode {
        uint32_t parent;
        uint8_t input_mask;
    };
    std::vector<HistoryNode> m_history;

    bool shouldProcessAndMark(const KinematicState& s, uint32_t parent_index, uint8_t input_mask);
    void markPlacement(const Placement& p);
    bool isPlacementFound(const Placement& p) const;
};

} // namespace tetris
