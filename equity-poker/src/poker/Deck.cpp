#include "Deck.h"
#include <algorithm>
#include <stdexcept>

namespace poker {

Deck::Deck() {
  cards.reserve(52); // optimise memory allocation
  // Loop Ranks (0-12) and Suits (0-3)
  for (int s = 0; s < 4; s++) {
    for (int r = 0; r < 13; r++) {
      cards.emplace_back(r, s);
    }
  }
}

void Deck::shuffle(std::mt19937 &rng) {
  if (cards.size() < 52) {
    // If cards were dealt, we need to reset before shuffling
    cards.clear();
    for (int s = 0; s < 4; s++) {
      for (int r = 0; r < 13; r++) {
        cards.emplace_back(r, s);
      }
    }
  }
  std::shuffle(cards.begin(), cards.end(), rng);
}

Card Deck::deal() {
  if (cards.empty())
    throw std::runtime_error("Deck empty");

  Card c = cards.back(); // getting top card
  cards.pop_back();
  return c;
}

} // namespace poker
