#include "chain.hpp"
#include <iostream>

static void printChain(const Blockchain& bc) {
    for (const Block& b : bc.blocks()) {
        std::cout << "  [" << b.height << "]"
                  << "  hash=" << b.hash.substr(0, 16) << "..."
                  << "  pow="  << b.pow << "\n";
    }
}

int main() {
    constexpr int DIFFICULTY = 4;

    Blockchain bc(DIFFICULTY);

    // -----------------------------------------------------------------------
    // Add 4 blocks on top of the genesis block.
    // addBlock() mines before appending, so each call takes a moment.
    // -----------------------------------------------------------------------
    bc.addBlock({{ Transaction{"Alice",   "Bob",     500, 1, 1716500100, "sig1"} }});
    bc.addBlock({{ Transaction{"Bob",     "Charlie", 200, 1, 1716500200, "sig2"} }});
    bc.addBlock({{ Transaction{"Charlie", "Dave",    100, 1, 1716500300, "sig3"} }});
    bc.addBlock({{ Transaction{"Dave",    "Alice",    50, 1, 1716500400, "sig4"} }});

    std::cout << "=== chain (" << bc.length() << " blocks) ===\n";
    printChain(bc);
    std::cout << "valid: " << (bc.isValid() ? "YES" : "NO") << "\n\n";

    // -----------------------------------------------------------------------
    // Tamper: mutate a transaction deep in the chain.
    // isValid() must catch it because the stored hash no longer matches
    // a fresh recalculation of that block.
    // -----------------------------------------------------------------------
    std::cout << "--- tamper block 2, amount 200 -> 9999 ---\n";
    // chain_ is private, but we can get a mutable ref via const_cast for the demo.
    // In production code nothing outside Blockchain touches the vector directly.
    Block& tampered = const_cast<Block&>(bc.blocks()[2]);
    tampered.data.transactions[0].amount = 9999;

    std::cout << "valid after tamper: " << (bc.isValid() ? "YES" : "NO") << "\n";

    return 0;
}
