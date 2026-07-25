cmake_minimum_required(VERSION 3.20)
project(TetrisAI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -ffast-math")

# Sanitizer options
option(USE_ASAN "Enable AddressSanitizer" OFF)
option(USE_TSAN "Enable ThreadSanitizer" OFF)
option(USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

set(SANITIZER_FLAGS "")
if(USE_ASAN)
    list(APPEND SANITIZER_FLAGS "-fsanitize=address")
endif()
if(USE_TSAN)
    list(APPEND SANITIZER_FLAGS "-fsanitize=thread")
endif()
if(USE_UBSAN)
    list(APPEND SANITIZER_FLAGS "-fsanitize=undefined")
endif()

include(FetchContent)

# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Fetch raylib
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        5.0
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Fetch pybind11
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# -----------------
# Target: tetris_core
# -----------------
add_library(tetris_core STATIC
    engine/Timing.cpp
    engine/Scoring.cpp
    engine/Piece.cpp
    engine/Board.cpp
    engine/Randomizer.cpp
    engine/Game.cpp
    engine/MoveSearch.cpp
    engine/Features.cpp
    engine/Evaluator.cpp
)
target_include_directories(tetris_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(tetris_core PUBLIC ${SANITIZER_FLAGS})
target_link_options(tetris_core PUBLIC ${SANITIZER_FLAGS})

# Enforce Release/O3 and LTO
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
        set_property(TARGET tetris_core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# -----------------
# Target: tetris_render
# -----------------
add_executable(tetris_render render/main.cpp)
target_link_libraries(tetris_render PRIVATE tetris_core raylib)
target_compile_options(tetris_render PRIVATE ${SANITIZER_FLAGS})
target_link_options(tetris_render PRIVATE ${SANITIZER_FLAGS})

# -----------------
# Target: tetris_test
# -----------------
add_executable(tetris_test
    tests/test_timing.cpp
    tests/test_scoring.cpp
    tests/test_piece.cpp
    tests/test_board.cpp
    tests/test_randomizer.cpp
    tests/test_game.cpp
    tests/test_mech.cpp
    tests/test_movesearch.cpp
    tests/test_movesearch_stress.cpp
    tests/test_features.cpp
)
target_link_libraries(tetris_test PRIVATE tetris_core Catch2::Catch2WithMain)
target_compile_options(tetris_test PRIVATE ${SANITIZER_FLAGS})
target_link_options(tetris_test PRIVATE ${SANITIZER_FLAGS})

# -----------------
# Target: Python Bindings
# -----------------
pybind11_add_module(tetris_core_module bindings/tetris_bindings.cpp)
target_link_libraries(tetris_core_module PRIVATE tetris_core)
set_target_properties(tetris_core_module PROPERTIES OUTPUT_NAME tetris_core)
