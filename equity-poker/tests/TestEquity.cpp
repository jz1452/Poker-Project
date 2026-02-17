#include "../src/poker/Card.h"
#include "../src/poker/Deck.h"
#include "../src/poker/EquityCalculator.h"
#include "../src/poker/Evaluator.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace poker;

void assert_equity(const std::string &label, double actual, double expected,
                   double tolerance = 0.05) {
  if (std::abs(actual - expected) < tolerance) {
    std::cout << "[PASS] " << label << ": " << actual << " (Exp: " << expected
              << ")" << std::endl;
  } else {
    std::cout << "[FAIL] " << label << ": " << actual << " (Exp: " << expected
              << ")" << std::endl;
    std::exit(1);
  }
}

void testEquity() {
  std::cout << "\n--- TESTING EQUITY CALCULATOR ---\n" << std::endl;

  // 1. AA vs 22 (Preflop) -> Approx 82% vs 18%
  std::vector<Card> p1 = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                          Card(Card::RANK_A, Card::SUIT_DIAMONDS)};
  std::vector<Card> p2 = {Card(Card::RANK_2, Card::SUIT_CLUBS),
                          Card(Card::RANK_2, Card::SUIT_SPADES)};
  std::vector<Card> board = {};

  auto equities = EquityCalculator::calculateEquity({p1, p2}, board);
  assert_equity("AA vs 22 (Preflop)", equities[0], 0.82);

  // 2. KK vs AA (Cooler) -> Approx 18% vs 82%
  std::vector<Card> kk = {Card(Card::RANK_K, Card::SUIT_HEARTS),
                          Card(Card::RANK_K, Card::SUIT_DIAMONDS)};
  equities = EquityCalculator::calculateEquity({kk, p1}, board);
  assert_equity("KK vs AA (Preflop)", equities[0], 0.18);

  // 3. Dominating Hand (AK vs AQ) -> Approx 74% vs 26%
  std::vector<Card> ak = {Card(Card::RANK_A, Card::SUIT_CLUBS),
                          Card(Card::RANK_K, Card::SUIT_SPADES)};
  std::vector<Card> aq = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                          Card(Card::RANK_Q, Card::SUIT_DIAMONDS)};
  equities = EquityCalculator::calculateEquity({ak, aq}, board);
  assert_equity("AK vs AQ (Preflop)", equities[0], 0.74);

  // 4. Post-Flop: Flush Draw vs Top Pair
  // Board: As Ks 2d
  // P1: Qs Js (Flush Draw + Straight Draw)
  // Top Pair P2: Ad Kd (Top Two Pair)
  std::vector<Card> draw = {Card(Card::RANK_Q, Card::SUIT_SPADES),
                            Card(Card::RANK_J, Card::SUIT_SPADES)};
  std::vector<Card> topTwo = {Card(Card::RANK_A, Card::SUIT_DIAMONDS),
                              Card(Card::RANK_K, Card::SUIT_DIAMONDS)};
  std::vector<Card> flop = {Card(Card::RANK_A, Card::SUIT_SPADES),
                            Card(Card::RANK_K, Card::SUIT_SPADES),
                            Card(Card::RANK_2, Card::SUIT_DIAMONDS)};

  // Royal Flush Draw vs Top Two Pair
  // (Standard deviation is small with 100k iters)
  equities = EquityCalculator::calculateEquity({draw, topTwo}, flop);
  assert_equity("Flush Draw vs Top Two Pair", equities[0], 0.42, 0.02);

  // 5. Split Pot (Chop)
  // Board: A K Q J T (Royal Flush on board)
  // P1: 2s 3s
  // P2: 4d 5d
  // Should be 50/50
  std::vector<Card> boardChop = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                                 Card(Card::RANK_K, Card::SUIT_HEARTS),
                                 Card(Card::RANK_Q, Card::SUIT_HEARTS),
                                 Card(Card::RANK_J, Card::SUIT_HEARTS),
                                 Card(Card::RANK_T, Card::SUIT_HEARTS)};
  std::vector<Card> junk1 = {Card(Card::RANK_2, Card::SUIT_SPADES),
                             Card(Card::RANK_3, Card::SUIT_SPADES)};
  std::vector<Card> junk2 = {Card(Card::RANK_4, Card::SUIT_DIAMONDS),
                             Card(Card::RANK_5, Card::SUIT_DIAMONDS)};

  equities = EquityCalculator::calculateEquity({junk1, junk2}, boardChop);
  assert_equity("Chop Pot (Royal on Board)", equities[0], 0.50, 0.01);

  std::cout << "\nAll Equity Tests PASSED!" << std::endl;
}

int main() {
  testEquity();
  return 0;
}
