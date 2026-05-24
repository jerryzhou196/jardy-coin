#include "chain.hpp"

Blockchain::Blockchain(int difficulty)
    : difficulty_(difficulty)
{
    chain_.push_back(makeGenesis());
}

Blockchain::Blockchain(std::vector<Block> blocks, int difficulty)
    : chain_(std::move(blocks))
    , difficulty_(difficulty)
{}

Block Blockchain::makeGenesis() const {
    BlockData data{{ Transaction{"network", "genesis", 0, 0, 0, "genesis"} }};
    Block b(data, "0", 0);
    b.mine(difficulty_);
    return b;
}

void Blockchain::addBlock(BlockData data) {
    const Block& tip = chain_.back();
    Block b(std::move(data), tip.hash, tip.height + 1);
    b.mine(difficulty_);
    chain_.push_back(std::move(b));
}

bool Blockchain::isValid() const {
    for (int i = 0; i < static_cast<int>(chain_.size()); ++i) {
        const Block& b = chain_[i];

        // Invariant 1: stored hash must match a fresh recalculation.
        if (b.hash != b.calculateHash()) {
            return false;
        }

        // Invariant 2: every block except genesis must link to the one before it.
        if (i > 0 && b.previousHash != chain_[i - 1].hash) {
            return false;
        }
    }
    return true;
}
