#include "../src/server/Lobby.h"
#include <cassert>
#include <iostream>

using namespace poker;
using namespace std;

void log(string msg) { cout << "[TEST] " << msg << endl; }

void testSitStand() {
  log("Running Sit/Stand Test...");
  Lobby lobby;
  Game &g = lobby.getGame();

  // Must join before sitting
  assert(lobby.join("p1", "Player 1") == true);
  assert(lobby.join("p2", "Player 2") == true);
  assert(lobby.join("p3", "Player 3") == true);

  assert(lobby.sitPlayer("p1", 0, 1000) == 0);
  assert(lobby.sitPlayer("p2", 1, 1000) == 1);
  assert(lobby.sitPlayer("p3", 0, 1000) == -1); // Seat taken

  // Can't sit without joining
  assert(lobby.sitPlayer("unknown", 2, 1000) == -1);

  assert(g.getSeats()[0].name == "Player 1");
  assert(lobby.standPlayer("p1") == true);
  assert(g.getSeats()[0].status == PlayerStatus::SittingOut);
  log("Sit/Stand Test Passed.");
}

void testBasicHand() {
  log("Running Basic Hand Test...");
  Game::Config conf;
  conf.smallBlind = 10;
  conf.bigBlind = 20;
  conf.startingStack = 1000;

  Lobby lobby(conf);
  Game &g = lobby.getGame();

  lobby.join("p1", "Alice");
  lobby.join("p2", "Bob");
  lobby.join("p3", "Charlie");

  lobby.sitPlayer("p1", 0, 1000);
  lobby.sitPlayer("p2", 1, 1000);
  lobby.sitPlayer("p3", 2, 1000);

  // p1 is host (first to join)
  assert(lobby.startGame("p1") == true);

  // Check Blinds
  int countSB = 0;
  int countBB = 0;
  int countBtn = 0;

  for (const auto &p : g.getSeats()) {
    if (p.chips == 990)
      countSB++;
    else if (p.chips == 980)
      countBB++;
    else if (p.chips == 1000)
      countBtn++;
  }

  assert(g.getPot() == 30);
  assert(countSB == 1);
  assert(countBB == 1);
  assert(countBtn == 1);

  log("Basic Hand Start Passed.");
}

void testSidePots() {
  log("Running rigorous Side Pot Verification...");
  Game::Config conf;
  conf.smallBlind = 10;
  conf.bigBlind = 20;
  conf.startingStack = 1000;

  Lobby lobby(conf);
  Game &g = lobby.getGame();

  lobby.join("pA", "Player A");
  lobby.join("pB", "Player B");
  lobby.join("pC", "Player C");

  // Buy in with different amounts to test side pots
  lobby.sitPlayer("pA", 0, 100);  // Short stack
  lobby.sitPlayer("pB", 1, 300);  // Medium stack
  lobby.sitPlayer("pC", 2, 1000); // Big stack

  lobby.setButtonPos(-1);

  // pA is host (first to join)
  assert(lobby.startGame("pA") == true);

  assert(g.playerAction("pA", "allin", 0) == true);
  assert(g.playerAction("pB", "allin", 0) == true);
  assert(g.playerAction("pC", "call", 0) == true);

  assert(g.getPot() == 0);

  int totalChips = 0;
  for (const auto &p : g.getSeats())
    totalChips += p.chips;
  assert(totalChips == 1400);

  log("Side Pot Logic Conservation of Mass Passed.");

  assert(g.getIsAllInShowdown() == true);
  assert(g.getStage() == GameStage::Showdown);

  const auto &results = g.getShowdownResults();
  assert(results.size() == 3);

  for (const auto &r : results) {
    assert(r.mustShow == true);
    assert(g.getSeats()[r.seatIndex].showCards == true);
  }

  assert(g.playerMuckOrShow("pA", false) == false);
  assert(g.playerMuckOrShow("pB", false) == false);
  assert(g.playerMuckOrShow("pC", false) == false);

  log("All-In Showdown Show Logic Passed.");
}

void testFoldWin() {
  log("Running Fold Win Test (no showdown)...");
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

  lobby.setButtonPos(-1);
  assert(lobby.startGame("p1") == true);

  assert(g.playerAction("p1", "fold", 0) == true);

  assert(g.getPot() == 0);
  assert(g.getSeats()[0].showCards == false);
  assert(g.getSeats()[1].showCards == false);

  assert(g.getShowdownResults().empty());

  assert(g.playerMuckOrShow("p2", true) == true);
  assert(g.getSeats()[1].showCards == true);

  assert(g.playerMuckOrShow("p2", false) == true);
  assert(g.getSeats()[1].showCards == false);

  assert(g.playerMuckOrShow("p1", true) == false);

  log("Fold Win Test Passed.");
}

void testMuckOrShow() {
  log("Running Muck/Show Test...");
  Game::Config conf;
  conf.smallBlind = 10;
  conf.bigBlind = 20;
  conf.startingStack = 1000;

  Lobby lobby(conf);
  Game &g = lobby.getGame();

  lobby.join("p1", "Alice");
  lobby.join("p2", "Bob");
  lobby.join("p3", "Charlie");

  lobby.sitPlayer("p1", 0, 1000);
  lobby.sitPlayer("p2", 1, 1000);
  lobby.sitPlayer("p3", 2, 1000);

  lobby.setButtonPos(-1);
  assert(lobby.startGame("p1") == true);

  assert(g.playerAction("p1", "call", 0) == true);
  assert(g.playerAction("p2", "call", 0) == true);
  assert(g.playerAction("p3", "check", 0) == true);

  for (int street = 0; street < 3; street++) {
    assert(g.playerAction("p2", "check", 0) == true);
    assert(g.playerAction("p3", "check", 0) == true);
    assert(g.playerAction("p1", "check", 0) == true);
  }

  assert(g.getStage() == GameStage::Showdown);
  assert(g.getPot() == 0);
  assert(g.getIsAllInShowdown() == false);

  const auto &results = g.getShowdownResults();
  assert(results.size() == 3);

  int winnerSeat = -1;
  int loserSeat = -1;
  for (const auto &r : results) {
    if (r.mustShow) {
      winnerSeat = r.seatIndex;
    } else if (loserSeat == -1) {
      loserSeat = r.seatIndex;
    }
  }

  assert(winnerSeat >= 0);
  assert(g.getSeats()[winnerSeat].showCards == true);

  if (loserSeat >= 0) {
    assert(g.getSeats()[loserSeat].showCards == false);

    std::string loserId = g.getSeats()[loserSeat].id;
    assert(g.playerMuckOrShow(loserId, true) == true);
    assert(g.getSeats()[loserSeat].showCards == true);

    assert(g.playerMuckOrShow(loserId, false) == true);
    assert(g.getSeats()[loserSeat].showCards == false);

    std::string winnerId = g.getSeats()[winnerSeat].id;
    assert(g.playerMuckOrShow(winnerId, false) == false);
    assert(g.getSeats()[winnerSeat].showCards == true);
  }

  log("Muck/Show Test Passed.");
}

int main() {
  testSitStand();
  testBasicHand();
  testSidePots();
  testFoldWin();
  testMuckOrShow();
  cout << "ALL TESTS PASSED!" << endl;
  return 0;
}
