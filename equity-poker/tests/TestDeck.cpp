#include "../src/poker/Card.h"
#include "../src/poker/Deck.h"
#include <iostream>
#include <random>
#include <vector>

int main() {
  std::cout << "--- DECK TEST START ---" << std::endl;

  // 1. Setup RNG
  std::random_device rd;
  std::mt19937 rng(rd());

  // 2. Create Deck
  poker::Deck deck;
  std::cout << "Deck created. Count: " << deck.count() << std::endl;

  // 3. Shuffle
  deck.shuffle(rng);
  std::cout << "Deck shuffled." << std::endl;

  // 4. Deal 5 Cards
  std::cout << "Dealing 5 cards: ";
  for (int i = 0; i < 5; i++) {
    poker::Card c = deck.deal();
    std::cout << c << " ";
  }
  std::cout << std::endl;

  std::cout << "Cards remaining: " << deck.count() << std::endl;
  std::cout << "--- DECK TEST END ---" << std::endl;

  return 0;
}
