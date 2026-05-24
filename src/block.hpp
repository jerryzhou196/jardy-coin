#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Transaction
// Represents a transfer of value between two parties.
// Amounts are stored as integers (smallest unit) to avoid floating-point drift.
// ---------------------------------------------------------------------------
struct Transaction {
    std::string from;
    std::string to;
    int64_t     amount;     // e.g. 150 = 1.50 coins with 2 decimal places
    int64_t     fee;
    int64_t     timestamp;  // Unix seconds
    std::string signature;  // placeholder — real auth added in a later layer

    // Pipe-separated so fields can't accidentally merge.
    std::string serialize() const;
};

// ---------------------------------------------------------------------------
// BlockData
// The payload a block commits to. Right now it's just a list of transactions.
// Serializing concatenates all transactions with newlines.
// ---------------------------------------------------------------------------
struct BlockData {
    std::vector<Transaction> transactions;
    std::string serialize() const;
};

// ---------------------------------------------------------------------------
// Block
// One link in the chain. Key fields:
//   previousHash  — binds this block to the one before it
//   height        — position in the chain (genesis = 0)
//   pow           — the nonce incremented during mining
//   hash          — SHA-256 of (previousHash + data + timestamp + height + pow)
//
// mine(difficulty) loops, incrementing pow, until the hash starts with
// `difficulty` leading zeros. That's proof-of-work.
// ---------------------------------------------------------------------------
class Block {
public:
    BlockData   data;
    std::string hash;
    std::string previousHash;
    int64_t     timestamp;
    int         height;
    int         pow;

    Block(BlockData data, std::string previousHash, int height);

    // Recomputes the hash from current field values.
    std::string calculateHash() const;

    // Increments pow until hash has `difficulty` leading '0' characters.
    void mine(int difficulty);

private:
    static int64_t currentTimestamp();
};
