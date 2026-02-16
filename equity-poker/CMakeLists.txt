cmake_minimum_required(VERSION 3.15)
project(EquityPoker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Dependencies
FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

FetchContent_Declare(usockets GIT_REPOSITORY https://github.com/uNetworking/uSockets GIT_TAG master)
FetchContent_MakeAvailable(usockets)

# Manual uSockets target because repo has no CMakeLists.txt
file(GLOB USOCKETS_SOURCES "${usockets_SOURCE_DIR}/src/*.c")
list(APPEND USOCKETS_SOURCES "${usockets_SOURCE_DIR}/src/eventing/epoll_kqueue.c")

find_package(OpenSSL)
if(OpenSSL_FOUND)
    list(APPEND USOCKETS_SOURCES "${usockets_SOURCE_DIR}/src/crypto/openssl.c")
    list(APPEND USOCKETS_SOURCES "${usockets_SOURCE_DIR}/src/crypto/sni_tree.cpp")
    add_library(uSockets STATIC ${USOCKETS_SOURCES})
    target_compile_definitions(uSockets PRIVATE LIBUS_USE_OPENSSL)
    target_link_libraries(uSockets PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    add_library(uSockets STATIC ${USOCKETS_SOURCES})
    target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)
endif()
target_include_directories(uSockets PUBLIC "${usockets_SOURCE_DIR}/src")

FetchContent_Declare(uwebsockets GIT_REPOSITORY https://github.com/uNetworking/uWebSockets GIT_TAG master)
FetchContent_MakeAvailable(uwebsockets)

# Shared source file lists to avoid repetition
set(POKER_SOURCES
    src/poker/Card.cpp
    src/poker/Deck.cpp
    src/poker/Evaluator.cpp
    src/poker/EvaluatorConstants.cpp
    src/poker/EquityCalculator.cpp
)

set(ENGINE_SOURCES
    src/engine/Game.cpp
    src/engine/Player.cpp
)

set(SERVER_SOURCES
    src/server/Lobby.cpp
)

# 1. Game Server Executable
add_executable(game_server
    src/server/GameServer.cpp
    ${SERVER_SOURCES}
    ${ENGINE_SOURCES}
    ${POKER_SOURCES}
)
target_include_directories(game_server PRIVATE 
    src/engine
    src/server 
    src/poker 
    ${usockets_SOURCE_DIR}/src
    ${uwebsockets_SOURCE_DIR}/src
)

find_package(ZLIB REQUIRED)
target_link_libraries(game_server PRIVATE nlohmann_json::nlohmann_json uSockets ZLIB::ZLIB)

# 2. Test: Equity Calculator
add_executable(test_equity 
    tests/TestEquity.cpp 
    ${POKER_SOURCES}
)
target_include_directories(test_equity PRIVATE src/poker)
target_link_libraries(test_equity PRIVATE nlohmann_json::nlohmann_json)

# 3. Test: Deck & Evaluator
add_executable(test_deck
    tests/TestDeck.cpp
    ${POKER_SOURCES}
)
target_include_directories(test_deck PRIVATE src/poker)
target_link_libraries(test_deck PRIVATE nlohmann_json::nlohmann_json)

# 4. Test: Game Engine
add_executable(test_game 
    tests/TestGame.cpp 
    ${SERVER_SOURCES}
    ${ENGINE_SOURCES}
    ${POKER_SOURCES}
)
target_include_directories(test_game PRIVATE src/engine src/server src/poker)
target_link_libraries(test_game PRIVATE nlohmann_json::nlohmann_json)

# 5. Test: Lobby
add_executable(test_lobby 
    tests/TestLobby.cpp 
    ${SERVER_SOURCES}
    ${ENGINE_SOURCES}
    ${POKER_SOURCES}
)
target_include_directories(test_lobby PRIVATE src/engine src/server src/poker)
target_link_libraries(test_lobby PRIVATE nlohmann_json::nlohmann_json)
