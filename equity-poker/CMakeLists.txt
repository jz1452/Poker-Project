cmake_minimum_required(VERSION 3.20)
project(EquityPoker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable aggressive warnings 
add_compile_options(-Wall -Wextra -Wpedantic)

# Sources
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable
add_executable(poker_sim main.cpp ${SOURCES})

# Include directories
target_include_directories(poker_sim PRIVATE ${CMAKE_SOURCE_DIR})
