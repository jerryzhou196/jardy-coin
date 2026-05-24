#include "block.hpp"

#include <iostream>

// Layer 1 driver — manually builds 3 blocks and mines each one.
// Nothing is persisted or networked yet; this just proves the core
// hashing and proof-of-work loop work correctly before we add the chain.

static void printBlock(const Block& b) {
    std::cout << "  height   : " << b.height        << "\n"
              << "  prevHash : " << b.previousHash  << "\n"
              << "  hash     : " << b.hash          << "\n"
              << "  pow      : " << b.pow           << "\n\n";
}

int main() {
    constexpr int DIFFICULTY = 4;   // hash must start with "0000"

    // -----------------------------------------------------------------------
    // Block 0 — genesis
    // The genesis block is special: its previousHash is an arbitrary sentinel
    // ("0") because nothing came before it.
    // -----------------------------------------------------------------------
    BlockData genesisData{{ Transaction{"network", "Alice", 1000, 0, 1716500000, "genesis"} }};
    Block genesis(genesisData, "0", 0);
    genesis.mine(DIFFICULTY);

    std::cout << "=== Block 0 (genesis) ===\n";
    printBlock(genesis);

    // -----------------------------------------------------------------------
    // Block 1
    // previousHash = genesis.hash  →  any change to genesis breaks this hash.
    // -----------------------------------------------------------------------
    BlockData data1{{ Transaction{"Alice", "Bob", 500, 1, 1716500100, "sig_alice"} }};
    Block block1(data1, genesis.hash, 1);
    block1.mine(DIFFICULTY);

    std::cout << "=== Block 1 ===\n";
    printBlock(block1);

    // -----------------------------------------------------------------------
    // Block 2
    // -----------------------------------------------------------------------
    BlockData data2{{ Transaction{"Bob", "Charlie", 200, 1, 1716500200, "sig_bob"} }};
    Block block2(data2, block1.hash, 2);
    block2.mine(DIFFICULTY);

    std::cout << "=== Block 2 ===\n";
    printBlock(block2);

    // -----------------------------------------------------------------------
    // Tamper check — mutate Block 1's data and recompute its hash.
    // block2.previousHash still points to the OLD block1.hash, so the chain
    // is now broken.  Layer 2 will add isValid() to catch this automatically.
    // -----------------------------------------------------------------------
    std::cout << "--- tamper demo ---\n";
    block1.data.transactions[0].amount = 9999;          // attacker changes amount
    std::string tamperedHash = block1.calculateHash();  // new hash after mutation
    std::cout << "block1 stored hash : " << block1.hash    << "\n";
    std::cout << "block1 fresh hash  : " << tamperedHash   << "\n";
    std::cout << "block2 prevHash    : " << block2.previousHash << "\n";
    std::cout << (tamperedHash == block2.previousHash ? "chain intact" : "chain BROKEN") << "\n";

    return 0;
}
