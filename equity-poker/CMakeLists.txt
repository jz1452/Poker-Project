cmake_minimum_required(VERSION 3.20)
project(EquityPoker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

# Sources (The Core Library)
# We exclude main.cpp from here so multiple targets can link against it
file(GLOB_RECURSE POKER_LIB_SOURCES 
    "src/poker/*.cpp"
    "src/engine/*.cpp"
)

# Target 1: The Main Game Server (Phase 3)
add_executable(poker_sim main.cpp ${POKER_LIB_SOURCES})
target_include_directories(poker_sim PRIVATE ${CMAKE_SOURCE_DIR})

# Target 2: The Deck Test (Phase 1)
add_executable(test_deck tests/TestDeck.cpp ${POKER_LIB_SOURCES})
target_include_directories(test_deck PRIVATE ${CMAKE_SOURCE_DIR})
