#pragma once

#include "Card.h"
#include <random>
#include <vector>

namespace poker {

// Basic 52-card deck
class Deck {
public:
  Deck();

  // Shuffle using a random number generator
  void shuffle(std::mt19937 &rng);

  // Deal one card from the top
  Card deal();

  // How many cards left?
  size_t count() const { return cards.size(); }

private:
  std::vector<Card> cards; // the deck
};

} // namespace poker
