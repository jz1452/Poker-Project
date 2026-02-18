#pragma once
#include "../poker/Deck.h"
#include "Player.h"
#include <algorithm>
#include <random>
#include <string>
#include <unordered_set>
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
    hasActedThisStreet.resize(config.maxSeats, false);
    std::random_device rd;
    rng = std::mt19937(rd());
  }

  void startHand();
  bool playerAction(std::string id, std::string action, int amount = 0);
  bool playerMuckOrShow(std::string id, bool show);
  bool sitPlayerAt(int seatIndex, const std::string &id, const std::string &name,
                   int chips);
  bool rebuyPlayer(const std::string &id, int amount);
  bool forfeitAndVacateSeat(const std::string &id, bool handInProgress);
  bool setPlayerConnected(const std::string &id, bool connected);
  bool markWaitingIfEligible(const std::string &id);
  void removeOrphanedSeats(const std::unordered_set<std::string> &validUserIds);
  int seatedPlayerCountWithChips() const;
  void resetForEndGame();
  bool applyConfig(const Config &newConfig);
  bool setButtonPosition(int pos);
  void setSeatStackForTesting(int seatIndex, int amount);
  int seatCount() const { return static_cast<int>(seats.size()); }
  bool isSeatOpen(int seatIndex) const;

  // State accessors
  const std::vector<Player> &getSeats() const { return seats; }
  const std::vector<Card> &getBoard() const { return board; }
  int getPot() const { return pot; }
  int getCurrentActor() const { return currentActor; }
  Config getGameConfig() const { return config; }
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
  std::vector<bool> hasActedThisStreet;

  GameStage stage = GameStage::Idle;

  std::vector<ShowdownResult> showdownResults;
  bool isAllInShowdown = false;
  int foldWinner = -1;

  void nextTurn();
  void nextStreet();
  void resolveSidePots();
  void distributePot();
  void checkShowdownResolved();
  bool resolveIfSingleActiveRemains();
  bool autoResolveDisconnectedTurnAtCurrentActor();
  void autoRunoutRemainingStreets();
  void vacateSeat(int seatIdx, bool preserveCommittedBets);
  bool bettingRoundComplete() const;
  int nextActorNeedingAction(int current) const;
  int bettingPlayerCount() const;
  int nextActivePlayer(int current);
  int activePlayerCount() const;
};

} // namespace poker
