#pragma once

#include "block.hpp"
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

    // Returns true only if every block satisfies both invariants above.
    bool isValid() const;

    const std::vector<Block>& blocks()     const { return chain_; }
    int                       length()     const { return static_cast<int>(chain_.size()); }
    int                       difficulty() const { return difficulty_; }

private:
    std::vector<Block> chain_;
    int difficulty_;

    Block makeGenesis() const;
};
