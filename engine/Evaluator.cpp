#include "Evaluator.h"
#include "Features.h"
#include <limits>
#include <algorithm>

namespace tetris {

static double evaluate_board(const Board& initial_board, PieceType type, int rotation, int x, int y, const std::array<double, 9>& weights) {
    Board sim_board = initial_board;
    sim_board.lockPiece(type, rotation, x, y);
    
    int piece_cells_cleared = 0;
    auto fullLines = sim_board.getFullLines();
    for (const auto& block : Piece::getBlocks(type, rotation)) {
        int by = y + block.y;
        bool cleared = false;
        for (int line_y : fullLines) {
            if (line_y == by) {
                cleared = true;
                break;
            }
        }
        if (cleared) piece_cells_cleared++;
    }
    
    int lines_cleared = fullLines.size();
    sim_board.removeLines(fullLines);
    
    auto features = Features::extract(sim_board, y, lines_cleared, piece_cells_cleared);
    
    double score = 0;
    for (size_t i = 0; i < 9; ++i) {
        score += weights[i] * features[i];
    }
    return score;
}

Placement Evaluator::getBestPlacementDepth2(const Game& game, const std::array<double, 9>& weights, MoveSearch& searcher) {
    FixedCapacityVector<Placement, 128> U1;
    
    KinematicState state;
    state.x = game.getActiveX();
    state.y = game.getActiveY();
    state.rotation = game.getActiveRotation();
    state.gravity_counter = game.getGravityCounter();
    state.das_charge = game.getDasCounter();
    state.das_direction = game.getDasDirection();
    state.soft_drop_held = game.getSoftDropHeld();
    state.soft_drop_counter = game.getSoftDropCounter();
    state.prev_cw = game.getPrevRotateCW();
    state.prev_ccw = game.getPrevRotateCCW();
    state.frames = 0;

    searcher.search(game.getBoard(), game.getActivePieceType(), game.getLevel(), state, &U1);

    if (U1.empty()) {
        return Placement{0, 0, 0, 0, 0, {}};
    }

    double best_V1 = -1e9;
    Placement best_u1 = U1[0];

    Input null_input;
    Input soft_drop_input;
    soft_drop_input.softDrop = true;

    for (const auto& u1 : U1) {
        Game G1 = game; // memcpy clone
        
        for (int i = 0; i < u1.num_inputs; ++i) {
            uint8_t mask = u1.input_masks[i];
            Input in{
                (mask & 1) != 0,
                (mask & 2) != 0,
                (mask & 8) != 0,
                (mask & 16) != 0,
                (mask & 4) != 0
            };
            G1.tick(in);
        }

        while (G1.getState() == GameState::PLAYING) {
            G1.tick(soft_drop_input);
        }
        
        while (G1.getState() != GameState::PLAYING && G1.getState() != GameState::GAME_OVER) {
            G1.tick(null_input);
        }

        if (G1.getState() == GameState::GAME_OVER) {
            continue; // Score is -inf
        }

        FixedCapacityVector<Placement, 128> U2;
        KinematicState state2;
        state2.x = G1.getActiveX();
        state2.y = G1.getActiveY();
        state2.rotation = G1.getActiveRotation();
        state2.gravity_counter = G1.getGravityCounter();
        state2.das_charge = G1.getDasCounter();
        state2.das_direction = G1.getDasDirection();
        state2.soft_drop_held = G1.getSoftDropHeld();
        state2.soft_drop_counter = G1.getSoftDropCounter();
        state2.prev_cw = G1.getPrevRotateCW();
        state2.prev_ccw = G1.getPrevRotateCCW();
        state2.frames = 0;

        searcher.search(G1.getBoard(), G1.getActivePieceType(), G1.getLevel(), state2, &U2);

        if (U2.empty()) {
            continue; // Score is -inf
        }

        double max_score_2 = -1e9;
        
        for (const auto& u2 : U2) {
            double score_2 = evaluate_board(G1.getBoard(), G1.getActivePieceType(), u2.rotation, u2.x, u2.y, weights);
            if (score_2 > max_score_2) {
                max_score_2 = score_2;
            }
        }

        if (max_score_2 > best_V1) {
            best_V1 = max_score_2;
            best_u1 = u1;
        }
    }

    return best_u1;
}

} // namespace tetris
