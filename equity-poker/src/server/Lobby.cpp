#include "Lobby.h"
#include "../poker/EquityCalculator.h"
#include <chrono>
#include <nlohmann/json.hpp>

namespace poker {

static std::string trimCopy(const std::string &input) {
  size_t start = input.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  size_t end = input.find_last_not_of(" \t\r\n");
  return input.substr(start, end - start + 1);
}

void Lobby::cleanupOrphanedSeats() {
  for (auto &p : game.seats) {
    if (p.id.empty())
      continue;

    bool stillInRoom = false;
    for (const auto &u : users) {
      if (u.id == p.id) {
        stillInRoom = true;
        break;
      }
    }

    if (!stillInRoom) {
      p = Player();
      p.status = PlayerStatus::SittingOut;
      p.id.clear();
    }
  }
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
    buyInAmount = game.getGameConfig().startingStack;

  game.seats[seatIndex].id = id;
  game.seats[seatIndex].name = user->name;
  game.seats[seatIndex].chips = buyInAmount;
  game.seats[seatIndex].status = PlayerStatus::Waiting;

  user->isSpectator = false;
  return seatIndex;
}

bool Lobby::standPlayer(std::string id) {
  int seatIdx = game.findSeatIndex(id);
  if (seatIdx < 0)
    return false;

  Player &seat = game.seats[seatIdx];
  const bool handInProgress =
      gameInProgress && game.getStage() != GameStage::Showdown &&
      game.getStage() != GameStage::Idle;
  const bool wasInCurrentHand =
      handInProgress && (seat.status == PlayerStatus::Active ||
                         seat.status == PlayerStatus::Folded ||
                         seat.status == PlayerStatus::AllIn);

  if (wasInCurrentHand) {
    bool foldedOutsideTurnFlow = false;
    if (seat.status == PlayerStatus::Active) {
      if (game.getCurrentActor() == seatIdx) {
        game.playerAction(id, "fold");
      } else {
        seat.status = PlayerStatus::Folded;
        foldedOutsideTurnFlow = true;
      }
    } else if (seat.status == PlayerStatus::AllIn) {
      // Leave/stand should forfeit this hand, same as folding.
      seat.status = PlayerStatus::Folded;
      foldedOutsideTurnFlow = true;
    }

    if (foldedOutsideTurnFlow && game.activePlayerCount() == 1) {
      for (int i = 0; i < game.config.maxSeats; i++) {
        if (game.seats[i].status == PlayerStatus::Active ||
            game.seats[i].status == PlayerStatus::AllIn) {
          game.foldWinner = i;
          break;
        }
      }
      game.distributePot();
    }

    // Vacate seat immediately, but keep committed chips as dead money until
    // hand settlement is complete.
    const int preservedCurrentBet = seat.currentBet;
    const int preservedTotalBet = seat.totalBet;
    seat = Player();
    seat.status = PlayerStatus::SittingOut;
    seat.currentBet = preservedCurrentBet;
    seat.totalBet = preservedTotalBet;
    seat.id.clear();
  } else {
    seat = Player();
    seat.status = PlayerStatus::SittingOut;
    seat.id.clear();
  }

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

  for (auto &p : game.seats) {
    if (p.id == id) {
      p.chips += amount;
      if (p.chips > 0 && p.isConnected &&
          p.status == PlayerStatus::SittingOut) {
        p.status = PlayerStatus::Waiting;
      }
      return true;
    }
  }
  return false; // Not seated
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

  // Reset transient hand state while preserving seated players, chips,
  // and host/lobby membership.
  game.stage = GameStage::Idle;
  game.board.clear();
  game.sidePots.clear();
  game.showdownResults.clear();
  game.isAllInShowdown = false;
  game.foldWinner = -1;
  game.pot = 0;
  game.currentBet = 0;
  game.minRaise = game.config.bigBlind;
  game.currentActor = -1;
  game.lastAggressor = -1;
  game.sbPos = -1;
  game.bbPos = -1;

  for (auto &p : game.seats) {
    p.hand.clear();
    p.currentBet = 0;
    p.totalBet = 0;
    p.showCards = false;

    if (p.id.empty()) {
      p.status = PlayerStatus::SittingOut;
    } else if (p.chips > 0 && p.isConnected) {
      p.status = PlayerStatus::Waiting;
    } else {
      p.status = PlayerStatus::SittingOut;
    }
  }

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
  for (auto &u : users) {
    if (u.id == id) {
      u.isConnected = false;
      if (u.isHost || hostId == id) {
        u.isHost = false;
        hostId.clear();
      }
      break;
    }
  }

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

  // Mark player seat as connected and ready for next hand
  for (auto &p : game.seats) {
    if (p.id == id) {
      p.isConnected = true;
      // mark as waiting so they get dealt into the next hand
      if (p.status == PlayerStatus::SittingOut && p.chips > 0) {
        p.status = PlayerStatus::Waiting;
      }
      break;
    }
  }

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
