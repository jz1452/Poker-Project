#include "EquityCalculator.h"
#include "Deck.h"
#include "Evaluator.h"
#include <algorithm>
#include <future>
#include <random>
#include <thread>
#include <vector>

namespace poker {

using namespace std;

// Helper to run a chunk of simulations
static vector<double> runSimulations(int iterations,
                                     const vector<vector<Card>> &hands,
                                     const vector<Card> &board,
                                     const vector<Card> &deck) {

  int numPlayers = hands.size();
  vector<double> wins(numPlayers, 0.0);

  // Local RNG for this thread
  random_device rd;
  mt19937 rng(rd());

  // Local copy of deck to shuffle
  vector<Card> currentDeck = deck;

  // Buffers for 7 cards per player
  vector<vector<Card>> playerHandCards(numPlayers);
  for (int i = 0; i < numPlayers; i++) {
    playerHandCards[i].reserve(7);
  }

  // Simulation Loop
  for (int i = 0; i < iterations; i++) {
    // 1. Shuffle remaining deck
    shuffle(currentDeck.begin(), currentDeck.end(), rng);

    // 2. Deal missing board cards
    int cardsNeeded = 5 - board.size();

    // 3. Evaluate each player
    int bestRank = 9999;
    vector<int> playerRanks(numPlayers);

    for (int p = 0; p < numPlayers; p++) {
      // Construct full hand: Hole Cards + Board + Dealt Board
      playerHandCards[p] = hands[p];
      playerHandCards[p].insert(playerHandCards[p].end(), board.begin(),
                                board.end());

      // Add random cards from deck for the rest of the board
      for (int k = 0; k < cardsNeeded; k++) {
        playerHandCards[p].push_back(currentDeck[k]);
      }

      playerRanks[p] = Evaluator::evaluate(playerHandCards[p]);
      if (playerRanks[p] < bestRank)
        bestRank = playerRanks[p];
    }

    // 4. Award wins (handle splits/ties)
    int winners = 0;
    for (int r : playerRanks) {
      if (r == bestRank)
        winners++;
    }

    double winShare = 1.0 / winners;
    for (int p = 0; p < numPlayers; p++) {
      if (playerRanks[p] == bestRank) {
        wins[p] += winShare;
      }
    }
  }

  return wins;
}

vector<double>
EquityCalculator::calculateEquity(const vector<vector<Card>> &hands,
                                  const vector<Card> &board) {

  // 1. Create the "Remaining Deck"
  // Start with full deck, remove all hole cards and board cards
  // 1. Create the "Remaining Deck"
  // Optimisation: Use a boolean array to mark used cards (Rank * 4 + Suit)
  // 13 ranks * 4 suits = 52 cards
  bool usedCards[52] = {false};

  for (const auto &hand : hands) {
    for (const auto &card : hand) {
      // 0..51 index
      int idx = card.rank() * 4 + card.suit();
      usedCards[idx] = true;
    }
  }

  for (const auto &card : board) {
    int idx = card.rank() * 4 + card.suit();
    usedCards[idx] = true;
  }

  // Rebuild the deck
  Deck fullDeck;
  std::vector<Card> remainingDeck;
  remainingDeck.reserve(52 - (hands.size() * 2 + board.size()));

  for (int r = 0; r < 13; r++) {
    for (int s = 0; s < 4; s++) {
      int idx = r * 4 + s;
      if (!usedCards[idx]) {
        remainingDeck.emplace_back(r, s);
      }
    }
  }

  // 2. Multithreading Setup
  int totalIterations = 100000; // 100k sims
  int numThreads = thread::hardware_concurrency();
  if (numThreads == 0)
    numThreads = 4;

  int simsPerThread = totalIterations / numThreads;

  vector<future<vector<double>>> futures;

  // 3. Launch Threads
  for (int i = 0; i < numThreads; i++) {
    futures.push_back(async(launch::async, runSimulations, simsPerThread, hands,
                            board, remainingDeck));
  }

  // 4. Aggregate Results
  vector<double> totalWins(hands.size(), 0.0);
  for (auto &f : futures) {
    vector<double> threadWins = f.get();
    for (size_t p = 0; p < hands.size(); p++) {
      totalWins[p] += threadWins[p];
    }
  }

  // 5. Convert to Percentage (0.0 - 1.0)
  vector<double> equities;
  for (double w : totalWins) {
    equities.push_back(w / totalIterations);
  }

  return equities;
}

} // namespace poker
