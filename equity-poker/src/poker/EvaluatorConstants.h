#pragma once

#include <vector>

namespace poker {

// Prime numbers for Ranks 2..A
static const int PRIMES[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41};

// Lookup tables for the Perfect Hash optimisation
extern short flushes[];              // check for flush rank
extern short unique5[];              // check for straight/high card
extern unsigned short hash_adjust[]; // help with hash function
extern unsigned short hash_values[]; // map product to rank

} // namespace poker
