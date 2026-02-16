#pragma once
#include "../poker/Deck.h"
#include "Player.h"
#include <algorithm>
#include <random>
#include <string>
#include <vector>

namespace poker {

struct SidePot {
  int amount = 0;
  std::vector<std::string> eligiblePlayers;
};

// Per-player showdown results for frontend display
struct ShowdownResult {
  int seatIndex = -1;
  int handRank = 99999; // Lower is better
  int chipsWon = 0;
  bool mustShow = false;   // True for winners and all-in players
  bool hasDecided = false; // True once player has made their muck/show choice
};

enum class GameStage { Idle, PreFlop, Flop, Turn, River, Showdown };

class Game {
  friend class Lobby;

public:
  struct Config {
    int smallBlind = 5;
    int bigBlind = 10;
    int maxSeats = 6;
    int startingStack = 1000;
    Config() {}
  };

  Game(Config c = Config()) : config(c) {
    seats.resize(config.maxSeats);
    std::random_device rd;
    rng = std::mt19937(rd());
  }

  void startHand();
  bool playerAction(std::string id, std::string action, int amount = 0);
  bool playerMuckOrShow(std::string id, bool show);

  // State accessors
  const std::vector<Player> &getSeats() const { return seats; }
  const std::vector<Card> &getBoard() const { return board; }
  int getPot() const { return pot; }
  int getCurrentActor() const { return currentActor; }
  GameStage getStage() const { return stage; }
  int getButtonPos() const { return buttonPos; }
  int getCurrentBet() const { return currentBet; }
  int getMinRaise() const { return minRaise; }
  int findSeatIndex(const std::string &id) const;
  const std::vector<ShowdownResult> &getShowdownResults() const {
    return showdownResults;
  }
  bool getIsAllInShowdown() const { return isAllInShowdown; }
  int getFoldWinner() const { return foldWinner; }

  friend void to_json(nlohmann::json &j, const Game &g);

private:
  Config config;
  std::vector<Player> seats;
  std::vector<SidePot> sidePots;
  std::vector<Card> board;
  Deck deck;
  std::mt19937 rng;

  int pot = 0;
  int buttonPos = -1;
  int currentActor = 0;
  int sbPos = -1;
  int bbPos = -1;

  int minRaise = 0;
  int currentBet = 0;
  int lastAggressor = -1;

  GameStage stage = GameStage::Idle;

  std::vector<ShowdownResult> showdownResults;
  bool isAllInShowdown = false;
  int foldWinner = -1;

  void nextTurn();
  void nextStreet();
  void resolveSidePots();
  void distributePot();
  void checkShowdownResolved(); // Auto-transition Showdown → Idle
  int nextActivePlayer(int current);
  int nextBettingPlayer(int current);
  int activePlayerCount() const;
};

} // namespace poker