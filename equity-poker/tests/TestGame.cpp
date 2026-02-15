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

  assert(lobby.sitPlayer("p1", "Player 1", 0) == 0);
  assert(lobby.sitPlayer("p2", "Player 2", 1) == 1);
  assert(lobby.sitPlayer("p3", "Player 3", 0) == -1); // Seat taken

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

  lobby.sitPlayer("p1", "Alice", 0);
  lobby.sitPlayer("p2", "Bob", 1);
  lobby.sitPlayer("p3", "Charlie", 2);

  g.startHand();

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

  // Sit 3 players: A(Seat 0), B(Seat 1), C(Seat 2)
  lobby.sitPlayer("pA", "Player A", 0);
  lobby.sitPlayer("pB", "Player B", 1);
  lobby.sitPlayer("pC", "Player C", 2);

  // Force Button to Seat 0 (A). startHand() advances by 1, so set to -1.
  // Therefore Button=A(0), SB=B(1), BB=C(2)
  lobby.setButtonPos(-1);

  // set stacks
  // A: 100 (Short), B: 300 (Medium), C: 1000 (Big)
  lobby.setPlayerStack(0, 100);
  lobby.setPlayerStack(1, 300);
  lobby.setPlayerStack(2, 1000);

  g.startHand();
  // After blinds: B posts SB(10) --> 290, C posts BB(20) --> 980, A has 100
  // Pot = 30, currentBet = 20

  assert(g.playerAction("pA", "allin", 0) == true);
  assert(g.playerAction("pB", "allin", 0) == true);
  assert(g.playerAction("pC", "call", 0) == true);

  // pot was distributed
  assert(g.getPot() == 0);

  // conservation of chips
  int totalChips = 0;
  for (const auto &p : g.getSeats())
    totalChips += p.chips;
  assert(totalChips == 1400);

  log("Side Pot Logic Conservation of Mass Passed.");

  // all-in showdown: all players must show
  assert(g.getIsAllInShowdown() == true);
  assert(g.getStage() == GameStage::Showdown);

  const auto &results = g.getShowdownResults();
  assert(results.size() == 3);

  for (const auto &r : results) {
    assert(r.mustShow == true);
    assert(g.getSeats()[r.seatIndex].showCards == true);
  }

  // Winners can't muck (already mustShow)
  // Losers can't muck either (all-in showdown)
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

  lobby.sitPlayer("p1", "Alice", 0);
  lobby.sitPlayer("p2", "Bob", 1);
  lobby.setButtonPos(-1);
  g.startHand();

  // Button (Alice) folds. Bob wins without showdown.
  assert(g.playerAction("p1", "fold", 0) == true);

  // Not a showdown — winner doesn't have to show
  assert(g.getPot() == 0);

  // Neither player should show cards by default (no showdown)
  assert(g.getSeats()[0].showCards == false); // Alice folded
  assert(g.getSeats()[1].showCards == false); // Bob won via fold, starts hidden

  // showdownResults should be empty (no showdown)
  assert(g.getShowdownResults().empty());

  // But Bob (fold-winner) can CHOOSE to show!
  assert(g.playerMuckOrShow("p2", true) == true);
  assert(g.getSeats()[1].showCards == true);

  // Bob can also re-muck
  assert(g.playerMuckOrShow("p2", false) == true);
  assert(g.getSeats()[1].showCards == false);

  // Alice (who folded) cannot show
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

  lobby.sitPlayer("p1", "Alice", 0);
  lobby.sitPlayer("p2", "Bob", 1);
  lobby.sitPlayer("p3", "Charlie", 2);
  lobby.setButtonPos(-1);
  g.startHand();

  // Play to showdown normally (Button=Alice, SB=Bob, BB=Charlie)
  assert(g.playerAction("p1", "call", 0) == true);
  assert(g.playerAction("p2", "call", 0) == true);
  assert(g.playerAction("p3", "check", 0) == true);

  // Post-Flop: 3 streets of checks to reach showdown
  for (int street = 0; street < 3; street++) {
    // Post-flop order: left of button = Bob(1), then Charlie(2), then Alice(0)
    assert(g.playerAction("p2", "check", 0) == true);
    assert(g.playerAction("p3", "check", 0) == true);
    assert(g.playerAction("p1", "check", 0) == true);
  }

  // Should be at showdown now
  assert(g.getStage() == GameStage::Showdown);
  assert(g.getPot() == 0);
  assert(g.getIsAllInShowdown() == false); // Normal showdown, not all-in

  // Find who won and who lost
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

  // Winner must show, loser doesn't have to
  assert(winnerSeat >= 0);
  assert(g.getSeats()[winnerSeat].showCards == true);

  if (loserSeat >= 0) {
    // Loser starts with cards hidden
    assert(g.getSeats()[loserSeat].showCards == false);

    // Loser can choose to show
    std::string loserId = g.getSeats()[loserSeat].id;
    assert(g.playerMuckOrShow(loserId, true) == true);
    assert(g.getSeats()[loserSeat].showCards == true);

    // Loser can muck again
    assert(g.playerMuckOrShow(loserId, false) == true);
    assert(g.getSeats()[loserSeat].showCards == false);

    // Winner can't muck
    std::string winnerId = g.getSeats()[winnerSeat].id;
    assert(g.playerMuckOrShow(winnerId, false) == false);
    assert(g.getSeats()[winnerSeat].showCards == true); // Still showing
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
