#include "Lobby.h"

namespace poker {

// --- User Management ---

bool Lobby::join(std::string id, std::string name) {
  for (const auto &u : users) {
    if (u.id == id)
      return false; // Already in room
  }

  User u;
  u.id = id;
  u.name = name;
  u.isSpectator = true;

  if (users.empty()) {
    u.isHost = true;
    hostId = id;
  }

  users.push_back(u);
  return true;
}

bool Lobby::leave(std::string id) {
  for (auto it = users.begin(); it != users.end(); ++it) {
    if (it->id == id) {
      bool wasHost = it->isHost;

      standPlayer(id);
      users.erase(it);

      if (wasHost && !users.empty()) {
        users[0].isHost = true;
        hostId = users[0].id;
      } else if (users.empty()) {
        hostId = "";
        gameInProgress = false;
      }
      return true;
    }
  }
  return false;
}

// Player Actions

int Lobby::sitPlayer(std::string id, int seatIndex, int buyInAmount) {
  // Must have joined the room first
  User *user = nullptr;
  for (auto &u : users) {
    if (u.id == id) {
      user = &u;
      break;
    }
  }
  if (!user)
    return -1;

  if (seatIndex < 0 || seatIndex >= (int)game.seats.size())
    return -1;

  // Seat must be empty
  if (game.seats[seatIndex].status != PlayerStatus::SittingOut &&
      !game.seats[seatIndex].id.empty())
    return -1;

  // Can't sit in two seats
  for (const auto &p : game.seats) {
    if (p.id == id)
      return -1;
  }

  if (buyInAmount <= 0)
    return -1;

  game.seats[seatIndex].id = id;
  game.seats[seatIndex].name = user->name;
  game.seats[seatIndex].chips = buyInAmount;
  game.seats[seatIndex].status = PlayerStatus::Waiting;

  user->isSpectator = false;
  return seatIndex;
}

bool Lobby::standPlayer(std::string id) {
  bool found = false;
  for (auto &p : game.seats) {
    if (p.id == id) {
      p = Player();
      p.status = PlayerStatus::SittingOut;
      p.id = "";
      found = true;
      break;
    }
  }

  if (found) {
    for (auto &u : users) {
      if (u.id == id) {
        u.isSpectator = true;
        break;
      }
    }
  }
  return found;
}

bool Lobby::rebuy(std::string id, int amount) {
  // Rebuy allowed if: no game running, OR hand is over
  bool handOver =
      (game.getStage() == GameStage::Showdown || game.getFoldWinner() >= 0);
  if (gameInProgress && !handOver)
    return false;
  if (amount <= 0)
    return false;

  for (auto &p : game.seats) {
    if (p.id == id) {
      p.chips += amount;
      return true;
    }
  }
  return false; // Not seated
}

// Host Actions

bool Lobby::startGame(std::string hostId) {
  if (this->hostId != hostId)
    return false;
  if (gameInProgress)
    return false;

  int count = 0;
  for (const auto &p : game.seats) {
    if (p.status != PlayerStatus::SittingOut && p.chips > 0)
      count++;
  }
  if (count < 2)
    return false;

  gameInProgress = true;
  game.startHand();
  return true;
}

bool Lobby::endGame(std::string hostId) {
  if (this->hostId != hostId)
    return false;
  gameInProgress = false;
  return true;
}

bool Lobby::startNextHand(std::string hostId) {
  if (this->hostId != hostId)
    return false;
  if (!gameInProgress)
    return false;

  int count = 0;
  for (const auto &p : game.seats) {
    if (p.status != PlayerStatus::SittingOut && p.chips > 0)
      count++;
  }
  if (count < 2) {
    gameInProgress = false;
    return false;
  }

  game.startHand();
  return true;
}

bool Lobby::updateConfig(std::string hostId, LobbyConfig newConfig) {
  if (this->hostId != hostId)
    return false;
  if (gameInProgress)
    return false;

  lobbyConfig = newConfig;

  Game::Config gc;
  gc.maxSeats = newConfig.maxSeats;
  gc.smallBlind = newConfig.smallBlind;
  gc.bigBlind = newConfig.bigBlind;
  gc.startingStack = newConfig.startingStack;
  game.config = gc;

  return true;
}

bool Lobby::kickPlayer(std::string hostId, std::string targetId) {
  if (this->hostId != hostId)
    return false;
  if (hostId == targetId)
    return false;
  return leave(targetId);
}

bool Lobby::setButtonPos(std::string hostId, int pos) {
  if (this->hostId != hostId)
    return false;
  game.buttonPos = pos;
  return true;
}

// --- For tests ---

void Lobby::setPlayerStack(int seatIndex, int amount) {
  if (seatIndex >= 0 && seatIndex < (int)game.seats.size()) {
    game.seats[seatIndex].chips = amount;
  }
}

void Lobby::setButtonPos(int pos) { game.buttonPos = pos; }

} // namespace poker
