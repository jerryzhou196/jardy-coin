#include "block.hpp"
#include "crypto.hpp"

#include <chrono>
#include <sstream>
#include <iostream>

// --- Transaction ---

std::string Transaction::serialize() const {
    std::ostringstream oss;
    oss << from      << "|"
        << to        << "|"
        << amount    << "|"
        << fee       << "|"
        << timestamp << "|"
        << signature;
    return oss.str();
}

// --- BlockData ---

std::string BlockData::serialize() const {
    std::ostringstream oss;
    for (const auto& tx : transactions) {
        oss << tx.serialize() << "\n";
    }
    return oss.str();
}

// --- Block ---

Block::Block(BlockData data, std::string previousHash, int height)
    : data(std::move(data))
    , previousHash(std::move(previousHash))
    , timestamp(currentTimestamp())
    , height(height)
    , pow(0)
{
    hash = calculateHash();
}

Block::Block(BlockData data, std::string previousHash, int height,
             int64_t timestamp, int pow, std::string hash)
    : data(std::move(data))
    , previousHash(std::move(previousHash))
    , timestamp(timestamp)
    , height(height)
    , pow(pow)
    , hash(std::move(hash))
{}

std::string Block::calculateHash() const {
    std::ostringstream oss;
    oss << previousHash    << "\n"
        << data.serialize() << "\n"
        << timestamp        << "\n"
        << height           << "\n"
        << pow;
    return sha256(oss.str());
}

void Block::mine(int difficulty) {
    std::atomic<bool> neverStop{false};
    mine(difficulty, neverStop);
}

void Block::mine(int difficulty, std::atomic<bool>& stop) {
    const std::string target(difficulty, '0');
    while (hash.substr(0, difficulty) != target && !stop.load()) {
        ++pow;
        hash = calculateHash();
    } 
    if (!stop.load()){
        std::cout << "[net] mined block " << height << " with hash " << hash << std::endl;
    } else {
        std::cout << "[net] mining stopped because of better block\n";
    }
}

int64_t Block::currentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}
