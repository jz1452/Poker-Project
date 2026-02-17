#pragma once
#include "../engine/Game.h"
#include <string>
#include <vector>

namespace poker {

struct LobbyConfig {
  std::string roomCode;
  int maxSeats = 6;
  int startingStack = 1000;
  int smallBlind = 5;
  int bigBlind = 10;
  int actionTimeout = 0; // infinite
  bool godMode = true;   // Spectators see all cards + live equity
};

struct User {
  std::string id;
  std::string name;
  bool isSpectator = true;
  bool isHost = false;
  bool isConnected = true;
};

// Manages table setup, user roles, and game lifecycle.
class Lobby {
public:
  Lobby(Game::Config conf = Game::Config()) : game(conf) {}

  // User management
  bool join(std::string id, std::string name);
  bool leave(std::string id);

  // Player actions (must join() first)
  int sitPlayer(std::string id, int seatIndex, int buyInAmount);
  bool standPlayer(std::string id);
  bool rebuy(std::string id, int amount);

  // Host actions
  bool startGame(std::string hostId);
  bool endGame(std::string hostId);
  bool startNextHand(std::string hostId);
  bool updateConfig(std::string hostId, LobbyConfig newConfig);
  bool kickPlayer(std::string hostId, std::string targetId);
  bool setButtonPos(std::string hostId, int pos);

  // For tests
  void setPlayerStack(int seatIndex, int amount);
  void setButtonPos(int pos);

  // Accessors
  Game &getGame() { return game; }
  const Game &getGame() const { return game; }
  const std::vector<User> &getUsers() const { return users; }
  const LobbyConfig &getLobbyConfig() const { return lobbyConfig; }
  std::string getHostId() const { return hostId; }
  bool isGameInProgress() const { return gameInProgress; }
  bool isUserHost(std::string id) const { return id == hostId; }
  bool isSpectator(const std::string &id) const;

  // Connection management
  void disconnectPlayer(const std::string &id);
  bool reconnectPlayer(const std::string &id);

  // Per-viewer serialisation (handles card masking & spectator view equities)
  nlohmann::json
  toJsonForViewer(const std::string &viewerId,
                  const nlohmann::json *cachedEquities = nullptr) const;
  nlohmann::json computeEquities() const;

  friend void to_json(nlohmann::json &j, const Lobby &l);

private:
  Game game;
  std::vector<User> users;
  std::string hostId;
  bool gameInProgress = false;
  LobbyConfig lobbyConfig;
};

} // namespace poker
