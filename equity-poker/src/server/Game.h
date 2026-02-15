#pragma once
#include "../poker/Deck.h"
#include "Player.h"
#include <algorithm>
#include <random> // Added for RNG
#include <string>
#include <vector>

namespace poker {

struct SidePot {
  int amount = 0;
  std::vector<std::string>
      eligiblePlayers; // IDs of players who can win this pot
};

enum class GameStage { PreFlop, Flop, Turn, River, Showdown };

class Game {
public:
  struct Config {
    int smallBlind = 5;
    int bigBlind = 10;
    int maxSeats = 9;
    int startingStack = 1000;

    Config() {}
  };

  Game(Config c = Config()) : config(c) {
    seats.resize(config.maxSeats);
    std::random_device rd;
    rng = std::mt19937(rd());
  }

  // --- Lobby Actions ---
  // Returns seat index or -1
  int sitPlayer(std::string id, std::string name, int seatIndex);
  bool standPlayer(std::string id);

  // --- Game Flow ---
  void startHand();
  // action: "fold", "call", "raise", "check", "allin"
  bool playerAction(std::string id, std::string action, int amount = 0);

  // --- State Access ---
  const std::vector<Player> &getSeats() const { return seats; }
  const std::vector<Card> &getBoard() const { return board; }
  int getPot() const { return pot; }
  int getCurrentActor() const { return currentActor; }
  GameStage getStage() const { return stage; }
  int getButtonPos() const { return buttonPos; }
  int getCurrentBet() const { return currentBet; }
  int getMinRaise() const { return minRaise; }

private:
  Config config;
  std::vector<Player> seats;
  std::vector<SidePot> sidePots;
  std::vector<Card> board;
  Deck deck;
  std::mt19937 rng; // The Game's private RNG

  int pot = 0;
  int buttonPos = 0;
  int currentActor = 0; // Index of player who must act

  // Store positions explicitly to handle Heads-Up logic easily
  int sbPos = -1;
  int bbPos = -1;

  // Betting State
  int minRaise = 0;
  int currentBet = 0;
  int lastAggressor = -1; // Index of the last player to raise

  GameStage stage = GameStage::PreFlop;

  // Internal Helpers
  void nextTurn();
  void nextStreet();
  void resolveSidePots();
  void distributePot();
  int nextActivePlayer(int current);
  int nextBettingPlayer(int current);
  int activePlayerCount() const;
};

} // namespace poker