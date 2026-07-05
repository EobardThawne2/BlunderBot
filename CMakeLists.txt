cmake_minimum_required(VERSION 3.10)
project(BlunderBot VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

# Enable maximum optimizations for a chess engine
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")

set(SOURCES
    src/main.cpp
    src/board.cpp
    src/bitboard.cpp
    src/magic.cpp
    src/movegen.cpp
    src/evaluate.cpp
    src/search.cpp
    src/tt.cpp
    src/zobrist.cpp
    src/uci.cpp
    src/nnue.cpp
    src/misc.cpp
)

add_executable(BlunderBot ${SOURCES})

# Copy the NNUE file to the build directory so the engine can find it
add_custom_command(TARGET BlunderBot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/nn-62ef826d1a6d.nnue"
        $<TARGET_FILE_DIR:BlunderBot>)
