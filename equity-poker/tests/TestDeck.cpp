#include "../src/poker/Card.h"
#include "../src/poker/Deck.h"
#include "../src/poker/Evaluator.h"
#include <cassert>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace poker;

// function to check if the rank is correct
void assert_exact_rank(const std::string &handName, int score, int expected) {
  if (score == expected) {
    std::cout << "[PASS] " << handName << ": " << score << std::endl;
  } else {
    // exit eagerly if it fails
    std::cout << "[FAIL] " << handName << ": " << score
              << " != Expected: " << expected << std::endl;
    std::exit(1);
  }
}

void testEvaluator() {
  std::cout << "\n--- TESTING THE EVALUATOR ---\n" << std::endl;

  // 1. Royal Flush (Ah Kh Qh Jh Th) => Rank 1
  std::vector<Card> rf = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                          Card(Card::RANK_K, Card::SUIT_HEARTS),
                          Card(Card::RANK_Q, Card::SUIT_HEARTS),
                          Card(Card::RANK_J, Card::SUIT_HEARTS),
                          Card(Card::RANK_T, Card::SUIT_HEARTS)};
  assert_exact_rank("Royal Flush", Evaluator::evaluate(rf), 1);

  // 2. Straight Flush (9s 8s 7s 6s 5s) => Rank 6
  std::vector<Card> sf = {Card(Card::RANK_9, Card::SUIT_SPADES),
                          Card(Card::RANK_8, Card::SUIT_SPADES),
                          Card(Card::RANK_7, Card::SUIT_SPADES),
                          Card(Card::RANK_6, Card::SUIT_SPADES),
                          Card(Card::RANK_5, Card::SUIT_SPADES)};
  assert_exact_rank("Straight Flush (9-high)", Evaluator::evaluate(sf), 6);

  // 3. Four of a Kind (Ah Ad Ac As 5h) => Rank 19 (AAAA5)
  std::vector<Card> quads = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                             Card(Card::RANK_A, Card::SUIT_DIAMONDS),
                             Card(Card::RANK_A, Card::SUIT_CLUBS),
                             Card(Card::RANK_A, Card::SUIT_SPADES),
                             Card(Card::RANK_5, Card::SUIT_HEARTS)};
  assert_exact_rank("Four of a Kind (AAAA5)", Evaluator::evaluate(quads), 19);

  // 4. Full House (Ah Ad Ac Ks Kh) => Rank 167 (AAA KK)
  std::vector<Card> fh = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                          Card(Card::RANK_A, Card::SUIT_DIAMONDS),
                          Card(Card::RANK_A, Card::SUIT_CLUBS),
                          Card(Card::RANK_K, Card::SUIT_SPADES),
                          Card(Card::RANK_K, Card::SUIT_HEARTS)};
  assert_exact_rank("Full House (AAA KK)", Evaluator::evaluate(fh), 167);

  // 5. Flush (Ah Jh 9h 8h 2h) => Rank 640
  std::vector<Card> flush = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                             Card(Card::RANK_J, Card::SUIT_HEARTS),
                             Card(Card::RANK_9, Card::SUIT_HEARTS),
                             Card(Card::RANK_8, Card::SUIT_HEARTS),
                             Card(Card::RANK_2, Card::SUIT_HEARTS)};
  assert_exact_rank("Flush (AJ982)", Evaluator::evaluate(flush), 640);

  // 6. Straight (Ah 5d 4c 3s 2h) => Rank 1609 (Wheel 5-high straight)
  std::vector<Card> straight = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                                Card(Card::RANK_5, Card::SUIT_DIAMONDS),
                                Card(Card::RANK_4, Card::SUIT_CLUBS),
                                Card(Card::RANK_3, Card::SUIT_SPADES),
                                Card(Card::RANK_2, Card::SUIT_HEARTS)};
  assert_exact_rank("Straight (Wheel)", Evaluator::evaluate(straight), 1609);

  // 7. Three of a Kind (Ah Ad Ac Ks Qh) => Rank 1610 (AAA KQ)
  std::vector<Card> trips = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                             Card(Card::RANK_A, Card::SUIT_DIAMONDS),
                             Card(Card::RANK_A, Card::SUIT_CLUBS),
                             Card(Card::RANK_K, Card::SUIT_SPADES),
                             Card(Card::RANK_Q, Card::SUIT_HEARTS)};
  assert_exact_rank("Three of a Kind (AAA KQ)", Evaluator::evaluate(trips),
                    1610);

  // 8. Two Pair (Kh Kd Qc Qs 2h) => Rank 2610 (KK QQ 2)
  std::vector<Card> twoPair = {Card(Card::RANK_K, Card::SUIT_HEARTS),
                               Card(Card::RANK_K, Card::SUIT_DIAMONDS),
                               Card(Card::RANK_Q, Card::SUIT_CLUBS),
                               Card(Card::RANK_Q, Card::SUIT_SPADES),
                               Card(Card::RANK_2, Card::SUIT_HEARTS)};
  assert_exact_rank("Two Pair (KK QQ 2)", Evaluator::evaluate(twoPair), 2610);

  // 9. One Pair (Ah Ad Kc Qs Jh) => Rank 3326 (AA K Q J)
  std::vector<Card> pair = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                            Card(Card::RANK_A, Card::SUIT_DIAMONDS),
                            Card(Card::RANK_K, Card::SUIT_CLUBS),
                            Card(Card::RANK_Q, Card::SUIT_SPADES),
                            Card(Card::RANK_J, Card::SUIT_HEARTS)};
  assert_exact_rank("One Pair (AA K Q J)", Evaluator::evaluate(pair), 3326);

  // 10. High Card (Ah Kd Qc Js 9h) => Rank 6186 (A K Q J 9)
  std::vector<Card> highCard = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                                Card(Card::RANK_K, Card::SUIT_DIAMONDS),
                                Card(Card::RANK_Q, Card::SUIT_CLUBS),
                                Card(Card::RANK_J, Card::SUIT_SPADES),
                                Card(Card::RANK_9, Card::SUIT_HEARTS)};
  assert_exact_rank("High Card (AKQJ9)", Evaluator::evaluate(highCard), 6186);

  // 11. 6-Card Hand (Flush + Extra Card)
  // Hand: Ah Kh Qh Jh Th  +  2s
  // Best 5: Ah Kh Qh Jh Th (Royal Flush, Rank 1)
  std::vector<Card> sixCards = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                                Card(Card::RANK_K, Card::SUIT_HEARTS),
                                Card(Card::RANK_Q, Card::SUIT_HEARTS),
                                Card(Card::RANK_J, Card::SUIT_HEARTS),
                                Card(Card::RANK_T, Card::SUIT_HEARTS),
                                Card(Card::RANK_2, Card::SUIT_SPADES)};
  assert_exact_rank("6-Card Hand (Royal Flush best 5)",
                    Evaluator::evaluate(sixCards), 1);

  // 12. 7-Card Hand (Royal Flush + 2 Extra)
  // Hand: Ah Kh Qh Jh Th 2s 3s
  // Best 5 cards: Ah Kh Qh Jh Th (Royal Flush) which is Rank 1
  std::vector<Card> sevenCards = {Card(Card::RANK_A, Card::SUIT_HEARTS),
                                  Card(Card::RANK_K, Card::SUIT_HEARTS),
                                  Card(Card::RANK_Q, Card::SUIT_HEARTS),
                                  Card(Card::RANK_J, Card::SUIT_HEARTS),
                                  Card(Card::RANK_T, Card::SUIT_HEARTS),
                                  Card(Card::RANK_2, Card::SUIT_SPADES),
                                  Card(Card::RANK_3, Card::SUIT_SPADES)};
  assert_exact_rank("7-Card Hand (Royal Flush best 5)",
                    Evaluator::evaluate(sevenCards), 1);

  std::cout << "\nAll Evaluator Tests PASSED!" << std::endl;
}

int main() {
  testEvaluator();
  return 0;
}
