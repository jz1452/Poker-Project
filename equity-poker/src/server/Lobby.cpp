#include "Lobby.h"
#include "../poker/EquityCalculator.h"
#include <nlohmann/json.hpp>

namespace poker {

// User Management

bool Lobby::join(std::string id, std::string name) {
  // Already in room
  for (const auto &u : users) {
    if (u.id == id)
      return false;
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

      // If mid-hand and it's their turn, auto-fold first
      int seatIdx = game.findSeatIndex(id);
      if (seatIdx >= 0 && gameInProgress &&
          game.getStage() != GameStage::Showdown &&
          game.getStage() != GameStage::Idle) {
        if (game.getCurrentActor() == seatIdx) {
          game.playerAction(id, "fold");
        }
      }

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
  // If mid-hand and it's their turn, auto-fold first
  int seatIdx = game.findSeatIndex(id);
  if (seatIdx >= 0 && gameInProgress &&
      game.getStage() != GameStage::Showdown &&
      game.getStage() != GameStage::Idle) {
    if (game.getCurrentActor() == seatIdx) {
      game.playerAction(id, "fold");
    }
  }

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
      (game.getStage() == GameStage::Idle ||
       game.getStage() == GameStage::Showdown || game.getFoldWinner() >= 0);
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
  game.stage = GameStage::Idle;
  return true;
}

bool Lobby::startNextHand(std::string hostId) {
  if (this->hostId != hostId)
    return false;
  if (!gameInProgress)
    return false;

  // Only allow starting a new hand if the current one is over
  bool handOver = (game.getStage() == GameStage::Idle ||
                   game.getStage() == GameStage::Showdown);
  if (!handOver)
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

  // Resize seats vector if maxSeats changed
  if ((int)game.seats.size() != newConfig.maxSeats) {
    game.seats.resize(newConfig.maxSeats);
  }

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

// Test function

void Lobby::setPlayerStack(int seatIndex, int amount) {
  if (seatIndex >= 0 && seatIndex < (int)game.seats.size()) {
    game.seats[seatIndex].chips = amount;
  }
}

void Lobby::setButtonPos(int pos) { game.buttonPos = pos; }

// Connection Management

bool Lobby::isSpectator(const std::string &id) const {
  for (const auto &u : users) {
    if (u.id == id)
      return u.isSpectator;
  }
  return true; // Not found = treat as spectator
}

void Lobby::disconnectPlayer(const std::string &id) {
  // Mark the player's seat as disconnected
  for (auto &p : game.seats) {
    if (p.id == id) {
      p.isConnected = false;
      break;
    }
  }

  // If it's their turn, auto-check or auto-fold
  int seatIdx = game.findSeatIndex(id);
  if (seatIdx >= 0 && game.getCurrentActor() == seatIdx &&
      game.getStage() != GameStage::Showdown &&
      game.getStage() != GameStage::Idle) {
    if (!game.playerAction(id, "check")) {
      game.playerAction(id, "fold");
    }
  }
}

bool Lobby::reconnectPlayer(const std::string &id) {
  // Verify the user exists in the lobby
  bool found = false;
  for (const auto &u : users) {
    if (u.id == id) {
      found = true;
      break;
    }
  }
  if (!found)
    return false;

  // Mark player seat as connected and ready for next hand
  for (auto &p : game.seats) {
    if (p.id == id) {
      p.isConnected = true;
      // If they were SittingOut due to disconnect, make them Waiting
      // so they get dealt into the next hand
      if (p.status == PlayerStatus::SittingOut && p.chips > 0) {
        p.status = PlayerStatus::Waiting;
      }
      break;
    }
  }
  return true;
}

// Per-Viewer Serialisation

nlohmann::json
Lobby::toJsonForViewer(const std::string &viewerId,
                       const nlohmann::json *cachedEquities) const {
  nlohmann::json state = *this; // Uses to_json(Lobby)

  auto &seats = state["game"]["seats"];
  std::string stage = state["game"]["stage"];
  bool atShowdown = (stage == "Showdown");
  bool viewerIsSpc = isSpectator(viewerId);

  // God Mode: spectators see all cards + live equity
  if (viewerIsSpc && lobbyConfig.godMode) {
    std::vector<std::vector<Card>> hands;
    std::vector<int> handSeatIndices;

    for (int i = 0; i < (int)game.seats.size(); i++) {
      const auto &p = game.seats[i];
      if (p.hand.size() == 2 && p.status != PlayerStatus::Folded &&
          p.status != PlayerStatus::SittingOut &&
          p.status != PlayerStatus::Waiting) {
        hands.push_back(p.hand);
        handSeatIndices.push_back(i);
      }
    }

    if (hands.size() >= 2) {
      if (cachedEquities) {
        state["equities"] = *cachedEquities;
      } else {
        state["equities"] = computeEquities();
      }
    }
    return state;
  }

  // Normal view: mask opponents' cards
  for (auto &seat : seats) {
    std::string seatId = seat.value("id", "");
    if (seatId.empty() || seatId == viewerId)
      continue;

    bool show = seat.value("showCards", false);
    if (!atShowdown && !show) {
      int cardCount = seat["hand"].size();
      seat["hand"] = nlohmann::json::array();
      seat["cardCount"] = cardCount;
    }
  }

  // Non-god-mode spectators see no hole cards at all
  if (viewerIsSpc && !atShowdown) {
    for (auto &seat : seats) {
      std::string seatId = seat.value("id", "");
      if (seatId.empty())
        continue;
      bool show = seat.value("showCards", false);
      if (!show) {
        int cardCount = seat["hand"].size();
        seat["hand"] = nlohmann::json::array();
        seat["cardCount"] = cardCount;
      }
    }
  }

  return state;
}

// JSON Serialisation Helpers
void to_json(nlohmann::json &j, const LobbyConfig &c) {
  j = nlohmann::json{{"roomCode", c.roomCode},
                     {"maxSeats", c.maxSeats},
                     {"startingStack", c.startingStack},
                     {"smallBlind", c.smallBlind},
                     {"bigBlind", c.bigBlind},
                     {"actionTimeout", c.actionTimeout},
                     {"godMode", c.godMode}};
}

void to_json(nlohmann::json &j, const User &u) {
  j = nlohmann::json{{"id", u.id},
                     {"name", u.name},
                     {"isSpectator", u.isSpectator},
                     {"isHost", u.isHost}};
}

void to_json(nlohmann::json &j, const Lobby &l) {
  j = nlohmann::json{{"lobbyConfig", l.lobbyConfig},
                     {"users", l.users},
                     {"hostId", l.hostId},
                     {"isGameInProgress", l.gameInProgress},
                     {"game", l.game}};
}

nlohmann::json Lobby::computeEquities() const {
  std::vector<std::vector<Card>> hands;
  std::vector<int> handSeatIndices;

  for (int i = 0; i < (int)game.seats.size(); i++) {
    const auto &p = game.seats[i];
    if (p.hand.size() == 2 && p.status != PlayerStatus::Folded &&
        p.status != PlayerStatus::SittingOut &&
        p.status != PlayerStatus::Waiting) {
      hands.push_back(p.hand);
      handSeatIndices.push_back(i);
    }
  }

  nlohmann::json equityMap = nlohmann::json::object();
  if (hands.size() >= 2) {
    auto equities = EquityCalculator::calculateEquity(hands, game.getBoard());
    for (size_t i = 0; i < handSeatIndices.size(); i++) {
      equityMap[std::to_string(handSeatIndices[i])] = equities[i];
    }
  }
  return equityMap;
}

} // namespace poker
