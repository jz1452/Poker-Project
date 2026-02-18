#include "Lobby.h"
#include "../poker/EquityCalculator.h"
#include <chrono>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace poker {

void to_json(nlohmann::json &j, const Game &g);
void to_json(nlohmann::json &j, const LobbyConfig &c);
void to_json(nlohmann::json &j, const User &u);
void to_json(nlohmann::json &j, const ChatMessage &m);

static std::string trimCopy(const std::string &input) {
  size_t start = input.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  size_t end = input.find_last_not_of(" \t\r\n");
  return input.substr(start, end - start + 1);
}

void Lobby::cleanupOrphanedSeats() {
  std::unordered_set<std::string> validUserIds;
  for (const auto &u : users) {
    validUserIds.insert(u.id);
  }
  game.removeOrphanedSeats(validUserIds);
}

// User Management

bool Lobby::join(std::string id, std::string name) {
  // Already in room
  for (const auto &u : users) {
    if (u.id == id)
      return false;
  }

  bool hasConnectedHost = false;
  for (const auto &u : users) {
    if (u.isHost && u.isConnected) {
      hasConnectedHost = true;
      break;
    }
  }

  User u;
  u.id = id;
  u.name = name;
  u.isSpectator = true;
  u.isConnected = true;

  if (!hasConnectedHost) {
    for (auto &existing : users) {
      existing.isHost = false;
    }
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

      if (users.empty()) {
        gameInProgress = false;
        hostId.clear();
        chatMessages.clear();
        nextChatMessageId = 1;
        return true;
      }

      if (wasHost || hostId == id) {
        hostId.clear();
        for (auto &u : users) {
          u.isHost = false;
        }
        for (auto &u : users) {
          if (u.isConnected) {
            u.isHost = true;
            hostId = u.id;
            break;
          }
        }
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

  if (seatIndex < 0 || seatIndex >= game.seatCount())
    return -1;

  if (buyInAmount <= 0)
    buyInAmount = game.getGameConfig().startingStack;

  if (!game.sitPlayerAt(seatIndex, id, user->name, buyInAmount))
    return -1;

  user->isSpectator = false;
  return seatIndex;
}

bool Lobby::standPlayer(std::string id) {
  const bool handInProgress =
      gameInProgress && game.getStage() != GameStage::Showdown &&
      game.getStage() != GameStage::Idle;

  if (!game.forfeitAndVacateSeat(id, handInProgress))
    return false;

  for (auto &u : users) {
    if (u.id == id) {
      u.isSpectator = true;
      break;
    }
  }

  return true;
}

bool Lobby::rebuy(std::string id, int amount) {
  // Rebuy is only allowed when table is in Idle (between hands)
  if (game.getStage() != GameStage::Idle)
    return false;
  if (amount <= 0)
    return false;
  return game.rebuyPlayer(id, amount);
}

bool Lobby::handleGameAction(const std::string &userId,
                             const std::string &command, int amount) {
  return game.playerAction(userId, command, amount);
}

bool Lobby::handleMuckOrShow(const std::string &userId, bool show) {
  return game.playerMuckOrShow(userId, show);
}

bool Lobby::addChatMessage(const std::string &userId, const std::string &text) {
  const User *sender = nullptr;
  for (const auto &u : users) {
    if (u.id == userId) {
      sender = &u;
      break;
    }
  }
  if (!sender)
    return false;

  std::string cleaned = trimCopy(text);
  if (cleaned.empty())
    return false;
  if (cleaned.size() > 280)
    cleaned = cleaned.substr(0, 280);

  ChatMessage msg;
  msg.id = std::to_string(nextChatMessageId++);
  msg.userId = userId;
  msg.name = sender->name;
  msg.text = cleaned;
  msg.timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  chatMessages.push_back(std::move(msg));
  if ((int)chatMessages.size() > maxChatMessages) {
    chatMessages.erase(chatMessages.begin());
  }
  return true;
}

// Host Actions

bool Lobby::startGame(std::string hostId) {
  if (this->hostId != hostId)
    return false;
  if (gameInProgress)
    return false;

  cleanupOrphanedSeats();

  if (game.seatedPlayerCountWithChips() < 2)
    return false;

  gameInProgress = true;
  game.startHand();
  return true;
}

bool Lobby::endGame(std::string hostId) {
  if (this->hostId != hostId)
    return false;

  gameInProgress = false;
  game.resetForEndGame();
  return true;
}

bool Lobby::startNextHand(std::string hostId) {
  if (this->hostId != hostId)
    return false;
  if (!gameInProgress)
    return false;

  // Only allow starting a new hand when the table is explicitly Idle
  if (game.getStage() != GameStage::Idle)
    return false;

  cleanupOrphanedSeats();

  if (game.seatedPlayerCountWithChips() < 2) {
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

  Game::Config gc;
  gc.maxSeats = newConfig.maxSeats;
  gc.smallBlind = newConfig.smallBlind;
  gc.bigBlind = newConfig.bigBlind;
  gc.startingStack = newConfig.startingStack;
  if (!game.applyConfig(gc)) {
    return false;
  }

  lobbyConfig = newConfig;

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
  return game.setButtonPosition(pos);
}

// Test function

void Lobby::setPlayerStack(int seatIndex, int amount) {
  game.setSeatStackForTesting(seatIndex, amount);
}

void Lobby::setButtonPos(int pos) { game.setButtonPosition(pos); }

// Connection Management

bool Lobby::isSpectator(const std::string &id) const {
  for (const auto &u : users) {
    if (u.id == id)
      return u.isSpectator;
  }
  return true; // Not found = treat as spectator
}

void Lobby::disconnectPlayer(const std::string &id) {
  bool disconnectedHost = false;
  for (auto &u : users) {
    if (u.id == id) {
      u.isConnected = false;
      if (u.isHost || hostId == id) {
        u.isHost = false;
        hostId.clear();
        disconnectedHost = true;
      }
      break;
    }
  }

  game.setPlayerConnection(id, false);

  if (disconnectedHost) {
    for (auto &u : users) {
      if (u.isConnected) {
        u.isHost = true;
        hostId = u.id;
        break;
      }
    }
  }
}


bool Lobby::reconnectPlayer(const std::string &id) {
  // Verify the user exists in the lobby and mark them connected.
  bool found = false;
  for (auto &u : users) {
    if (u.id == id) {
      u.isConnected = true;
      found = true;
      break;
    }
  }
  if (!found)
    return false;

  game.setPlayerConnection(id, true);
  game.markWaitingIfEligible(id);

  bool hasConnectedHost = false;
  for (const auto &u : users) {
    if (u.isHost && u.isConnected) {
      hasConnectedHost = true;
      break;
    }
  }

  if (!hasConnectedHost) {
    for (auto &u : users) {
      u.isHost = false;
    }
    for (auto &u : users) {
      if (u.id == id) {
        u.isHost = true;
        hostId = id;
        break;
      }
    }
  }

  return true;
}

// Per-Viewer Serialisation

nlohmann::json
Lobby::toJsonForViewer(const std::string &viewerId,
                       bool includeEquities,
                       const nlohmann::json *cachedEquities) const {
  nlohmann::json state{
      {"lobbyConfig", lobbyConfig},
      {"users", users},
      {"chatMessages", chatMessages},
      {"hostId", hostId},
      {"isGameInProgress", gameInProgress},
      {"game", game},
  };

  auto &seats = state["game"]["seats"];
  std::string stage = state["game"]["stage"];
  bool atShowdown = (stage == "Showdown");
  bool viewerIsSpc = isSpectator(viewerId);

  // God Mode: spectators see all cards + live equity
  if (viewerIsSpc && lobbyConfig.godMode) {
    const auto &gameSeats = game.getSeats();
    int visibleHandCount = 0;
    for (int i = 0; i < static_cast<int>(gameSeats.size()); i++) {
      const auto &p = gameSeats[i];
      if (p.hand.size() == 2 && p.status != PlayerStatus::Folded &&
          p.status != PlayerStatus::SittingOut &&
          p.status != PlayerStatus::Waiting) {
        visibleHandCount++;
      }
    }

    if (visibleHandCount >= 2) {
      if (cachedEquities) {
        state["equities"] = *cachedEquities;
      } else if (includeEquities) {
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
                     {"isHost", u.isHost},
                     {"isConnected", u.isConnected}};
}

void to_json(nlohmann::json &j, const ChatMessage &m) {
  j = nlohmann::json{{"id", m.id},
                     {"userId", m.userId},
                     {"name", m.name},
                     {"text", m.text},
                     {"timestamp", m.timestamp}};
}

void to_json(nlohmann::json &j, const Lobby &l) {
  j = nlohmann::json{{"lobbyConfig", l.lobbyConfig},
                     {"users", l.users},
                     {"chatMessages", l.chatMessages},
                     {"hostId", l.hostId},
                     {"isGameInProgress", l.gameInProgress},
                     {"game", l.game}};
}

nlohmann::json Lobby::computeEquities() const {
  std::vector<std::vector<Card>> hands;
  std::vector<int> handSeatIndices;
  const auto &gameSeats = game.getSeats();

  for (int i = 0; i < static_cast<int>(gameSeats.size()); i++) {
    const auto &p = gameSeats[i];
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
