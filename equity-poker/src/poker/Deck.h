#pragma once

#include "Card.h"
#include <random>
#include <vector>

namespace poker {

class Deck {
public:
  Deck();

  // re-fill the deck with 52 cards and shuffle
  void shuffle(std::mt19937 &rng); // Mersenne Twister

  // deals 1 card. Returns the Card object.
  Card deal();

  // shows how many cards in the deck
  size_t count() const { return cards.size(); }

private:
  std::vector<Card> cards; // the deck
};

} // namespace poker
