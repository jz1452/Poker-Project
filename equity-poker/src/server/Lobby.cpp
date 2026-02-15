#include "Lobby.h"

namespace poker {

int Lobby::sitPlayer(std::string id, std::string name, int seatIndex) {
  if (seatIndex < 0 || seatIndex >= (int)game.seats.size())
    return -1;
  // Check if seat is already taken
  if (game.seats[seatIndex].status != PlayerStatus::SittingOut &&
      !game.seats[seatIndex].id.empty()) {
    return -1;
  }

  game.seats[seatIndex].id = id;
  game.seats[seatIndex].name = name;
  game.seats[seatIndex].chips = game.config.startingStack;
  game.seats[seatIndex].status = PlayerStatus::Waiting;

  return seatIndex;
}

bool Lobby::standPlayer(std::string id) {
  for (auto &p : game.seats) {
    if (p.id == id) {
      p = Player();
      p.status = PlayerStatus::SittingOut;
      p.id = "";
      return true;
    }
  }
  return false;
}

void Lobby::setPlayerStack(int seatIndex, int amount) {
  if (seatIndex >= 0 && seatIndex < (int)game.seats.size()) {
    game.seats[seatIndex].chips = amount;
  }
}

void Lobby::setButtonPos(int pos) { game.buttonPos = pos; }

} // namespace poker
