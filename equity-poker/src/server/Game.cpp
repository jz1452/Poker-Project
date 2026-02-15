#include "Game.h"
#include "../poker/Evaluator.h"
#include <iostream>

namespace poker {

void Game::startHand() {
  int activeCount = 0;
  for (const auto &p : seats) {
    if (p.status != PlayerStatus::SittingOut && p.chips > 0)
      activeCount++;
  }
  if (activeCount < 2)
    return;

  // Reset state
  pot = 0;
  currentBet = 0;
  board.clear();
  sidePots.clear();
  showdownResults.clear();
  isAllInShowdown = false;
  foldWinner = -1;
  deck = Deck();
  deck.shuffle(rng);

  // Move button to next eligible player
  int attempts = 0;
  do {
    buttonPos = (buttonPos + 1) % config.maxSeats;
    attempts++;
  } while ((seats[buttonPos].status == PlayerStatus::SittingOut ||
            seats[buttonPos].chips == 0) &&
           attempts < config.maxSeats * 2);

  for (auto &p : seats) {
    if (p.status != PlayerStatus::SittingOut)
      p.resetHand();
  }

  // Heads-up: button is SB. Otherwise SB is left of button.
  if (activeCount == 2) {
    sbPos = buttonPos;
    bbPos = nextActivePlayer(sbPos);
  } else {
    sbPos = nextActivePlayer(buttonPos);
    bbPos = nextActivePlayer(sbPos);
  }

  int sbAmount = std::min(config.smallBlind, seats[sbPos].chips);
  seats[sbPos].chips -= sbAmount;
  seats[sbPos].currentBet = sbAmount;
  seats[sbPos].totalBet = sbAmount;
  pot += sbAmount;

  int bbAmount = std::min(config.bigBlind, seats[bbPos].chips);
  seats[bbPos].chips -= bbAmount;
  seats[bbPos].currentBet = bbAmount;
  seats[bbPos].totalBet = bbAmount;
  pot += bbAmount;

  currentBet = config.bigBlind;
  minRaise = config.bigBlind;

  // Deal 2 cards to each player, starting left of button
  for (int i = 0; i < 2; i++) {
    int dealIdx = nextActivePlayer(buttonPos);
    for (int j = 0; j < activeCount; j++) {
      seats[dealIdx].hand.push_back(deck.deal());
      dealIdx = nextActivePlayer(dealIdx);
    }
  }

  // Heads-up: SB acts first pre-flop. Otherwise left of BB.
  if (activeCount == 2) {
    currentActor = sbPos;
  } else {
    currentActor = nextActivePlayer(bbPos);
  }

  lastAggressor = currentActor;
  stage = GameStage::PreFlop;
}

// Next player who hasn't folded (includes All-In)
int Game::nextActivePlayer(int current) {
  int idx = current;
  for (int i = 0; i < config.maxSeats; i++) {
    idx = (idx + 1) % config.maxSeats;
    if (seats[idx].status == PlayerStatus::Active ||
        seats[idx].status == PlayerStatus::AllIn) {
      return idx;
    }
  }
  return current;
}

// Next player who can still bet (Active only, skips All-In)
int Game::nextBettingPlayer(int current) {
  int idx = current;
  for (int i = 0; i < config.maxSeats; i++) {
    idx = (idx + 1) % config.maxSeats;
    if (seats[idx].status == PlayerStatus::Active) {
      return idx;
    }
  }
  return current;
}

int Game::activePlayerCount() const {
  int count = 0;
  for (const auto &p : seats) {
    if (p.status == PlayerStatus::Active || p.status == PlayerStatus::AllIn)
      count++;
  }
  return count;
}

bool Game::playerAction(std::string id, std::string action, int amount) {
  if (stage == GameStage::Showdown)
    return false;

  Player &p = seats[currentActor];
  if (p.id != id)
    return false;

  if (action == "fold") {
    p.status = PlayerStatus::Folded;
    if (activePlayerCount() == 1) {
      // Record fold-winner so they can optionally show
      for (int i = 0; i < config.maxSeats; i++) {
        if (seats[i].status == PlayerStatus::Active ||
            seats[i].status == PlayerStatus::AllIn) {
          foldWinner = i;
          break;
        }
      }
      distributePot();
      return true;
    }
  } else if (action == "call") {
    int callCost = currentBet - p.currentBet;
    if (callCost >= p.chips)
      return playerAction(id, "allin", 0); // Auto all-in

    p.chips -= callCost;
    p.currentBet += callCost;
    p.totalBet += callCost;
    pot += callCost;
  } else if (action == "check") {
    if (currentBet > p.currentBet)
      return false;
  } else if (action == "raise") {
    // amount is the TOTAL bet (e.g. "raise to 200")
    int toAdd = amount - p.currentBet;

    if (toAdd > p.chips)
      return false;
    if (amount < currentBet + minRaise)
      return false;

    p.chips -= toAdd;
    p.currentBet += toAdd;
    p.totalBet += toAdd;
    pot += toAdd;

    int raiseDiff = amount - currentBet;
    currentBet = amount;
    minRaise = raiseDiff;
    lastAggressor = currentActor;
  } else if (action == "allin") {
    int allInAmount = p.chips;
    p.chips = 0;
    p.currentBet += allInAmount;
    p.totalBet += allInAmount;
    pot += allInAmount;
    p.status = PlayerStatus::AllIn;

    if (p.currentBet > currentBet) {
      int raiseSize = p.currentBet - currentBet;
      // Only reopen betting if it's a full raise
      if (raiseSize >= minRaise) {
        minRaise = raiseSize;
        lastAggressor = currentActor;
      }
      // Short shove: doesn't reopen, don't update lastAggressor
      currentBet = p.currentBet;
    }
  } else {
    return false;
  }

  nextTurn();
  return true;
}

void Game::nextTurn() {
  int next = nextBettingPlayer(currentActor);
  bool roundEnds = false;

  // Full orbit complete or no other bettor remains
  if (next == lastAggressor || next == currentActor) {
    roundEnds = true;
    // Exception: BB option pre-flop (hasn't acted yet)
    if (stage == GameStage::PreFlop && next == bbPos &&
        currentBet == config.bigBlind && next != currentActor) {
      roundEnds = false;
    }
  }

  // BB checked their option
  if (stage == GameStage::PreFlop && currentActor == bbPos &&
      currentActor == lastAggressor) {
    if (seats[currentActor].currentBet == currentBet)
      roundEnds = true;
  }

  if (roundEnds) {
    if (stage == GameStage::Showdown)
      return;
    nextStreet();
    // Auto-walk remaining streets if < 2 players can bet (all-in scenario)
    if (stage != GameStage::Showdown) {
      int bettingPlayers = 0;
      for (auto &p : seats)
        if (p.status == PlayerStatus::Active)
          bettingPlayers++;
      if (bettingPlayers < 2 && activePlayerCount() > 1) {
        while (stage != GameStage::Showdown)
          nextStreet();
      }
    }
    return;
  }

  currentActor = next;
}

void Game::nextStreet() {
  currentBet = 0;
  minRaise = config.bigBlind;
  lastAggressor = -1;

  for (auto &p : seats) {
    if (p.status != PlayerStatus::Folded &&
        p.status != PlayerStatus::SittingOut) {
      p.currentBet = 0;
    }
  }

  if (stage == GameStage::PreFlop) {
    stage = GameStage::Flop;
  } else if (stage == GameStage::Flop) {
    stage = GameStage::Turn;
  } else if (stage == GameStage::Turn) {
    stage = GameStage::River;
  } else if (stage == GameStage::River) {
    stage = GameStage::Showdown;
    distributePot();
    return;
  }

  // Deal community cards (3 on flop, 1 on turn/river)
  int cardsToDeal = (stage == GameStage::Flop) ? 3 : 1;
  if (!deck.getCards().empty())
    deck.deal(); // Burn
  for (int i = 0; i < cardsToDeal; i++) {
    if (!deck.getCards().empty())
      board.push_back(deck.deal());
  }

  // Post-flop action starts left of button
  currentActor = nextBettingPlayer(buttonPos);
  lastAggressor = currentActor;
}

void Game::resolveSidePots() {
  sidePots.clear();

  // Collect distinct bet levels
  std::vector<int> levels;
  for (const auto &p : seats) {
    if (p.totalBet > 0)
      levels.push_back(p.totalBet);
  }
  std::sort(levels.begin(), levels.end());
  levels.erase(std::unique(levels.begin(), levels.end()), levels.end());

  // Build a pot for each level
  int previousLevel = 0;
  for (int level : levels) {
    int contribution = level - previousLevel;
    SidePot sp;
    sp.amount = 0;

    for (int i = 0; i < config.maxSeats; i++) {
      if (seats[i].totalBet >= level) {
        sp.amount += contribution;
        if (seats[i].status != PlayerStatus::Folded &&
            seats[i].status != PlayerStatus::SittingOut) {
          sp.eligiblePlayers.push_back(seats[i].id);
        }
      } else if (seats[i].totalBet > previousLevel) {
        sp.amount += (seats[i].totalBet - previousLevel);
      }
    }

    if (sp.amount > 0)
      sidePots.push_back(sp);
    previousLevel = level;
  }
}

void Game::distributePot() {
  resolveSidePots();

  Evaluator eval;
  showdownResults.clear();

  // Detect all-in showdown (< 2 Active players = everyone was all-in)
  int activeBettors = 0;
  for (const auto &p : seats) {
    if (p.status == PlayerStatus::Active)
      activeBettors++;
  }
  isAllInShowdown = (activeBettors < 2);

  std::vector<int> chipsWonPerSeat(config.maxSeats, 0);

  for (const auto &sp : sidePots) {
    if (sp.eligiblePlayers.empty())
      continue;

    // Map player IDs back to seat indices
    std::vector<int> eligibleIdx;
    for (const std::string &id : sp.eligiblePlayers) {
      for (int i = 0; i < config.maxSeats; i++) {
        if (seats[i].id == id) {
          eligibleIdx.push_back(i);
          break;
        }
      }
    }

    // Find best hand(s)
    int bestScore = 99999;
    std::vector<int> winners;

    if (eligibleIdx.size() == 1) {
      winners.push_back(eligibleIdx[0]);
    } else {
      for (int idx : eligibleIdx) {
        std::vector<Card> sevenCards = seats[idx].hand;
        sevenCards.insert(sevenCards.end(), board.begin(), board.end());

        int score = 99999;
        if (sevenCards.size() >= 5)
          score = eval.evaluate(sevenCards);

        if (score < bestScore) {
          bestScore = score;
          winners.clear();
          winners.push_back(idx);
        } else if (score == bestScore) {
          winners.push_back(idx);
        }
      }
    }

    if (winners.empty())
      continue;
    int share = sp.amount / winners.size();
    int remainder = sp.amount % winners.size();

    for (int w : winners) {
      seats[w].chips += share;
      chipsWonPerSeat[w] += share;
    }

    // Odd chip goes to first winner left of button
    if (remainder > 0) {
      int current = buttonPos;
      while (remainder > 0) {
        current = nextActivePlayer(current);
        for (int w : winners) {
          if (w == current) {
            seats[w].chips += 1;
            chipsWonPerSeat[w] += 1;
            remainder--;
            break;
          }
        }
      }
    }
  }

  // Build showdown results (skip for fold-wins)
  if (stage == GameStage::Showdown) {
    for (int i = 0; i < config.maxSeats; i++) {
      if (seats[i].status == PlayerStatus::Folded ||
          seats[i].status == PlayerStatus::SittingOut ||
          seats[i].status == PlayerStatus::Waiting)
        continue;

      ShowdownResult result;
      result.seatIndex = i;
      result.chipsWon = chipsWonPerSeat[i];

      std::vector<Card> sevenCards = seats[i].hand;
      sevenCards.insert(sevenCards.end(), board.begin(), board.end());
      if (sevenCards.size() >= 5)
        result.handRank = eval.evaluate(sevenCards);

      result.mustShow = (chipsWonPerSeat[i] > 0) || isAllInShowdown;
      seats[i].showCards = result.mustShow;

      showdownResults.push_back(result);
    }
  }

  pot = 0;
}

bool Game::playerMuckOrShow(std::string id, bool show) {
  if (stage != GameStage::Showdown && foldWinner == -1)
    return false;

  for (int i = 0; i < config.maxSeats; i++) {
    if (seats[i].id == id) {
      // Fold-winner can freely toggle
      if (foldWinner == i) {
        seats[i].showCards = show;
        return true;
      }

      // Outside showdown, only fold-winner is allowed
      if (stage != GameStage::Showdown)
        return false;

      // Can't muck if forced to show (winner or all-in)
      for (const auto &r : showdownResults) {
        if (r.seatIndex == i && r.mustShow)
          return false;
      }
      seats[i].showCards = show;
      return true;
    }
  }
  return false;
}

} // namespace poker