#include "engine/MoveSearch.h"
#include <cstring>
#include <algorithm>

namespace tetris {

MoveSearch::MoveSearch() {
    m_visited.fill({0, 30000, 0xFFFFFFFF, 0, 0, 0});
    m_placements_found.fill(0);
}

inline bool MoveSearch::shouldProcessAndMark(const KinematicState& s, uint32_t parent_index, uint8_t input_mask) {
    int x_idx = s.x + 2; // 0..11
    
    int index = x_idx;
    index = index * 23 + s.y; // 0..22
    index = index * 4 + s.rotation; // 0..3
    index = index * 3 + s.das_direction; // 0..2
    
    auto& meta = m_visited[index];
    
    if (meta.search_id != m_current_search_id) {
        meta.search_id = m_current_search_id;
        meta.best_frames = s.frames;
        meta.parent_index = parent_index;
        meta.input_mask = input_mask;
        meta.max_das_charge = s.das_charge;
        meta.max_sd_counter = s.soft_drop_counter;
        return true;
    }
    
    bool better = false;
    if (s.das_charge > meta.max_das_charge) {
        meta.max_das_charge = s.das_charge;
        better = true;
    }
    if (s.soft_drop_counter > meta.max_sd_counter) {
        meta.max_sd_counter = s.soft_drop_counter;
        better = true;
    }
    
    return better;
}

inline void MoveSearch::markPlacement(const Placement& p) {
    int x = p.x + 2;
    int y = p.y;
    int rot = p.rotation;
    int index = (x * 23 + y) * 4 + rot;
    m_placements_found[index] = m_current_search_id;
}

inline bool MoveSearch::isPlacementFound(const Placement& p) const {
    int x = p.x + 2;
    int y = p.y;
    int rot = p.rotation;
    int index = (x * 23 + y) * 4 + rot;
    return m_placements_found[index] == m_current_search_id;
}

void MoveSearch::search(const Board& board, PieceType piece, int level, const KinematicState& initialState, FixedCapacityVector<Placement, 128>* out_placements) {
    out_placements->clear();
    
    // O(1) clear array trick
    m_current_search_id++;
    if (m_current_search_id == 0) {
        m_current_search_id = 1;
        m_visited.fill({0, 30000, 0xFFFFFFFF, 0, 0, 0});
        m_placements_found.fill(0);
    }
    
    int head = 0;
    int tail = 0;
    int queue_size = 0;
    
    m_queue_arr.resize(MAX_QUEUE_SIZE);
    
    m_history.clear();
    m_history.reserve(100000);
    
    m_history.push_back({0xFFFFFFFF, 0});
    m_queue_arr[tail] = {initialState, 0};
    
    tail = (tail + 1) % MAX_QUEUE_SIZE;
    queue_size++;
    shouldProcessAndMark(initialState, 0, 0);
    
    int gravityLimit = Timing::getGravityFrames(level);
    int maxFrames = 22 * gravityLimit + 20; 
    
    while (queue_size > 0) {
        const KinematicState state = m_queue_arr[head].state;
        uint32_t current_history_idx = m_queue_arr[head].history_idx;
        head = (head + 1) % MAX_QUEUE_SIZE;
        queue_size--;
        
        if (state.frames >= maxFrames) {
            continue;
        }

        // Generate combinations:
        // Horizontal: None(0), Left(1), Right(2)
        // Vertical: None(0), Down(1)
        // Rotation: None(0), CW(1), CCW(2)
        for (int h = 0; h < 3; ++h) {
            for (int v = 0; v < 2; ++v) {
                for (int r = 0; r < 3; ++r) {
                    KinematicState next = state;
                    next.frames++;
                    
                    bool input_left = (h == 1);
                    bool input_right = (h == 2);
                    bool input_down = (v == 1);
                    bool input_cw = (r == 1);
                    bool input_ccw = (r == 2);
                    
                    uint8_t input_mask = 0;
                    if (input_left) input_mask |= 1;
                    if (input_right) input_mask |= 2;
                    if (input_down) input_mask |= 4;
                    if (input_cw) input_mask |= 8;
                    if (input_ccw) input_mask |= 16;
                    
                    // 1. DAS
                    bool leftPress = input_left && (state.das_direction != 1);
                    bool rightPress = input_right && (state.das_direction != 2);
                    
                    if (input_left || input_right) {
                        if (leftPress || rightPress) {
                            next.das_charge = 0;
                        }
                        next.das_direction = input_left ? 1 : 2;
                    } else {
                        next.das_charge = 0;
                        next.das_direction = 0;
                    }
                    
                    if (next.das_direction != 0) {
                        if (next.das_charge == 0 || next.das_charge == Timing::DAS_INITIAL_DELAY) {
                            int dx = (next.das_direction == 1) ? -1 : 1;
                            if (board.isValidPlacement(piece, next.rotation, next.x + dx, next.y)) {
                                next.x += dx;
                                next.das_charge = (next.das_charge == 0) ? 1 : Timing::DAS_RESET_VALUE;
                            } else {
                                next.das_charge = Timing::DAS_MAX_CHARGE;
                            }
                        } else {
                            if (next.das_charge < Timing::DAS_MAX_CHARGE) {
                                next.das_charge++;
                            }
                        }
                    }
                    
                    // 2. Rotation
                    bool cwPress = input_cw && !state.prev_cw;
                    bool ccwPress = input_ccw && !state.prev_ccw;
                    
                    if (cwPress || ccwPress) {
                        int newRot = next.rotation;
                        if (cwPress) newRot = (next.rotation + 1) % 4;
                        else if (ccwPress) newRot = (next.rotation + 3) % 4;
                        
                        if (board.isValidPlacement(piece, newRot, next.x, next.y)) {
                            next.rotation = newRot;
                        }
                    }
                    next.prev_cw = input_cw;
                    next.prev_ccw = input_ccw;
                    
                    // 3. Soft Drop
                    bool softDropActive = false;
                    bool downPress = input_down && !state.soft_drop_held;
                    
                    if (input_down) {
                        if (downPress) next.soft_drop_counter = 0;
                        next.soft_drop_held = true;
                        next.soft_drop_counter++;
                        if (next.soft_drop_counter == Timing::SOFT_DROP_INITIAL_DELAY) {
                            softDropActive = true;
                            next.soft_drop_counter = 1; // repeat every 2 frames
                        }
                    } else {
                        next.soft_drop_counter = 0;
                        next.soft_drop_held = false;
                    }
                    
                    // 4. Gravity & Lock
                    next.gravity_counter++;
                    bool lock_triggered = false;
                    
                    if (next.gravity_counter >= gravityLimit || softDropActive) {
                        if (board.isValidPlacement(piece, next.rotation, next.x, next.y + 1)) {
                            next.y++;
                            if (next.gravity_counter >= gravityLimit) {
                                next.gravity_counter = 0;
                            }
                        } else {
                            // Lock triggered
                            lock_triggered = true;
                        }
                    }
                    
                    if (lock_triggered) {
                        Placement p{next.x, next.y, next.rotation, next.frames, 0, {}};
                        if (!isPlacementFound(p)) {
                            markPlacement(p);
                            
                            // Reconstruct path
                            int steps = next.frames - initialState.frames;
                            p.num_inputs = steps;
                            int input_idx = steps - 1;
                            
                            if (input_idx >= 0 && input_idx < 1500) {
                                uint8_t final_input = 0;
                                if (input_left) final_input |= 1;
                                if (input_right) final_input |= 2;
                                if (input_down) final_input |= 4;
                                if (input_cw) final_input |= 8;
                                if (input_ccw) final_input |= 16;
                                
                                p.input_masks[input_idx--] = final_input;
                                
                                uint32_t curr = current_history_idx;
                                while (curr != 0xFFFFFFFF && m_history[curr].parent != 0xFFFFFFFF && input_idx >= 0) {
                                    p.input_masks[input_idx--] = m_history[curr].input_mask;
                                    curr = m_history[curr].parent;
                                }
                            }
                            
                            out_placements->push_back(p);
                        }
                    } else {
                        if (shouldProcessAndMark(next, 0, 0)) {
                            if (queue_size < MAX_QUEUE_SIZE) {
                                uint32_t next_history_idx = m_history.size();
                                m_history.push_back({current_history_idx, input_mask});
                                m_queue_arr[tail] = {next, next_history_idx};
                                tail = (tail + 1) % MAX_QUEUE_SIZE;
                                queue_size++;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // no return statement needed
}

} // namespace tetris
