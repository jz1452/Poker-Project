#include "Player.h"
#include <nlohmann/json.hpp>

namespace poker {

// Helper to convert status to string
NLOHMANN_JSON_SERIALIZE_ENUM(PlayerStatus,
                             {{PlayerStatus::SittingOut, "SittingOut"},
                              {PlayerStatus::Waiting, "Waiting"},
                              {PlayerStatus::Active, "Active"},
                              {PlayerStatus::Folded, "Folded"},
                              {PlayerStatus::AllIn, "AllIn"}})

void to_json(nlohmann::json &j, const Player &p) {
  j = nlohmann::json{{"id", p.id},
                     {"name", p.name},
                     {"chips", p.chips},
                     {"currentBet", p.currentBet},
                     {"totalBet", p.totalBet},
                     {"status", p.status},
                     {"hand", p.hand},
                     {"showCards", p.showCards},
                     {"isConnected", p.isConnected}};
}

} // namespace poker
