#include "Evaluator.h"
#include "EvaluatorConstants.h"
#include <iomanip>
#include <iostream>

namespace poker {

// Perfect Hash Function
static unsigned find_fast(unsigned u) {
  unsigned a, b, r;
  u += 0xe91aaa35;
  u ^= u >> 16;
  u += u << 8;
  u ^= u >> 4;
  b = (u >> 8) & 0x1ff;
  a = (u + (u << 2)) >> 19;
  r = a ^ hash_adjust[b];
  return r;
}

// Change our card to the Cactus Kev format
// p = prime number
// r = rank
// s = suit bit
// b = rank bit
static int convert_card(const Card &c) {
  // 1. Prime (0-7)
  int p = PRIMES[c.rank()];

  // 2. Rank (8-11)
  int r = (c.rank() << 8);

  // 3. Suit (12-15)
  int s = (1 << (12 + c.suit()));

  // 4. Rank Bit (16-31)
  int b = (1 << (16 + c.rank()));

  return b | s | r | p;
}

int Evaluator::evaluate(const std::vector<Card> &cards) {
  // If 5 cards -> evaluate5
  if (cards.size() == 5) {
    return evaluate5(cards[0], cards[1], cards[2], cards[3], cards[4]);
  }

  int bestScore = 9999;
  int n = cards.size();

  // If 6 or 7 cards -> Loop 21 combinations
  for (int i = 0; i < n - 4; i++) {
    for (int j = i + 1; j < n - 3; j++) {
      for (int k = j + 1; k < n - 2; k++) {
        for (int l = k + 1; l < n - 1; l++) {
          for (int m = l + 1; m < n; m++) {
            int score =
                evaluate5(cards[i], cards[j], cards[k], cards[l], cards[m]);
            if (score < bestScore)
              bestScore = score;
          }
        }
      }
    }
  }
  return bestScore;
}

int Evaluator::evaluate5(const Card &c1_obj, const Card &c2_obj,
                         const Card &c3_obj, const Card &c4_obj,
                         const Card &c5_obj) {
  // Convert to Cactus Kev Format
  int c1 = convert_card(c1_obj);
  int c2 = convert_card(c2_obj);
  int c3 = convert_card(c3_obj);
  int c4 = convert_card(c4_obj);
  int c5 = convert_card(c5_obj);

  int q = (c1 | c2 | c3 | c4 | c5) >> 16;
  short s;

  // Check Flush
  // (c1 & ... & 0xF000) checks if ALL cards share a suit bit
  if (c1 & c2 & c3 & c4 & c5 & 0xF000)
    return flushes[q];

  // Check Rank (Unique5 Lookup)
  if ((s = unique5[q]))
    return s;

  // Check Rank (Perfect Hash Lookup for non-unique-rank hands like Pairs)
  // We multiply only the lower 8 bits (the Primes)
  return hash_values[find_fast((c1 & 0xff) * (c2 & 0xff) * (c3 & 0xff) *
                               (c4 & 0xff) * (c5 & 0xff))];
}

} // namespace poker
