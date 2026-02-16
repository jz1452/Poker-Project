#include "../src/server/Lobby.h"
#include <cassert>
#include <iostream>

using namespace poker;
using namespace std;

void log(string msg) { cout << "[TestLobby] " << msg << endl; }

void testHostAssignment() {
  log("Testing Host Assignment...");
  Lobby lobby;

  // First joiner = Host
  assert(lobby.join("p1", "Alice") == true);
  assert(lobby.isUserHost("p1") == true);

  // Second joiner = Not Host
  assert(lobby.join("p2", "Bob") == true);
  assert(lobby.isUserHost("p2") == false);

  // Host leaves -> Host role reassigned to p2
  assert(lobby.leave("p1") == true);
  assert(lobby.isUserHost("p2") == true);

  // Last user leaves -> Room empty (hostId cleared)
  assert(lobby.leave("p2") == true);
  assert(lobby.getHostId() == "");

  log("Passed.");
}

void testAccessControl() {
  log("Testing Access Control (Host Privileges)...");
  Lobby lobby;
  lobby.join("host", "Host");
  lobby.join("guest", "Guest");

  // Only host can start game
  assert(lobby.startGame("guest") == false);

  // Need 2 players seated to start
  lobby.sitPlayer("host", 0, 1000);
  lobby.sitPlayer("guest", 1, 1000);

  assert(lobby.startGame("host") == true);
  assert(lobby.isGameInProgress() == true);

  // Only host can end game
  assert(lobby.endGame("guest") == false);
  assert(lobby.endGame("host") == true);
  assert(lobby.isGameInProgress() == false);

  log("Passed.");
}

void testKickPlayer() {
  log("Testing Kick Player...");
  Lobby lobby;
  lobby.join("host", "Host");
  lobby.join("troll", "Troll");

  // Guest cannot kick
  assert(lobby.kickPlayer("troll", "host") == false);

  // Host can kick
  assert(lobby.kickPlayer("host", "troll") == true);

  // Verify troll is gone
  bool found = false;
  for (const auto &u : lobby.getUsers()) {
    if (u.id == "troll")
      found = true;
  }
  assert(!found);

  log("Passed.");
}

void testConfigUpdates() {
  log("Testing Config Updates...");
  Lobby lobby;
  lobby.join("host", "Host");

  LobbyConfig newConf;
  newConf.smallBlind = 100;
  newConf.bigBlind = 200;

  // Guest cannot update
  assert(lobby.updateConfig("guest", newConf) == false);

  // Host can update
  assert(lobby.updateConfig("host", newConf) == true);
  assert(lobby.getLobbyConfig().bigBlind == 200);

  assert(lobby.getGame().getMinRaise() == 200 ||
         lobby.getGame().getCurrentBet() == 0);

  log("Passed.");
}

void testRebuy() {
  log("Testing Rebuy...");
  Lobby lobby;
  lobby.join("p1", "Alice");
  lobby.sitPlayer("p1", 0, 1000);

  // Can rebuy between hands
  assert(lobby.rebuy("p1", 500) == true);

  // Cannot rebuy mid-game
  lobby.join("p2", "Bob");
  lobby.sitPlayer("p2", 1, 1000);
  lobby.startGame("p1");

  assert(lobby.rebuy("p1", 500) == false);

  lobby.endGame("p1");
  assert(lobby.rebuy("p1", 500) == true);

  log("Passed.");
}

void testFoldWinBlocking() {
  log("Testing StartNextHand Blocking (Fold Win)...");
  Game::Config conf;
  conf.smallBlind = 10;
  conf.bigBlind = 20;
  conf.startingStack = 1000;

  Lobby lobby(conf);
  Game &g = lobby.getGame();

  lobby.join("p1", "Alice");
  lobby.join("p2", "Bob");

  lobby.sitPlayer("p1", 0, 1000);
  lobby.sitPlayer("p2", 1, 1000);

  // p1 is host
  assert(lobby.startGame("p1") == true);

  // p1 folds -> p2 wins
  assert(g.playerAction("p1", "fold", 0) == true);

  // Hand is over (fold winner set), but p2 hasn't decided muck/show yet.
  // Host tries to start next hand -> SHOULD FAIL
  assert(lobby.startNextHand("p1") == false);

  // p2 decides to muck
  assert(g.playerMuckOrShow("p2", false) == true);

  // Now stage should be Idle (because p2 decided)
  assert(g.getStage() == GameStage::Idle);

  // Host tries to start next hand -> SHOULD SUCCEED
  assert(lobby.startNextHand("p1") == true);

  log("Passed.");
}

int main() {
  testHostAssignment();
  testAccessControl();
  testKickPlayer();
  testConfigUpdates();
  testRebuy();
  testFoldWinBlocking();
  cout << "ALL LOBBY TESTS PASSED!" << endl;
  return 0;
}
