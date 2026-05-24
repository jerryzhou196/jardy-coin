#pragma once

#include "chain.hpp"
#include <string>

// ---------------------------------------------------------------------------
// Node
// Owns one Blockchain and exposes two modes that mirror the Go article:
//
//   listen(port)             — wait for an incoming connection, exchange
//                             chains, adopt the peer's if it's longer.
//
//   connect(host, port)      — dial a peer, exchange chains, same rule.
//
// "Exchange" means: send our chain as a JSON line, then read theirs.
// The longest valid chain wins.  This is the core of Nakamoto consensus.
//
// Both calls block until the single exchange is complete.  Multi-peer
// broadcasting comes in Layer 5 (NetworkManager).
// ---------------------------------------------------------------------------
class Node {
public:
    explicit Node(int difficulty);

    // Add a transaction to the next block and mine it.
    void submitTransaction(Transaction tx);

    void listen(int port);
    void connect(const std::string& host, int port);

    void printChain() const;
    int  chainLength() const;

private:
    Blockchain chain_;

    // Send our chain to `fd`, receive theirs, adopt if longer + valid.
    void syncWith(int fd);

    // Serialize chain to a single newline-terminated JSON string.
    std::string serialize() const;

    // Replace chain_ with `incoming` if it is longer and valid.
    void adoptIfBetter(Blockchain incoming);
};
