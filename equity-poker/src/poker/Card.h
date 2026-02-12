#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace poker {

// The core "Bitwise" Card representation.
// We use a single byte (uint8_t) to represent a card.
//
// Bits 0-3: Rank (0-12) -> 2, 3, 4, 5, 6, 7, 8, 9, T, J, Q, K, A
// Bits 4-5: Suit (0-3)  -> Clubs(0), Diamonds(1), Hearts(2), Spades(3)
// Bits 6-7: Reserved (0)
//
// Example: Ace of Spades
// Rank: Ace = 12 (1100 binary)
// Suit: Spades = 3 (11 binary)
// Binary: 00 11 1100 -> 0x3C (Decimal 60)

struct Card {
  uint8_t val;

  // Constructors
  constexpr Card() : val(0) {}
  constexpr Card(uint8_t v) : val(v) {}
  constexpr Card(int rank, int suit)
      : val(static_cast<uint8_t>((suit << 4) | rank)) {}

  // Comparison
  bool operator==(const Card &other) const { return val == other.val; }
  bool operator!=(const Card &other) const { return val != other.val; }
  bool operator<(const Card &other) const { return val < other.val; }

  // Accessors
  constexpr int rank() const { return val & 0xF; }
  constexpr int suit() const { return (val >> 4) & 0x3; }

  // Constants
  static const int RANK_2 = 0;
  static const int RANK_A = 12;
  static const int SUIT_CLUBS = 0;
  static const int SUIT_DIAMONDS = 1;
  static const int SUIT_HEARTS = 2;
  static const int SUIT_SPADES = 3;

  // Output
  std::string toString() const;

  // Static Helpers
  static Card fromString(const std::string &s);
};

// Stream operator for easy printing
std::ostream &operator<<(std::ostream &os, const Card &c);

} // namespace poker
