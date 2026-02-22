# Equity Poker

A full-stack, real-time Texas Hold'em project built with a custom C++ game engine, WebSocket server, and React frontend.

## Project Overview

This project is a multiplayer poker app where users can join a room, sit at a table, play live hands, spectate ongoing games, and view equity information.  
The project focuses on game-state correctness, reconnect-friendly sessions, and practical full-stack system design.

## Features

- Real-time multiplayer table with host, player, and spectator roles
- Full hand lifecycle support (blinds, betting rounds, all-ins, side pots, showdown/muck)
- Perfect Hash hand evaluator (Cactus Kev-style) for fast hand ranking
- Monte Carlo equity simulation engine for live spectator equity updates
- Reconnect flow with identity persistence and host failover logic
- Live chat and spectator tools
- C++ test binaries for core game and lobby behavior

## Tech Stack

- Backend: C++17, uWebSockets, nlohmann/json
- Frontend: React 18, Vite, Zustand, JavaScript

## Quick Start

### 1) Build and run backend

```bash
cd equity-poker
cmake -S . -B build
cmake --build build -j8
./build/game_server
```

Backend runs on port `9001`.

### 2) Run frontend

Open a second terminal:

```bash
cd equity-poker/frontend
npm install
npm run dev
```

Then open the local Vite URL shown in terminal (typically `http://localhost:5173`).

## Run Tests

From `equity-poker`:

```bash
./build/test_game
./build/test_lobby
./build/test_deck
./build/test_equity
```

## Repository Structure

```text
myProject/
├── README.md
└── equity-poker/
    ├── src/
    │   ├── engine/      # Core poker/game logic
    │   ├── server/      # Lobby + WebSocket server
    │   └── poker/       # Card, evaluator, equity calculator
    ├── frontend/        # React client
    ├── tests/           # C++ test programs
    └── CMakeLists.txt
```
