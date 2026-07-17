cmake_minimum_required(VERSION 3.10)
project(BlunderBot VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

# Enable maximum optimizations for a chess engine
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/O2 /GL /W4)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")
endif()

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
    src/book.cpp
    src/misc.cpp
)

set(MAKE_BOOK_SOURCES
    src/make_book.cpp
    src/board.cpp
    src/bitboard.cpp
    src/magic.cpp
    src/movegen.cpp
    src/zobrist.cpp
)

add_executable(BlunderBot ${SOURCES})
add_executable(make_book ${MAKE_BOOK_SOURCES})

find_package(Threads REQUIRED)
target_link_libraries(BlunderBot PRIVATE Threads::Threads)

# Copy the NNUE file to the build directory so the engine can find it
add_custom_command(TARGET BlunderBot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/nn-62ef826d1a6d.nnue"
        $<TARGET_FILE_DIR:BlunderBot>)
