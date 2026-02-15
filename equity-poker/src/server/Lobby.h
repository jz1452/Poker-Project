#pragma once
#include "Game.h"
#include <string>

namespace poker {

// Manages table setup: seating, stacks, and config between hands.
class Lobby {
public:
  Lobby(Game::Config conf = Game::Config()) : game(conf) {}

  int sitPlayer(std::string id, std::string name, int seatIndex);
  bool standPlayer(std::string id);
  void setPlayerStack(int seatIndex, int amount);
  void setButtonPos(int pos);

  Game &getGame() { return game; }
  const Game &getGame() const { return game; }

private:
  Game game;
};

} // namespace poker
