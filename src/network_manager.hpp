#pragma once

#include "chain.hpp"
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// NetworkManager
// Extends Layer 4's single-connection Node into a full mesh.
//
// Each instance is both a server and a client simultaneously:
//   - A background thread runs the TCP accept loop (server role).
//   - addPeer() dials out to a known peer (client role).
//
// When a peer connects (either direction) we run the same syncWith exchange
// from Layer 4.  After adopting or rejecting their chain we broadcast our
// current chain to every other known peer so the update propagates.
//
// Concurrency model:
//   - chain_ is protected by chainMutex_. Every read or write locks it.
//   - peers_ is protected by peersMutex_.
//   - Each accepted connection is handled in its own detached thread.
// ---------------------------------------------------------------------------
class NetworkManager {
public:
    NetworkManager(int difficulty, int listenPort);
    ~NetworkManager();

    // Dial out to a peer and perform an initial chain sync.
    void addPeer(const std::string& host, int port);

    // Mine a new block and broadcast it to all peers.
    void submitTransaction(Transaction tx);

    // Validate sender balance against the current chain, then submit.
    // Returns false and prints a reason if the transaction is invalid.
    bool submitManualTransaction(Transaction tx);

    int64_t getBalance(const std::string& address) const;
    std::map<std::string, int64_t> allBalances() const;

    void printChain() const;
    int  chainLength() const;

    // Block the calling thread until Ctrl-C / stop() is called.
    void run();
    void stop();

private:
    // ---- shared state ----
    mutable std::mutex chainMutex_;
    Blockchain         chain_;

    std::mutex                              peersMutex_;
    std::vector<std::pair<std::string,int>> peers_;  // (host, port)

    std::atomic<bool>                             stopMining {false};

    // ---- server ----
    int              listenPort_;
    int              serverFd_  = -1;
    std::atomic<bool> running_  {false};

    // ---- internals ----
    void acceptLoop();

    // Open a connection to (host, port), sync, close.
    void syncWithPeer(const std::string& host, int port);

    // Send our chain to fd, read theirs, adopt if better.
    // Returns true if we adopted their chain.
    bool exchangeChains(int fd);

    // Push our current chain to every known peer.
    void broadcast();

    std::string serializeChain() const;
    void        adoptIfBetter(Blockchain incoming);

    static std::string readLine(int fd);
    static void        writeLine(int fd, const std::string& s);
};
