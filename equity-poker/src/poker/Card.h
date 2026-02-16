#pragma once

#include <cstdint>
#include <iostream>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace poker {

// Represents a card
// Just using one byte for efficiency
// structure: ssssrrrr (actually just using lower bits mostly)
// 0-3: rank, 4-5: suit
// basically a char

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
  static const int RANK_3 = 1;
  static const int RANK_4 = 2;
  static const int RANK_5 = 3;
  static const int RANK_6 = 4;
  static const int RANK_7 = 5;
  static const int RANK_8 = 6;
  static const int RANK_9 = 7;
  static const int RANK_T = 8;
  static const int RANK_J = 9;
  static const int RANK_Q = 10;
  static const int RANK_K = 11;
  static const int RANK_A = 12;
  static const int SUIT_CLUBS = 0;
  static const int SUIT_DIAMONDS = 1;
  static const int SUIT_HEARTS = 2;
  static const int SUIT_SPADES = 3;

  // Output
  std::string toString() const;

  // Make a card from string (e.g. "Ah")
  static Card fromString(const std::string &s);
};

// Stream operator for easy printing
std::ostream &operator<<(std::ostream &os, const Card &c);

// JSON Serialization
void to_json(nlohmann::json &j, const Card &c);

} // namespace poker
