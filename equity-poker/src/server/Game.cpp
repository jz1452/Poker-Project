#include "Game.h"
#include "../poker/Evaluator.h"
#include <iostream>

namespace poker {

// --- Lobby Actions ---

int Game::sitPlayer(std::string id, std::string name, int seatIndex) {
  if (seatIndex < 0 || seatIndex >= config.maxSeats)
    return -1;
  // Check if seat is already taken
  if (seats[seatIndex].status != PlayerStatus::SittingOut &&
      !seats[seatIndex].id.empty()) {
    return -1;
  }

  seats[seatIndex].id = id;
  seats[seatIndex].name = name;
  seats[seatIndex].chips = config.startingStack;
  seats[seatIndex].status = PlayerStatus::Waiting; // Wait for next hand

  return seatIndex;
}

bool Game::standPlayer(std::string id) {
  for (auto &p : seats) {
    if (p.id == id) {
      // Reset player to default state
      p = Player();
      p.status = PlayerStatus::SittingOut;
      p.id = "";
      return true;
    }
  }
  return false;
}

// --- Game Flow ---

void Game::startHand() {
  int activeCount = 0;
  for (const auto &p : seats) {
    if (p.status != PlayerStatus::SittingOut && p.chips > 0)
      activeCount++;
  }
  if (activeCount < 2)
    return;

  // 1. Reset State
  pot = 0;
  currentBet = 0;
  board.clear();
  sidePots.clear();
  deck = Deck();
  deck.shuffle(rng);

  // 2. Move Button
  int attempts = 0;
  do {
    buttonPos = (buttonPos + 1) % config.maxSeats;
    attempts++;
  } while ((seats[buttonPos].status == PlayerStatus::SittingOut ||
            seats[buttonPos].chips == 0) &&
           attempts < config.maxSeats * 2);

  // 3. Mark Players Active & Reset Hands
  for (auto &p : seats) {
    if (p.status != PlayerStatus::SittingOut) {
      p.resetHand();
    }
  }

  // 4. Post Blinds
  if (activeCount == 2) {
    sbPos = buttonPos;
    bbPos = nextActivePlayer(sbPos);
  } else {
    sbPos = nextActivePlayer(buttonPos);
    bbPos = nextActivePlayer(sbPos);
  }

  // Post SB
  int sbAmount = std::min(config.smallBlind, seats[sbPos].chips);
  seats[sbPos].chips -= sbAmount;
  seats[sbPos].currentBet = sbAmount;
  seats[sbPos].totalBet = sbAmount;
  pot += sbAmount;

  // Post BB
  int bbAmount = std::min(config.bigBlind, seats[bbPos].chips);
  seats[bbPos].chips -= bbAmount;
  seats[bbPos].currentBet = bbAmount;
  seats[bbPos].totalBet = bbAmount;
  pot += bbAmount;

  currentBet = config.bigBlind;
  minRaise = config.bigBlind;

  // 5. Deal Cards
  for (int i = 0; i < 2; i++) {
    int dealIdx = nextActivePlayer(buttonPos);
    for (int j = 0; j < activeCount; j++) {
      seats[dealIdx].hand.push_back(deck.deal());
      dealIdx = nextActivePlayer(dealIdx);
    }
  }

  // 6. Set Actor
  if (activeCount == 2) {
    currentActor = sbPos;
  } else {
    currentActor = nextActivePlayer(bbPos);
  }

  lastAggressor = currentActor;
  stage = GameStage::PreFlop;
}

// Find next seated, non-folded player
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

// Only return players who have chips (Active)
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

  // 1. Validate Turn
  Player &p = seats[currentActor];
  if (p.id != id)
    return false;
  // 2. Validate Actions
  if (action == "fold") {
    p.status = PlayerStatus::Folded;
    // Check for immediate win
    if (activePlayerCount() == 1) {
      distributePot();
      startHand();
      return true;
    }
  } else if (action == "call") {
    int callCost = currentBet - p.currentBet;
    if (callCost >= p.chips) {
      // Auto-convert to All-In
      return playerAction(id, "allin", 0);
    }

    p.chips -= callCost;
    p.currentBet += callCost;
    p.totalBet += callCost;
    pot += callCost;
  } else if (action == "check") {
    if (currentBet > p.currentBet)
      return false;
  } else if (action == "raise") {
    // Amount is the TOTAL bet (e.g. "Raise to 200")
    int raiseAmount = amount;
    int toAdd = raiseAmount - p.currentBet;

    // Must normally be >= minRaise + currentBet
    if (toAdd > p.chips)
      return false;
    if (raiseAmount < currentBet + minRaise)
      return false;
    p.chips -= toAdd;
    p.currentBet += toAdd;
    p.totalBet += toAdd;
    pot += toAdd;

    // Update Game State
    int raiseDiff = raiseAmount - currentBet;
    currentBet = raiseAmount;
    minRaise = raiseDiff;         // Min raise rule
    lastAggressor = currentActor; // Action re-opens for everyone
  } else if (action == "allin") {
    int allInAmount = p.chips;
    p.chips = 0;

    p.currentBet += allInAmount;
    p.totalBet += allInAmount;
    pot += allInAmount;
    p.status = PlayerStatus::AllIn;

    // Strict Re-Open
    if (p.currentBet > currentBet) {
      int raiseSize = p.currentBet - currentBet;

      // Does this All-In reopen the betting?
      if (raiseSize >= minRaise) {
        // Yes, it's a full raise
        minRaise = raiseSize;
        lastAggressor = currentActor;
      }
      // No, it's a short-stack shove.
      // Action does NOT reopen for those who already acted.
      // dont update lastAggressor.
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

  // Rule 1: We completed a full orbit (Next player is Last Aggressor)
  if (next == lastAggressor) {
    roundEnds = true;

    // Exception: Pre-Flop, if nobody raised, the Big Blind gets an option to
    // raise
    if (stage == GameStage::PreFlop && next == bbPos &&
        currentBet == config.bigBlind) {
      roundEnds = false;
    }
  }

  // Rule 2: The Big Blind checked their Option (Pre-Flop only)
  if (stage == GameStage::PreFlop && currentActor == bbPos &&
      currentActor == lastAggressor) {
    // If they checked, next street
    if (seats[currentActor].currentBet == currentBet) {
      roundEnds = true;
    }
  }

  if (roundEnds) {
    nextStreet();
    return;
  }

  currentActor = next;

  // Check for "Auto-Walk" (Everyone else All-In/Folded)
  // If only 1 person has chips left to bet, run the rest of the board
  if (activePlayerCount() > 1) {
    int bettingPlayers = 0;
    for (auto &p : seats)
      if (p.status == PlayerStatus::Active)
        bettingPlayers++;
    if (bettingPlayers < 2) {
      while (stage != GameStage::Showdown)
        nextStreet();
    }
  }
}

void Game::nextStreet() {
  // 1. Reset betting
  currentBet = 0;
  minRaise = config.bigBlind;
  lastAggressor = -1; // Reset aggressor

  for (auto &p : seats) {
    if (p.status != PlayerStatus::Folded &&
        p.status != PlayerStatus::SittingOut) {
      p.currentBet = 0;
    }
  }
  // 2. Advance Stage
  if (stage == GameStage::PreFlop) {
    stage = GameStage::Flop;
  } else if (stage == GameStage::Flop) {
    stage = GameStage::Turn;
  } else if (stage == GameStage::Turn) {
    stage = GameStage::River;
  } else if (stage == GameStage::River) {
    stage = GameStage::Showdown;
    distributePot();
    startHand();
    return;
  }
  // 3. Deal Cards
  int cardsToDeal = (stage == GameStage::Flop) ? 3 : 1;
  if (!deck.getCards().empty())
    deck.deal(); // Burn
  for (int i = 0; i < cardsToDeal; i++) {
    if (!deck.getCards().empty())
      board.push_back(deck.deal());
  }

  // 4. Set First Actor (Left of Button)
  currentActor = nextBettingPlayer(buttonPos);
  lastAggressor = currentActor;
}

void Game::resolveSidePots() {
  sidePots.clear();

  // 1. Identify distinct bet levels (from shortest stack to largest)
  std::vector<int> levels;
  for (const auto &p : seats) {
    if (p.totalBet > 0)
      levels.push_back(p.totalBet);
  }
  std::sort(levels.begin(), levels.end());
  levels.erase(std::unique(levels.begin(), levels.end()), levels.end());

  // 2. Build Pots for each level
  int previousLevel = 0;
  for (int level : levels) {
    int contribution = level - previousLevel;
    SidePot sp;
    sp.amount = 0;

    for (int i = 0; i < config.maxSeats; i++) {
      if (seats[i].totalBet >= level) {
        // Full contribution for this level
        sp.amount += contribution;
        // Eligible to win if they haven't folded
        if (seats[i].status != PlayerStatus::Folded &&
            seats[i].status != PlayerStatus::SittingOut) {
          sp.eligiblePlayers.push_back(seats[i].id);
        }
      } else if (seats[i].totalBet > previousLevel) {
        // Partial contribution (All-In for less)
        sp.amount += (seats[i].totalBet - previousLevel);
        // NOT eligible for this level
      }
    }

    if (sp.amount > 0)
      sidePots.push_back(sp);
    previousLevel = level;
  }
}

void Game::distributePot() {
  // 1. Calculate the pots structure
  resolveSidePots();

  Evaluator eval;

  // 2. Process each pot (Main Pot is just the first one)
  for (const auto &sp : sidePots) {
    if (sp.eligiblePlayers.empty())
      continue;

    // Convert IDs back to seat indices
    std::vector<int> eligibleIdx;
    for (const std::string &id : sp.eligiblePlayers) {
      for (int i = 0; i < config.maxSeats; i++) {
        if (seats[i].id == id) {
          eligibleIdx.push_back(i);
          break;
        }
      }
    }

    // 3. Find Winner for current pot
    int bestScore = 99999;
    std::vector<int> winners;

    if (eligibleIdx.size() == 1) {
      // Optimization: Auto-win if only 1 player (everyone else folded)
      winners.push_back(eligibleIdx[0]);
    } else {
      for (int idx : eligibleIdx) {
        std::vector<Card> sevenCards = seats[idx].hand;
        sevenCards.insert(sevenCards.end(), board.begin(), board.end());

        int score = 99999;
        if (sevenCards.size() >= 5) {
          score = eval.evaluate(sevenCards);
        }

        if (score < bestScore) {
          bestScore = score;
          winners.clear();
          winners.push_back(idx);
        } else if (score == bestScore) {
          winners.push_back(idx);
        }
      }
    }

    // 4. Distribute Chips
    if (winners.empty())
      continue;
    int share = sp.amount / winners.size();
    int remainder = sp.amount % winners.size();

    for (int w : winners) {
      seats[w].chips += share;
    }

    // Odd Chip
    if (remainder > 0) {
      int current = buttonPos;
      while (remainder > 0) {
        current = nextActivePlayer(current);
        for (int w : winners) {
          if (w == current) {
            seats[w].chips += 1;
            remainder--;
            break;
          }
        }
      }
    }
  }

  pot = 0;
}

} // namespace poker