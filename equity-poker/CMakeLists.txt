cmake_minimum_required(VERSION 3.10)
project(EquityPoker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Main Executable (Phase 1)
add_executable(poker_eval 
    main.cpp 
    src/poker/Card.cpp 
    src/poker/Deck.cpp
    src/poker/Evaluator.cpp
    src/poker/EvaluatorConstants.cpp
    src/poker/EquityCalculator.cpp
)
target_include_directories(poker_eval PRIVATE src/poker)

# 2. Test Executable (Phase 2 - Equity)
add_executable(test_equity 
    tests/TestEquity.cpp 
    src/poker/Card.cpp 
    src/poker/Deck.cpp
    src/poker/Evaluator.cpp
    src/poker/EvaluatorConstants.cpp
    src/poker/EquityCalculator.cpp
)
target_include_directories(test_equity PRIVATE src/poker)

# 3. Test Executable (Phase 3 - Game Logic)
add_executable(test_game 
    tests/TestGame.cpp 
    src/server/Game.cpp 
    src/server/Lobby.cpp
    src/poker/Card.cpp 
    src/poker/Deck.cpp
    src/poker/Evaluator.cpp
    src/poker/EvaluatorConstants.cpp
)
target_include_directories(test_game PRIVATE src/server src/poker)

# 4. Test Executable (Phase 3 - Lobby Logic)
add_executable(test_lobby 
    tests/TestLobby.cpp 
    src/server/Game.cpp 
    src/server/Lobby.cpp
    src/poker/Card.cpp 
    src/poker/Deck.cpp
    src/poker/Evaluator.cpp
    src/poker/EvaluatorConstants.cpp
)
target_include_directories(test_lobby PRIVATE src/server src/poker)
