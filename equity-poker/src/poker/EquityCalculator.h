#pragma once

#include "Card.h"
#include <vector>

namespace poker {

class EquityCalculator {
public:
  // Calculate equity for specific known hands
  // Returns a vector of equities (e.g. [0.75, 0.25])
  // - hands: a vector where each element is a list of 2 hole cards
  // - board: 0, 3, 4, or 5 cards
  static std::vector<double>
  calculateEquity(const std::vector<std::vector<Card>> &hands,
                  const std::vector<Card> &board);
};

} // namespace poker
