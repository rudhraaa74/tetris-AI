#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "engine/Game.h"
#include "engine/Input.h"
#include "engine/MoveSearch.h"
#include "engine/Features.h"
#include "engine/Evaluator.h"
#include <vector>
#include <iostream>
#include <chrono>

namespace py = pybind11;
using namespace tetris;

std::vector<Input> get_placement_inputs(const Placement& p) {
    std::vector<Input> inputs;
    inputs.reserve(p.num_inputs);
    for (int i = 0; i < p.num_inputs; ++i) {
        uint8_t mask = p.input_masks[i];
        inputs.push_back(Input{
            (mask & 1) != 0,
            (mask & 2) != 0,
            (mask & 8) != 0,
            (mask & 16) != 0,
            (mask & 4) != 0
        });
    }
    return inputs;
}

PYBIND11_MODULE(tetris_core, m) {
    m.doc() = "Tetris Engine Python Bindings";

    py::enum_<GameState>(m, "GameState")
        .value("PLAYING", GameState::PLAYING)
        .value("ENTRY_DELAY", GameState::ENTRY_DELAY)
        .value("LINE_CLEAR", GameState::LINE_CLEAR)
        .value("GAME_OVER", GameState::GAME_OVER)
        .export_values();

    py::enum_<PieceType>(m, "PieceType")
        .value("T", PieceType::T)
        .value("J", PieceType::J)
        .value("Z", PieceType::Z)
        .value("O", PieceType::O)
        .value("S", PieceType::S)
        .value("L", PieceType::L)
        .value("I", PieceType::I)
        .export_values();

    py::class_<Input>(m, "Input")
        .def(py::init<>())
        .def(py::init<bool, bool, bool, bool, bool>(),
             py::arg("left")=false, py::arg("right")=false, py::arg("cw")=false, py::arg("ccw")=false, py::arg("down")=false)
        .def_readwrite("left", &Input::left)
        .def_readwrite("right", &Input::right)
        .def_readwrite("rotateCW", &Input::rotateCW)
        .def_readwrite("rotateCCW", &Input::rotateCCW)
        .def_readwrite("softDrop", &Input::softDrop);

    py::class_<Placement>(m, "Placement")
        .def_readonly("x", &Placement::x)
        .def_readonly("y", &Placement::y)
        .def_readonly("rotation", &Placement::rotation)
        .def_readonly("required_frames", &Placement::required_frames)
        .def_property_readonly("inputs", &get_placement_inputs);

    py::class_<Board>(m, "Board")
        .def(py::init<>())
        .def("get_cell", &Board::getCell)
        .def("get_grid", [](const Board& b) {
            std::vector<std::vector<int>> grid(22, std::vector<int>(10));
            for(int r=0; r<22; ++r) {
                for(int c=0; c<10; ++c) {
                    grid[r][c] = static_cast<int>(b.getCell(c, r));
                }
            }
            return grid;
        });

    py::class_<Game>(m, "Game")
        .def(py::init<int, uint16_t>(), py::arg("startLevel") = 0, py::arg("seed") = 0x8988)
        .def("reset", &Game::reset, py::arg("startLevel") = 0, py::arg("seed") = 0x8988)
        .def("tick", &Game::tick)
        .def("getState", &Game::getState)
        .def("isGameOver", [](const Game& g) { return g.getState() == GameState::GAME_OVER; })
        .def("getScore", &Game::getScore)
        .def("getLevel", &Game::getLevel)
        .def("getLines", &Game::getLines)
        .def("getTetrises", &Game::getTetrises)
        .def("getActivePieceType", &Game::getActivePieceType)
        .def("getNextPieceType", &Game::getNextPieceType)
        .def("getActiveX", &Game::getActiveX)
        .def("getActiveY", &Game::getActiveY)
        .def("getActiveRotation", &Game::getActiveRotation)
        .def("getBoard", &Game::getBoard, py::return_value_policy::reference_internal);

    py::class_<MoveSearch>(m, "MoveSearch")
        .def(py::init<>())
        .def("getValidPlacements", [](MoveSearch& self, const Game& game) {
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
            state.frames = 0; // relative search depth

            FixedCapacityVector<Placement, 128> placements;
            self.search(game.getBoard(), game.getActivePieceType(), game.getLevel(), state, &placements);
            std::vector<Placement> result(placements.begin(), placements.end());
            return result;
        });

    m.def("extract_features", [](const Board& initial_board, PieceType type, int rotation, int x, int y) {
        Board sim_board = initial_board;
        // Lock piece manually
        sim_board.lockPiece(type, rotation, x, y);
        
        // Count eroded cells
        int piece_cells_cleared = 0;
        auto fullLines = sim_board.getFullLines();
        for (const auto& block : Piece::getBlocks(type, rotation)) {
            int by = y + block.y;
            if (std::find(fullLines.begin(), fullLines.end(), by) != fullLines.end()) {
                piece_cells_cleared++;
            }
        }
        
        int lines_cleared = fullLines.size();
        sim_board.removeLines(fullLines);
        
        return Features::extract(sim_board, y, lines_cleared, piece_cells_cleared);
    });

    m.def("run_seeded_game_depth2", [](int seed, const std::array<double, 9>& weights, int level, int max_moves, int core_id) {
        Game game(level, seed);
        
        MoveSearch searcher;
        int moves = 0;
        int last_log_lines = 0;
        auto start_time = std::chrono::steady_clock::now();
        
        while (game.getState() != GameState::GAME_OVER && (max_moves < 0 || moves < max_moves)) {
            if (game.getState() == GameState::ENTRY_DELAY || game.getState() == GameState::LINE_CLEAR) {
                Input null_input;
                game.tick(null_input);
                continue;
            }
            
            Placement best_p = Evaluator::getBestPlacementDepth2(game, weights, searcher);
            
            // Execute the placement
            for (int i = 0; i < best_p.num_inputs; ++i) {
                uint8_t mask = best_p.input_masks[i];
                Input in{
                    (mask & 1) != 0,
                    (mask & 2) != 0,
                    (mask & 8) != 0,
                    (mask & 16) != 0,
                    (mask & 4) != 0
                };
                game.tick(in);
                if (game.getState() != GameState::PLAYING) break;
            }
            
            // Fast forward until lock
            Input soft_drop;
            soft_drop.softDrop = true;
            while (game.getState() == GameState::PLAYING) {
                game.tick(soft_drop);
            }
            
            moves++;
            
            if (game.getLines() - last_log_lines >= 1000) {
                last_log_lines = (game.getLines() / 1000) * 1000;
                auto now = std::chrono::steady_clock::now();
                std::chrono::duration<double> diff = now - start_time;
                double speed = moves / diff.count();
                std::cout << "[Core " << core_id << "] Seed " << seed 
                          << " | Lines: " << last_log_lines 
                          << " | Score: " << game.getScore() 
                          << " | Speed: " << (int)speed << " moves/sec" << std::endl;
            }
        }
        
        return py::make_tuple(game.getScore(), game.getLines(), game.getTetrises(), moves);
    }, py::arg("seed"), py::arg("weights"), py::arg("level"), py::arg("max_moves") = -1, py::arg("core_id") = 0);

    m.def("get_best_placement_depth2", [](const Game& game, const std::array<double, 9>& weights, MoveSearch& searcher) {
        return Evaluator::getBestPlacementDepth2(game, weights, searcher);
    });
}
