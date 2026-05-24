#include "chain.hpp"
#include <iomanip>
#include <map>
#include <ostream>

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
    // The genesis block is hardcoded — every node must start from the same root.
    // We use the restore constructor with timestamp=0, pow=0, and an empty hash,
    // then compute the hash deterministically from those fixed fields.
    // No clock, no mining: every instance produces the exact same block.
    BlockData data{{ Transaction{"network", "genesis", 0, 0, 0, "genesis"} }};
    Block b(data, "0", 0, /*timestamp=*/0, /*pow=*/0, /*hash=*/"");
    b.hash = b.calculateHash();
    return b;
}

void Blockchain::addBlock(BlockData data) {
    const Block& tip = chain_.back();
    Block b(std::move(data), tip.hash, tip.height + 1);
    b.mine(difficulty_);
    chain_.push_back(std::move(b));
}

void Blockchain::appendBlock(Block b) {
    chain_.push_back(std::move(b));
}

int64_t Blockchain::getBalance(const std::string& address) const {
    int64_t balance = 0;
    for (const Block& b : chain_) {
        for (const Transaction& tx : b.data.transactions) {
            if (tx.to   == address) balance += tx.amount;
            if (tx.from == address) balance -= (tx.amount + tx.fee);
        }
    }
    return balance;
}

std::map<std::string, int64_t> Blockchain::allBalances() const {
    std::map<std::string, int64_t> balances;
    for (const Block& b : chain_) {
        for (const Transaction& tx : b.data.transactions) {
            if (tx.from != "network") balances[tx.from] -= (tx.amount + tx.fee);
            balances[tx.to] += tx.amount;
        }
    }
    for (auto it = balances.begin(); it != balances.end(); ) {
        it = (it->second == 0) ? balances.erase(it) : std::next(it);
    }
    return balances;
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

std::ostream& operator<<(std::ostream& os, const Blockchain& bc) {
    os << "Blockchain [" << bc.chain_.size() << " blocks, difficulty=" << bc.difficulty_ << "]\n";
    for (const Block& b : bc.chain_) {
        os << "  [" << std::setw(4) << b.height << "]"
           << "  hash="     << b.hash.substr(0, 16)         << "..."
           << "  prev="     << b.previousHash.substr(0, 8)  << "..."
           << "  txs="      << b.data.transactions.size()
           << "  pow="      << b.pow
           << "\n";
        for (const Transaction& tx : b.data.transactions) {
            os << "           "
               << tx.from << " -> " << tx.to
               << "  amount=" << tx.amount
               << "\n";
        }
    }
    return os;
}
