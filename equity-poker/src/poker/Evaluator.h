#pragma once

#include "Card.h"
#include <vector>

namespace poker {

// Hand Evaluator using Paul Senzee's perfect hash method
// Super fast O(1) lookup
class Evaluator {
public:
  // Evaluates 5, 6, or 7 cards and returns a rank (1 = Royal Flush)
  static int evaluate(const std::vector<Card> &cards);

private:
  // Helper for just 5 cards
  static int evaluate5(const Card &c1, const Card &c2, const Card &c3,
                       const Card &c4, const Card &c5);
};

} // namespace poker
