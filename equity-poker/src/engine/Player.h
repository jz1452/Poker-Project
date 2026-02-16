#pragma once
#include "../poker/Card.h"
#include <string>
#include <vector>

namespace poker {

enum class PlayerStatus { SittingOut, Waiting, Active, Folded, AllIn };

struct Player {
  std::string id;
  std::string name;

  int chips = 0;
  int currentBet = 0; // Bet in current betting round
  int totalBet = 0;   // Total bet across the entire hand (for side pots)

  PlayerStatus status = PlayerStatus::SittingOut;
  std::vector<Card> hand;

  bool showCards = false;
  bool isConnected = true;

  void resetHand() {
    if (!isConnected || chips <= 0) {
      status = PlayerStatus::SittingOut;
    } else {
      status = PlayerStatus::Active;
    }
    currentBet = 0;
    totalBet = 0;
    hand.clear();
    showCards = false;
  }
};

// JSON Serialization
#include <nlohmann/json_fwd.hpp>
void to_json(nlohmann::json &j, const Player &p);

} // namespace poker