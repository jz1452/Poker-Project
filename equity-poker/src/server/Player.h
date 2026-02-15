#pragma once
#include "../poker/Card.h"
#include <string>
#include <vector>

namespace poker {

enum class PlayerStatus {
  SittingOut,
  Waiting, // Joined, but waiting for next hand
  Active,  // In the hand
  Folded,
  AllIn
};

struct Player {
  std::string id;   // Unique ID (WebSocket UUID)
  std::string name; // Display Name

  int chips = 0;      // Total stack
  int currentBet = 0; // Bet in the CURRENT betting round (for resolving pulls)
  int totalBet = 0;   // Total bet in the entire hand (for Side Pots)

  PlayerStatus status = PlayerStatus::Waiting;
  std::vector<Card> hand; // 2 Hole Cards

  // For disconnection handling
  bool isConnected = true;

  // Resets state for a new hand
  void resetHand() {
    status = (chips > 0) ? PlayerStatus::Active : PlayerStatus::SittingOut;
    currentBet = 0;
    totalBet = 0;
    hand.clear();
  }
};

} // namespace poker