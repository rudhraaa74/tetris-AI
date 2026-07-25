#include "raylib.h"
#include "engine/Game.h"
#include "engine/Piece.h"
#include "engine/MoveSearch.h"
#include "engine/Evaluator.h"
#include <string>
#include <vector>
#include <queue>
#include <array>

using namespace tetris;

const int CELL_SIZE = 30;
const int BOARD_X = 100;
const int BOARD_Y = 50;

Color getPieceColor(PieceType type) {
    switch (type) {
        case PieceType::T: return MAGENTA;
        case PieceType::J: return BLUE;
        case PieceType::Z: return RED;
        case PieceType::O: return YELLOW;
        case PieceType::S: return GREEN;
        case PieceType::L: return ORANGE;
        case PieceType::I: return SKYBLUE;
        default: return BLANK;
    }
}

void DrawBoard(const Game& game) {
    const Board& board = game.getBoard();
    
    DrawRectangle(BOARD_X, BOARD_Y, Board::WIDTH * CELL_SIZE, (Board::HEIGHT - Board::BUFFER_ROWS) * CELL_SIZE, BLACK);
    DrawRectangleLines(BOARD_X, BOARD_Y, Board::WIDTH * CELL_SIZE, (Board::HEIGHT - Board::BUFFER_ROWS) * CELL_SIZE, WHITE);
    
    for (int y = Board::BUFFER_ROWS; y < Board::HEIGHT; ++y) {
        for (int x = 0; x < Board::WIDTH; ++x) {
            PieceType cell = board.getCell(x, y);
            if (cell != PieceType::NONE) {
                DrawRectangle(BOARD_X + x * CELL_SIZE, BOARD_Y + (y - Board::BUFFER_ROWS) * CELL_SIZE, CELL_SIZE, CELL_SIZE, getPieceColor(cell));
                DrawRectangleLines(BOARD_X + x * CELL_SIZE, BOARD_Y + (y - Board::BUFFER_ROWS) * CELL_SIZE, CELL_SIZE, CELL_SIZE, LIGHTGRAY);
            }
        }
    }
    
    if (game.getState() == GameState::PLAYING) {
        auto blocks = Piece::getBlocks(game.getActivePieceType(), game.getActiveRotation());
        for (const auto& b : blocks) {
            int px = game.getActiveX() + b.x;
            int py = game.getActiveY() + b.y;
            if (py >= Board::BUFFER_ROWS) {
                DrawRectangle(BOARD_X + px * CELL_SIZE, BOARD_Y + (py - Board::BUFFER_ROWS) * CELL_SIZE, CELL_SIZE, CELL_SIZE, getPieceColor(game.getActivePieceType()));
                DrawRectangleLines(BOARD_X + px * CELL_SIZE, BOARD_Y + (py - Board::BUFFER_ROWS) * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE);
            }
        }
    }
}

int main() {
    InitWindow(800, 700, "NES Tetris AI Sandbox - 2-Piece Lookahead");
    SetTargetFPS(60); // Set to 600 for fast forward viewing!

    Game game(19, 42); // Level 19, seed 42
    
    std::array<double, 9> best_weights = {
        -4.28190302, 5.09973296, -6.20119369, -8.17077911,
        -3.86185653, -4.20967858, -3.00149556, -8.15636664, 2.62364571
    };
    
    MoveSearch searcher;
    std::queue<Input> ai_input_queue;
    bool ai_enabled = true;

    while (!WindowShouldClose()) {
        Input input;
        input.left = IsKeyDown(KEY_LEFT);
        input.right = IsKeyDown(KEY_RIGHT);
        input.softDrop = IsKeyDown(KEY_DOWN);
        input.rotateCW = IsKeyDown(KEY_X) || IsKeyDown(KEY_UP);
        input.rotateCCW = IsKeyDown(KEY_Z);
        
        if (IsKeyPressed(KEY_R)) {
            game.reset(19, 42);
            while(!ai_input_queue.empty()) ai_input_queue.pop();
        }
        
        if (IsKeyPressed(KEY_A)) {
            ai_enabled = !ai_enabled;
        }

        Input tick_input;
        if (ai_enabled) {
            if (game.getState() == GameState::PLAYING && ai_input_queue.empty()) {
                Placement best_p = Evaluator::getBestPlacementDepth2(game, best_weights, searcher);
                for (int i = 0; i < best_p.num_inputs; ++i) {
                    uint8_t mask = best_p.input_masks[i];
                    Input in{
                        (mask & 1) != 0,
                        (mask & 2) != 0,
                        (mask & 8) != 0,
                        (mask & 16) != 0,
                        (mask & 4) != 0
                    };
                    ai_input_queue.push(in);
                }
                Input soft_drop;
                soft_drop.softDrop = true;
                ai_input_queue.push(soft_drop); // one extra to ensure lock
            }
            
            if (!ai_input_queue.empty()) {
                tick_input = ai_input_queue.front();
                ai_input_queue.pop();
                
                // If it locked, clear the remaining queue (like the fast drop lock)
                if (game.getState() != GameState::PLAYING) {
                    while(!ai_input_queue.empty()) ai_input_queue.pop();
                }
            } else if (game.getState() != GameState::PLAYING) {
                // ARE or Line clear wait
            }
        } else {
            tick_input = input;
        }

        game.tick(tick_input);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        
        DrawBoard(game);
        
        DrawText(TextFormat("SCORE: %06d", game.getScore()), 450, 100, 20, WHITE);
        DrawText(TextFormat("LEVEL: %d", game.getLevel()), 450, 140, 20, WHITE);
        DrawText(TextFormat("LINES: %d", game.getLines()), 450, 180, 20, WHITE);
        
        std::string stateStr = "PLAYING";
        if (game.getState() == GameState::ENTRY_DELAY) stateStr = "ARE / SPAWNING";
        if (game.getState() == GameState::LINE_CLEAR) stateStr = "LINE CLEAR";
        if (game.getState() == GameState::GAME_OVER) stateStr = "GAME OVER";
        
        DrawText(TextFormat("STATE: %s", stateStr.c_str()), 450, 220, 20, WHITE);
        
        DrawText("NEXT:", 450, 260, 20, WHITE);
        PieceType nextType = game.getNextPieceType();
        if (nextType != PieceType::NONE) {
            auto nextBlocks = Piece::getBlocks(nextType, 0);
            for (const auto& b : nextBlocks) {
                DrawRectangle(450 + (b.x + 2) * CELL_SIZE, 300 + b.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, getPieceColor(nextType));
                DrawRectangleLines(450 + (b.x + 2) * CELL_SIZE, 300 + b.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE);
            }
        }
        
        DrawText("Controls:", 450, 450, 20, LIGHTGRAY);
        DrawText("Arrows: Move/Drop", 450, 480, 20, LIGHTGRAY);
        DrawText("Z/X: Rotate", 450, 510, 20, LIGHTGRAY);
        DrawText("R: Reset", 450, 540, 20, LIGHTGRAY);
        DrawText(TextFormat("A: Toggle AI (%s)", ai_enabled ? "ON" : "OFF"), 450, 570, 20, YELLOW);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
