#include "App.h"
#include "Lobby.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

namespace {

constexpr int kProtocolVersion = 1;

constexpr const char *kErrInvalidAction = "INVALID_ACTION";
constexpr const char *kErrUnauthorized = "UNAUTHORIZED";
constexpr const char *kErrStaleConnection = "STALE_CONNECTION";
constexpr const char *kErrBadPayload = "BAD_PAYLOAD";
constexpr const char *kErrInternalError = "INTERNAL_ERROR";

} // namespace

struct PerSocketData {
  std::string userId;
};

using WebSocket = uWS::WebSocket<false, true, PerSocketData>;

// Global state
poker::Lobby lobby;
std::unordered_map<std::string, WebSocket *> connectedSockets;
std::unordered_map<WebSocket *, std::string> socketOwners;
json spectatorEquityCache = json::object();
bool hasSpectatorEquityCache = false;

bool isCurrentSocketForUser(WebSocket *ws, const std::string &userId) {
  auto it = connectedSockets.find(userId);
  return it != connectedSockets.end() && it->second == ws;
}

void sendJson(WebSocket *ws, const json &message) {
  ws->send(message.dump(), uWS::OpCode::TEXT);
}

json makeEventEnvelope(const std::string &eventName, const json &data) {
  json envelope = json::object();
  envelope["v"] = kProtocolVersion;
  envelope["kind"] = "event";
  envelope["event"] = eventName;
  envelope["data"] = data;
  return envelope;
}

void unbindSocket(WebSocket *ws) {
  auto ownerIt = socketOwners.find(ws);
  if (ownerIt == socketOwners.end())
    return;

  const std::string userId = ownerIt->second;
  socketOwners.erase(ownerIt);

  auto connIt = connectedSockets.find(userId);
  if (connIt != connectedSockets.end() && connIt->second == ws) {
    connectedSockets.erase(connIt);
  }

  PerSocketData *userData = ws->getUserData();
  if (userData->userId == userId) {
    userData->userId.clear();
  }
}

void unbindUser(const std::string &userId) {
  auto connIt = connectedSockets.find(userId);
  if (connIt == connectedSockets.end())
    return;

  WebSocket *ws = connIt->second;
  connectedSockets.erase(connIt);

  auto ownerIt = socketOwners.find(ws);
  if (ownerIt != socketOwners.end() && ownerIt->second == userId) {
    socketOwners.erase(ownerIt);
  }

  PerSocketData *userData = ws->getUserData();
  if (userData->userId == userId) {
    userData->userId.clear();
  }
}

void bindSocketToUser(WebSocket *ws, const std::string &userId) {
  // Detach any previous user binding on this socket.
  unbindSocket(ws);

  // Replace older socket for the same user.
  auto existing = connectedSockets.find(userId);
  if (existing != connectedSockets.end() && existing->second != ws) {
    WebSocket *oldWs = existing->second;
    connectedSockets.erase(existing);

    auto oldOwner = socketOwners.find(oldWs);
    if (oldOwner != socketOwners.end()) {
      socketOwners.erase(oldOwner);
    }

    PerSocketData *oldData = oldWs->getUserData();
    if (oldData->userId == userId) {
      oldData->userId.clear();
    }

    oldWs->close();
  }

  connectedSockets[userId] = ws;
  socketOwners[ws] = userId;
  ws->getUserData()->userId = userId;
}

// Send personalised state to every connected client
void broadcastToAll(bool includeEquities = true) {
  json cachedEquities;
  json *equitiesPtr = nullptr;

  // Pre-calculate equities if God Mode is active
  // avoids running Monte Carlo sim for every spectator
  if (lobby.getLobbyConfig().godMode) {
    if (includeEquities) {
      cachedEquities = lobby.computeEquities();
      spectatorEquityCache = cachedEquities;
      hasSpectatorEquityCache = true;
      equitiesPtr = &cachedEquities;
    } else if (hasSpectatorEquityCache) {
      equitiesPtr = &spectatorEquityCache;
    }
  } else {
    hasSpectatorEquityCache = false;
    spectatorEquityCache = json::object();
  }

  std::string spectatorPayload;
  bool spectatorPayloadReady = false;

  for (auto &[userId, ws] : connectedSockets) {
    // Spectators see identical state: we can serialize once and reuse the
    // string.
    if (lobby.isSpectator(userId)) {
      if (!spectatorPayloadReady) {
        json msg = makeEventEnvelope("game_state",
                                     lobby.toJsonForViewer("", includeEquities,
                                                           equitiesPtr));
        spectatorPayload = msg.dump();
        spectatorPayloadReady = true;
      }
      ws->send(spectatorPayload, uWS::OpCode::TEXT);
    } else {
      // Active players need unique views
      json msg = makeEventEnvelope("game_state",
                                   lobby.toJsonForViewer(userId, includeEquities,
                                                         equitiesPtr));
      sendJson(ws, msg);
    }
  }
}

namespace {

struct ActionResult {
  bool success = false;
  std::string errorCode;
  std::string errorMsg;
  bool includeEquitiesInBroadcast = false;
  json data = json::object();
};

struct ActionContext {
  WebSocket *ws = nullptr;
  PerSocketData *userData = nullptr;
  const json &data;
};

using ActionHandler = ActionResult (*)(const ActionContext &);

ActionResult makeSuccess() {
  ActionResult result;
  result.success = true;
  return result;
}

ActionResult makeError(const std::string &code, const std::string &message) {
  ActionResult result;
  result.success = false;
  result.errorCode = code;
  result.errorMsg = message;
  return result;
}

bool readRequiredString(const json &data, const char *key, std::string &out,
                        ActionResult &error, bool allowEmpty = false) {
  auto it = data.find(key);
  if (it == data.end() || !it->is_string()) {
    error = makeError(kErrBadPayload,
                      std::string("Missing or invalid '") + key + "' field");
    return false;
  }

  out = it->get<std::string>();
  if (!allowEmpty && out.empty()) {
    error = makeError(kErrBadPayload,
                      std::string("Field '") + key + "' must not be empty");
    return false;
  }

  return true;
}

bool readOptionalString(const json &data, const char *key, std::string &out,
                        ActionResult &error) {
  auto it = data.find(key);
  if (it == data.end())
    return true;
  if (!it->is_string()) {
    error = makeError(kErrBadPayload,
                      std::string("Field '") + key + "' must be a string");
    return false;
  }
  out = it->get<std::string>();
  return true;
}

bool readRequiredInt(const json &data, const char *key, int &out,
                     ActionResult &error) {
  auto it = data.find(key);
  if (it == data.end() || !it->is_number_integer()) {
    error = makeError(kErrBadPayload,
                      std::string("Missing or invalid '") + key + "' field");
    return false;
  }
  out = it->get<int>();
  return true;
}

bool readOptionalInt(const json &data, const char *key, int &out,
                     ActionResult &error) {
  auto it = data.find(key);
  if (it == data.end())
    return true;
  if (!it->is_number_integer()) {
    error = makeError(kErrBadPayload,
                      std::string("Field '") + key + "' must be an integer");
    return false;
  }
  out = it->get<int>();
  return true;
}

bool readRequiredBool(const json &data, const char *key, bool &out,
                      ActionResult &error) {
  auto it = data.find(key);
  if (it == data.end() || !it->is_boolean()) {
    error = makeError(kErrBadPayload,
                      std::string("Missing or invalid '") + key + "' field");
    return false;
  }
  out = it->get<bool>();
  return true;
}

bool readOptionalBool(const json &data, const char *key, bool &out,
                      ActionResult &error) {
  auto it = data.find(key);
  if (it == data.end())
    return true;
  if (!it->is_boolean()) {
    error = makeError(kErrBadPayload,
                      std::string("Field '") + key + "' must be a boolean");
    return false;
  }
  out = it->get<bool>();
  return true;
}

ActionResult handleJoin(const ActionContext &ctx) {
  std::string name;
  ActionResult error;
  if (!readRequiredString(ctx.data, "name", name, error)) {
    return error;
  }

  std::string id;
  if (!readOptionalString(ctx.data, "id", id, error)) {
    return error;
  }

  if (id.empty()) {
    std::random_device rd;
    id = "user_" + std::to_string(rd() % 900000 + 100000);
  }

  ActionResult result = makeSuccess();

  // Try reconnect first, then fresh join
  if (lobby.reconnectPlayer(id)) {
    bindSocketToUser(ctx.ws, id);
    std::cout << "Player reconnected: " << id << std::endl;
  } else if (lobby.join(id, name)) {
    bindSocketToUser(ctx.ws, id);
  } else {
    return makeError(kErrInvalidAction, "Could not join");
  }

  result.data["userId"] = id;
  return result;
}

ActionResult handleSit(const ActionContext &ctx) {
  ActionResult error;
  int seat = -1;
  int buyIn = 0;

  if (!readRequiredInt(ctx.data, "seatIndex", seat, error)) {
    return error;
  }
  if (!readOptionalInt(ctx.data, "buyIn", buyIn, error)) {
    return error;
  }

  if (lobby.sitPlayer(ctx.userData->userId, seat, buyIn) == -1) {
    return makeError(kErrInvalidAction,
                     "Could not sit (seat taken or invalid buy-in)");
  }

  return makeSuccess();
}

ActionResult handleStand(const ActionContext &ctx) {
  if (!lobby.standPlayer(ctx.userData->userId)) {
    return makeError(kErrInvalidAction, "Could not stand");
  }

  ActionResult result = makeSuccess();
  result.includeEquitiesInBroadcast = true;
  return result;
}

ActionResult handleStartGame(const ActionContext &ctx) {
  if (!lobby.startGame(ctx.userData->userId)) {
    return makeError(kErrInvalidAction, "Failed (not host or not enough players)");
  }

  ActionResult result = makeSuccess();
  result.includeEquitiesInBroadcast = true;
  return result;
}

ActionResult handleStartNextHand(const ActionContext &ctx) {
  if (!lobby.startNextHand(ctx.userData->userId)) {
    return makeError(kErrInvalidAction, "Failed (not host or game not started)");
  }

  ActionResult result = makeSuccess();
  result.includeEquitiesInBroadcast = true;
  return result;
}

ActionResult handleGameAction(const ActionContext &ctx) {
  ActionResult error;
  std::string command;
  int amount = 0;

  if (!readRequiredString(ctx.data, "command", command, error)) {
    return error;
  }
  if (!readOptionalInt(ctx.data, "amount", amount, error)) {
    return error;
  }

  if (!lobby.handleGameAction(ctx.userData->userId, command, amount)) {
    return makeError(kErrInvalidAction, "Invalid action (not your turn?)");
  }

  ActionResult result = makeSuccess();
  result.includeEquitiesInBroadcast = true;
  return result;
}

ActionResult handleMuckShow(const ActionContext &ctx) {
  ActionResult error;
  bool show = false;
  if (!readRequiredBool(ctx.data, "show", show, error)) {
    return error;
  }

  if (!lobby.handleMuckOrShow(ctx.userData->userId, show)) {
    return makeError(kErrInvalidAction, "Cannot muck/show right now");
  }

  ActionResult result = makeSuccess();
  result.includeEquitiesInBroadcast = true;
  return result;
}

ActionResult handleRebuy(const ActionContext &ctx) {
  ActionResult error;
  int amount = 0;
  if (!readRequiredInt(ctx.data, "amount", amount, error)) {
    return error;
  }

  if (amount <= 0) {
    return makeError(kErrInvalidAction, "Invalid rebuy amount");
  }

  if (!lobby.rebuy(ctx.userData->userId, amount)) {
    return makeError(kErrInvalidAction, "Rebuy failed (hand in progress?)");
  }

  return makeSuccess();
}

ActionResult handleChat(const ActionContext &ctx) {
  ActionResult error;
  std::string messageText;
  if (!readRequiredString(ctx.data, "message", messageText, error, true)) {
    return error;
  }

  if (!lobby.addChatMessage(ctx.userData->userId, messageText)) {
    return makeError(kErrInvalidAction, "Invalid chat message");
  }

  return makeSuccess();
}

ActionResult handleUpdateConfig(const ActionContext &ctx) {
  ActionResult error;
  poker::LobbyConfig newConfig = lobby.getLobbyConfig();

  if (!readOptionalInt(ctx.data, "maxSeats", newConfig.maxSeats, error)) {
    return error;
  }
  if (!readOptionalInt(ctx.data, "startingStack", newConfig.startingStack,
                       error)) {
    return error;
  }
  if (!readOptionalInt(ctx.data, "smallBlind", newConfig.smallBlind, error)) {
    return error;
  }
  if (!readOptionalInt(ctx.data, "bigBlind", newConfig.bigBlind, error)) {
    return error;
  }
  if (!readOptionalInt(ctx.data, "actionTimeout", newConfig.actionTimeout,
                       error)) {
    return error;
  }
  if (!readOptionalBool(ctx.data, "godMode", newConfig.godMode, error)) {
    return error;
  }
  if (!readOptionalString(ctx.data, "roomCode", newConfig.roomCode, error)) {
    return error;
  }

  if (!lobby.updateConfig(ctx.userData->userId, newConfig)) {
    return makeError(kErrInvalidAction, "Failed (not host or game in progress)");
  }

  return makeSuccess();
}

ActionResult handleEndGame(const ActionContext &ctx) {
  if (!lobby.endGame(ctx.userData->userId)) {
    return makeError(kErrInvalidAction, "Failed (not host)");
  }

  return makeSuccess();
}

ActionResult handleKickPlayer(const ActionContext &ctx) {
  ActionResult error;
  std::string targetId;

  if (!readRequiredString(ctx.data, "targetId", targetId, error)) {
    return error;
  }

  if (!lobby.kickPlayer(ctx.userData->userId, targetId)) {
    return makeError(kErrInvalidAction,
                     "Failed (not host or invalid target)");
  }

  // Also disconnect the kicked player's socket.
  auto it = connectedSockets.find(targetId);
  if (it != connectedSockets.end()) {
    WebSocket *targetWs = it->second;
    json kickedData = json::object();
    kickedData["message"] = "You were kicked by the host.";
    sendJson(targetWs, makeEventEnvelope("kicked", kickedData));
    unbindUser(targetId);
    targetWs->close();
  }

  ActionResult result = makeSuccess();
  result.includeEquitiesInBroadcast = true;
  return result;
}

ActionResult handleLeave(const ActionContext &ctx) {
  if (!lobby.leave(ctx.userData->userId)) {
    return makeError(kErrInvalidAction, "Could not leave");
  }

  unbindUser(ctx.userData->userId);
  ctx.userData->userId.clear();

  ActionResult result = makeSuccess();
  result.includeEquitiesInBroadcast = true;
  return result;
}

const std::unordered_map<std::string, ActionHandler> &getActionHandlers() {
  static const std::unordered_map<std::string, ActionHandler> handlers = {
      {"join", handleJoin},
      {"sit", handleSit},
      {"stand", handleStand},
      {"start_game", handleStartGame},
      {"start_next_hand", handleStartNextHand},
      {"game_action", handleGameAction},
      {"muck_show", handleMuckShow},
      {"rebuy", handleRebuy},
      {"chat", handleChat},
      {"update_config", handleUpdateConfig},
      {"end_game", handleEndGame},
      {"kick_player", handleKickPlayer},
      {"leave", handleLeave},
  };
  return handlers;
}

bool parseRequestEnvelope(const json &raw, std::string &requestId,
                          std::string &action, json &data,
                          ActionResult &error) {
  if (!raw.is_object()) {
    error = makeError(kErrBadPayload, "Payload must be a JSON object");
    return false;
  }

  auto idIt = raw.find("id");
  if (idIt != raw.end() && idIt->is_string()) {
    requestId = idIt->get<std::string>();
  }

  if (idIt == raw.end() || !idIt->is_string() || requestId.empty()) {
    error = makeError(kErrBadPayload,
                      "Missing or invalid 'id' field (must be non-empty string)");
    return false;
  }

  auto versionIt = raw.find("v");
  if (versionIt == raw.end() || !versionIt->is_number_integer() ||
      versionIt->get<int>() != kProtocolVersion) {
    error = makeError(kErrBadPayload, "Missing or unsupported protocol version");
    return false;
  }

  auto kindIt = raw.find("kind");
  if (kindIt == raw.end() || !kindIt->is_string() ||
      kindIt->get<std::string>() != "request") {
    error = makeError(kErrBadPayload,
                      "Missing or invalid 'kind' field (expected 'request')");
    return false;
  }

  auto actionIt = raw.find("action");
  if (actionIt == raw.end() || !actionIt->is_string() ||
      actionIt->get<std::string>().empty()) {
    error = makeError(kErrBadPayload,
                      "Missing or invalid 'action' field");
    return false;
  }
  action = actionIt->get<std::string>();

  auto dataIt = raw.find("data");
  if (dataIt == raw.end() || !dataIt->is_object()) {
    error = makeError(kErrBadPayload,
                      "Missing or invalid 'data' field (must be object)");
    return false;
  }
  data = *dataIt;

  return true;
}

json makeResponseEnvelope(const std::string &requestId,
                          const ActionResult &result) {
  json response = json::object();
  response["v"] = kProtocolVersion;
  response["kind"] = "response";
  response["id"] = requestId.empty() ? json(nullptr) : json(requestId);
  response["ok"] = result.success;

  if (result.success) {
    response["data"] = result.data;
  } else {
    json error = json::object();
    error["code"] =
        result.errorCode.empty() ? std::string(kErrInternalError) : result.errorCode;
    error["message"] = result.errorMsg.empty() ? "Request failed" : result.errorMsg;
    response["error"] = error;
  }

  return response;
}

} // namespace

int main() {
  std::cout << "Starting Game Server on port 9001..." << std::endl;

  uWS::App()
      .ws<PerSocketData>(
          "/*",
          {.compression = uWS::SHARED_COMPRESSOR,
           .maxPayloadLength = 16 * 1024,
           .idleTimeout = 120,

           .open =
               [](WebSocket * /*ws*/) {
                 std::cout << "Client connected!" << std::endl;
               },

           .message =
               [](WebSocket *ws, std::string_view message,
                  uWS::OpCode /*opCode*/) {
                 try {
                   auto raw = json::parse(message);
                   PerSocketData *userData = ws->getUserData();

                   std::cout << "Received: " << message << std::endl;

                   std::string requestId;
                   std::string action;
                   json data = json::object();
                   ActionResult result;

                   if (!parseRequestEnvelope(raw, requestId, action, data,
                                             result)) {
                     sendJson(ws, makeResponseEnvelope(requestId, result));
                     return;
                   }

                   if (action != "join" && userData->userId.empty()) {
                     result = makeError(kErrUnauthorized, "Must join first");
                   } else if (action != "join" &&
                              !isCurrentSocketForUser(ws, userData->userId)) {
                     result = makeError(kErrStaleConnection,
                                        "Stale connection. Please reconnect.");
                   } else {
                     const auto &handlers = getActionHandlers();
                     auto handlerIt = handlers.find(action);
                     if (handlerIt == handlers.end()) {
                       result = makeError(kErrInvalidAction,
                                          "Unknown action: '" + action + "'");
                     } else {
                       result =
                           handlerIt->second(ActionContext{ws, userData, data});
                     }
                   }

                   sendJson(ws, makeResponseEnvelope(requestId, result));

                   if (result.success) {
                     broadcastToAll(result.includeEquitiesInBroadcast);
                   }

                 } catch (const json::exception &) {
                   ActionResult parseError =
                       makeError(kErrBadPayload, "Invalid JSON payload");
                   sendJson(ws, makeResponseEnvelope("", parseError));
                 } catch (const std::exception &e) {
                   std::cerr << "Error: " << e.what() << std::endl;
                   ActionResult internalError =
                       makeError(kErrInternalError, "Internal server error");
                   sendJson(ws, makeResponseEnvelope("", internalError));
                 }
               },

           .close =
               [](WebSocket *ws, int /*code*/, std::string_view /*message*/) {
                 auto ownerIt = socketOwners.find(ws);
                 if (ownerIt == socketOwners.end())
                   return;

                 std::string userId = ownerIt->second;
                 socketOwners.erase(ownerIt);

                 auto connIt = connectedSockets.find(userId);
                 if (connIt == connectedSockets.end() || connIt->second != ws) {
                   return;
                 }

                 connectedSockets.erase(connIt);
                 PerSocketData *userData = ws->getUserData();
                 if (userData->userId == userId) {
                   userData->userId.clear();
                 }

                 std::cout << "Client disconnected: " << userId << std::endl;
                 lobby.disconnectPlayer(userId);
                 broadcastToAll(true);
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
