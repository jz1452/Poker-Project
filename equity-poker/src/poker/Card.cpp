#include "Card.h"
#include <stdexcept>

namespace poker {

using namespace std;

static const char RANK_CHARS[] = "23456789TJQKA";
static const char SUIT_CHARS[] = "cdhs"; // Clubs, Diamonds, Hearts, Spades

string Card::toString() const {
  int r = val & 0xF;
  int s = (val >> 4) & 0x3;

  // Safety check just in case
  if (r > 12 || s > 3)
    return "??";

  char buffer[3] = {RANK_CHARS[r], SUIT_CHARS[s], '\0'};
  return string(buffer);
}

Card Card::fromString(const string &s) {
  if (s.length() < 2)
    throw invalid_argument("Invalid card string");

  char rChar = s[0];
  char sChar = s[1];

  int rank = -1;
  for (int i = 0; i <= 12; i++) {
    if (RANK_CHARS[i] == rChar) {
      rank = i;
      break;
    }
  }

  int suit = -1;
  for (int i = 0; i <= 3; i++) {
    if (SUIT_CHARS[i] == sChar) {
      suit = i;
      break;
    }
  }

  if (rank == -1 || suit == -1)
    throw invalid_argument("Invalid card string");

  return Card(rank, suit);
}

ostream &operator<<(ostream &os, const Card &c) {
  os << c.toString();
  return os;
}

} // namespace poker