#pragma once

#include "block.hpp"
#include <map>
#include <ostream>
#include <vector>

// ---------------------------------------------------------------------------
// Blockchain
// Owns the ordered list of blocks and enforces two invariants:
//
//   1. Linkage  — each block's previousHash equals the hash of the block
//                 before it.
//   2. Integrity — each block's stored hash matches a fresh recalculation,
//                  so any field mutation is immediately detectable.
//
// The genesis block (height 0) is created in the constructor. Every
// subsequent block is added via addBlock(), which mines it before appending.
// ---------------------------------------------------------------------------
class Blockchain {
public:
    // Normal constructor: creates and mines the genesis block.
    explicit Blockchain(int difficulty);

    // Restore constructor: used by deserialization to load a pre-built chain
    // received from the network. No genesis block is created; blocks are
    // stored as-is and isValid() should be called to verify them.
    Blockchain(std::vector<Block> blocks, int difficulty);

    // Mine a new block containing `data` and append it to the chain.
    void addBlock(BlockData data);

    // Append an already-mined block (used by NetworkManager to mine outside the lock).
    void appendBlock(Block b);

    // Returns true only if every block satisfies both invariants above.
    bool isValid() const;

    // Sum of all incoming amounts minus all outgoing amounts for an address.
    // "network" is the coinbase source and is never debited.
    int64_t getBalance(const std::string& address) const;

    // Returns balances for every address that has appeared in the chain,
    // excluding "network" (the coinbase source) and zero-balance entries.
    std::map<std::string, int64_t> allBalances() const;

    const std::vector<Block>& blocks()     const { return chain_; }
    int                       length()     const { return static_cast<int>(chain_.size()); }
    int                       difficulty() const { return difficulty_; }

    friend std::ostream& operator<<(std::ostream& os, const Blockchain& bc);

private:
    std::vector<Block> chain_;
    int difficulty_;

    Block makeGenesis() const;
};
