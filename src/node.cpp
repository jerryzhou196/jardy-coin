#include "node.hpp"
#include "serialization.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Read from `fd` until we see a newline, returning everything up to it.
// This is our simple framing protocol: one JSON object per line.
static std::string readLine(int fd) {
    std::string result;
    char ch;
    while (true) {
        ssize_t n = read(fd, &ch, 1);
        if (n <= 0) break;
        if (ch == '\n') break;
        result += ch;
    }
    return result;
}

// Write `s` followed by a newline so the receiver's readLine() terminates.
static void writeLine(int fd, const std::string& s) {
    std::string msg = s + "\n";
    size_t sent = 0;
    while (sent < msg.size()) {
        ssize_t n = write(fd, msg.c_str() + sent, msg.size() - sent);
        if (n <= 0) throw std::runtime_error("write failed");
        sent += n;
    }
}

// ---------------------------------------------------------------------------
// Node
// ---------------------------------------------------------------------------

Node::Node(int difficulty) : chain_(difficulty) {}

void Node::submitTransaction(Transaction tx) {
    chain_.addBlock(BlockData{{ std::move(tx) }});
}

std::string Node::serialize() const {
    return json(chain_).dump();   // compact — no extra whitespace
}

void Node::adoptIfBetter(Blockchain incoming) {
    if (incoming.isValid() && incoming.length() > chain_.length()) {
        std::cout << "[sync] adopted peer chain  len="
                  << incoming.length() << "\n";
        chain_ = std::move(incoming);
    } else {
        std::cout << "[sync] kept own chain  len=" << chain_.length() << "\n";
    }
}

void Node::syncWith(int fd) {
    // Both sides send first, then read — so neither blocks waiting for the
    // other to go first.  Works for exactly two peers; Layer 5 will sequence
    // this properly for N peers.
    writeLine(fd, serialize());

    std::string raw = readLine(fd);
    if (raw.empty()) {
        std::cerr << "[sync] received empty payload\n";
        return;
    }

    try {
        Blockchain peer = blockchain_from_json(json::parse(raw));
        adoptIfBetter(std::move(peer));
    } catch (const std::exception& e) {
        std::cerr << "[sync] parse error: " << e.what() << "\n";
    }
}

// ---------------------------------------------------------------------------
// listen — server side
// ---------------------------------------------------------------------------
void Node::listen(int port) {
    // Create a TCP socket.
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) throw std::runtime_error("socket() failed");

    // SO_REUSEADDR lets us restart the process without waiting for TIME_WAIT.
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;   // accept on all interfaces
    addr.sin_port        = htons(port);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (::listen(serverFd, 1) < 0)
        throw std::runtime_error("listen() failed");

    std::cout << "[node] listening on port " << port << "\n";

    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (clientFd < 0) throw std::runtime_error("accept() failed");

    char peerIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, peerIp, sizeof(peerIp));
    std::cout << "[node] connected to peer " << peerIp << "\n";

    syncWith(clientFd);

    close(clientFd);
    close(serverFd);
}

// ---------------------------------------------------------------------------
// connect — client side
// ---------------------------------------------------------------------------
void Node::connect(const std::string& host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket() failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
        throw std::runtime_error("invalid address: " + host);

    std::cout << "[node] connecting to " << host << ":" << port << "\n";

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("connect() failed");

    std::cout << "[node] connected\n";

    syncWith(fd);

    close(fd);
}

// ---------------------------------------------------------------------------
// printChain
// ---------------------------------------------------------------------------
void Node::printChain() const {
    for (const Block& b : chain_.blocks()) {
        std::cout << "  [" << b.height << "]"
                  << "  hash=" << b.hash.substr(0, 16) << "..."
                  << "  txs=" << b.data.transactions.size() << "\n";
    }
}

int Node::chainLength() const { return chain_.length(); }
