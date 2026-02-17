#include "App.h"
#include "Lobby.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

struct PerSocketData {
  std::string userId;
};

using WebSocket = uWS::WebSocket<false, true, PerSocketData>;

// Global state
poker::Lobby lobby;
std::unordered_map<std::string, WebSocket *> connectedSockets;

// Send personalised state to every connected client
void broadcastToAll() {
  json cachedEquities;
  json *equitiesPtr = nullptr;

  // Pre-calculate equities if God Mode is active
  // avoids running Monte Carlo sim for every spectator
  if (lobby.getLobbyConfig().godMode) {
    cachedEquities = lobby.computeEquities();
    equitiesPtr = &cachedEquities;
  }

  std::string spectatorPayload;
  bool spectatorPayloadReady = false;

  for (auto &[userId, ws] : connectedSockets) {
    // Spectators see identical state: we can serialize once and reuse the
    // string.
    if (lobby.isSpectator(userId)) {
      if (!spectatorPayloadReady) {
        json msg;
        msg["type"] = "gameState";
        // Use empty ID for generic spectator view
        msg["data"] = lobby.toJsonForViewer("", equitiesPtr);
        spectatorPayload = msg.dump();
        spectatorPayloadReady = true;
      }
      ws->send(spectatorPayload, uWS::OpCode::TEXT);
    } else {
      // Active players need unique views
      json msg;
      msg["type"] = "gameState";
      msg["data"] = lobby.toJsonForViewer(userId, equitiesPtr);
      ws->send(msg.dump(), uWS::OpCode::TEXT);
    }
  }
}

int main() {
  std::cout << "Starting Game Server on port 9001..." << std::endl;

  uWS::App()
      .ws<PerSocketData>(
          "/*",
          {.compression = uWS::SHARED_COMPRESSOR,
           .maxPayloadLength = 16 * 1024,
           .idleTimeout = 120,

           .open =
               [](WebSocket *ws) {
                 std::cout << "Client connected!" << std::endl;
               },

           .message =
               [](WebSocket *ws, std::string_view message, uWS::OpCode opCode) {
                 try {
                   auto j = json::parse(message);
                   std::string action = j.value("action", "");
                   PerSocketData *userData = ws->getUserData();

                   std::cout << "Received: " << message << std::endl;

                   bool success = false;
                   std::string errorMsg;

                   if (action == "join") {
                     std::string name = j.value("name", "");
                     std::string id = j.value("id", "");
                     if (name.empty()) {
                       errorMsg = "Missing 'name' field";
                     } else {
                       if (id.empty()) {
                         std::random_device rd;
                         id = "user_" + std::to_string(rd() % 900000 + 100000);
                       }
                       // Try reconnect first, then fresh join
                       if (lobby.reconnectPlayer(id)) {
                         userData->userId = id;
                         connectedSockets[id] = ws;
                         std::cout << "Player reconnected: " << id << std::endl;
                         success = true;
                       } else if (lobby.join(id, name)) {
                         userData->userId = id;
                         connectedSockets[id] = ws;
                         success = true;
                       } else {
                         errorMsg = "Could not join";
                       }
                       // Send assigned ID back to client
                       if (success) {
                         json joinResp;
                         joinResp["type"] = "joinSuccess";
                         joinResp["userId"] = id;
                         ws->send(joinResp.dump(), uWS::OpCode::TEXT);
                       }
                     }
                   } else if (action == "sit") {
                     if (userData->userId.empty()) {
                       errorMsg = "Must join first";
                     } else {
                       int seat = j.value("seatIndex", -1);
                       int buyIn = j.value("buyIn", 0);
                       if (seat < 0) {
                         errorMsg = "Missing 'seatIndex'";
                       } else if (lobby.sitPlayer(userData->userId, seat,
                                                  buyIn) != -1) {
                         success = true;
                       } else {
                         errorMsg =
                             "Could not sit (seat taken or invalid buy-in)";
                       }
                     }
                   } else if (action == "stand") {
                     success = lobby.standPlayer(userData->userId);
                     if (!success)
                       errorMsg = "Could not stand";
                   } else if (action == "start_game") {
                     success = lobby.startGame(userData->userId);
                     if (!success)
                       errorMsg = "Failed (not host or not enough players)";
                   } else if (action == "start_next_hand") {
                     success = lobby.startNextHand(userData->userId);
                     if (!success)
                       errorMsg = "Failed (not host or game not started)";
                   } else if (action == "game_action") {
                     std::string command = j.value("command", "");
                     int amount = j.value("amount", 0);
                     if (command.empty()) {
                       errorMsg = "Missing 'command' field";
                     } else {
                       success = lobby.getGame().playerAction(userData->userId,
                                                              command, amount);
                       if (!success)
                         errorMsg = "Invalid action (not your turn?)";
                     }
                   } else if (action == "muck_show") {
                     bool show = j.value("show", false);
                     success = lobby.getGame().playerMuckOrShow(
                         userData->userId, show);
                     if (!success)
                       errorMsg = "Cannot muck/show right now";
                   } else if (action == "rebuy") {
                     int amount = j.value("amount", 0);
                     if (amount <= 0) {
                       errorMsg = "Invalid rebuy amount";
                     } else {
                       success = lobby.rebuy(userData->userId, amount);
                       if (!success)
                         errorMsg = "Rebuy failed (hand in progress?)";
                     }
                   } else if (action == "chat") {
                     if (userData->userId.empty()) {
                       errorMsg = "Must join first";
                     } else {
                       std::string messageText = j.value("message", "");
                       success =
                           lobby.addChatMessage(userData->userId, messageText);
                       if (!success)
                         errorMsg = "Invalid chat message";
                     }
                   } else if (action == "update_config") {
                     poker::LobbyConfig newConfig;
                     newConfig.maxSeats = j.value("maxSeats", 6);
                     newConfig.startingStack = j.value("startingStack", 1000);
                     newConfig.smallBlind = j.value("smallBlind", 5);
                     newConfig.bigBlind = j.value("bigBlind", 10);
                     newConfig.actionTimeout = j.value("actionTimeout", 30);
                     newConfig.godMode = j.value("godMode", true);
                     newConfig.roomCode =
                         j.value("roomCode", lobby.getLobbyConfig().roomCode);
                     success = lobby.updateConfig(userData->userId, newConfig);
                     if (!success)
                       errorMsg = "Failed (not host or game in progress)";
                   } else if (action == "end_game") {
                     success = lobby.endGame(userData->userId);
                     if (!success)
                       errorMsg = "Failed (not host)";
                   } else if (action == "kick_player") {
                     std::string targetId = j.value("targetId", "");
                     if (targetId.empty()) {
                       errorMsg = "Missing 'targetId' field";
                     } else {
                       success = lobby.kickPlayer(userData->userId, targetId);
                       if (success) {
                         // Also disconnect the kicked player's socket
                         auto it = connectedSockets.find(targetId);
                         if (it != connectedSockets.end()) {
                           json kickedResp;
                           kickedResp["type"] = "kicked";
                           kickedResp["message"] = "You were kicked by the host.";
                           it->second->send(kickedResp.dump(), uWS::OpCode::TEXT);
                           it->second->close();
                         }
                       } else {
                         errorMsg = "Failed (not host or invalid target)";
                       }
                     }
                   } else if (action == "leave") {
                     success = lobby.leave(userData->userId);
                     if (success) {
                       connectedSockets.erase(userData->userId);
                       userData->userId.clear();
                     }
                   } else {
                     errorMsg = "Unknown action: '" + action + "'";
                   }

                   // Response
                   if (success) {
                     broadcastToAll();
                   } else if (!errorMsg.empty()) {
                     json err;
                     err["type"] = "error";
                     err["message"] = errorMsg;
                     ws->send(err.dump(), opCode);
                   }

                 } catch (const json::exception &e) {
                   json err;
                   err["type"] = "error";
                   err["message"] = std::string("Invalid JSON: ") + e.what();
                   ws->send(err.dump(), uWS::OpCode::TEXT);
                 } catch (const std::exception &e) {
                   std::cerr << "Error: " << e.what() << std::endl;
                 }
               },

           .close =
               [](WebSocket *ws, int /*code*/, std::string_view /*message*/) {
                 PerSocketData *userData = ws->getUserData();
                 if (!userData->userId.empty()) {
                   std::cout << "Client disconnected: " << userData->userId
                             << std::endl;
                   lobby.disconnectPlayer(userData->userId);
                   connectedSockets.erase(userData->userId);
                   broadcastToAll();
                 }
               }})
      .listen(9001,
              [](auto *listen_socket) {
                if (listen_socket) {
                  std::cout << "Listening on port 9001" << std::endl;
                }
              })
      .run();

  return 0;
}
