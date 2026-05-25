#include "network_manager.hpp"
#include "serialization.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <thread>

// ---------------------------------------------------------------------------
// Framing — identical to Layer 4: one JSON object per newline.
// ---------------------------------------------------------------------------

std::string NetworkManager::readLine(int fd) {
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

void NetworkManager::writeLine(int fd, const std::string& s) {
    std::string msg = s + "\n";
    size_t sent = 0;
    while (sent < msg.size()) {
        ssize_t n = write(fd, msg.c_str() + sent, msg.size() - sent);
        if (n <= 0) throw std::runtime_error("write failed");
        sent += static_cast<size_t>(n);
    }
}

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

NetworkManager::NetworkManager(int difficulty, int listenPort, bool verbose)
    : chain_(difficulty)
    , listenPort_(listenPort)
    , verbose_(verbose)
{}

NetworkManager::~NetworkManager() {
    stop();
}

// ---------------------------------------------------------------------------
// run / stop
// ---------------------------------------------------------------------------

void NetworkManager::run() {
    // Open the server socket.
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(listenPort_);

    if (bind(serverFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (::listen(serverFd_, 8) < 0)
        throw std::runtime_error("listen() failed");

    running_ = true;
    std::cout << "[net] listening on port " << listenPort_ << std::endl;

    acceptLoop();
}

void NetworkManager::stop() {
    running_ = false;
    if (serverFd_ >= 0) {
        close(serverFd_);
        serverFd_ = -1;
    }
}

// ---------------------------------------------------------------------------
// Accept loop — runs on acceptThread_
// ---------------------------------------------------------------------------

void NetworkManager::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t   clientLen = sizeof(clientAddr);

        int clientFd = accept(serverFd_,
                              reinterpret_cast<sockaddr*>(&clientAddr),
                              &clientLen);
        if (clientFd < 0) {
            if (running_) std::cerr << "[net] accept() error\n";
            break;
        }

        char peerIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, peerIp, sizeof(peerIp));
        std::cout << "[net] inbound connection from " << peerIp << std::endl;

        // Handle each connection on its own thread so we never block the
        // accept loop while waiting for a slow peer's chain payload.
        std::thread([this, clientFd]() {
            bool adopted = exchangeChains(clientFd);
            close(clientFd);
            // If we got a better chain from this peer, push ours onward.
            if (adopted) broadcast();
        }).detach();
    }
}

// ---------------------------------------------------------------------------
// addPeer — dial out, sync, register the peer address for future broadcasts
// ---------------------------------------------------------------------------

void NetworkManager::addPeer(const std::string& host, int port) {
    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        peers_.push_back({host, port});
    }
    syncWithPeer(host, port);
}

void NetworkManager::syncWithPeer(const std::string& host, int port) {
    // Use getaddrinfo so both raw IPs and Docker/DNS hostnames are accepted.
    addrinfo hints{}, *res;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        std::cerr << "[net] could not resolve: " << host << std::endl;
        return;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); std::cerr << "[net] socket() failed\n"; return; }

    if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        std::cerr << "[net] could not connect to " << host << ":" << port << std::endl;
        close(fd);
        return;
    }

    freeaddrinfo(res);
    std::cout << "[net] connected to " << host << ":" << port << std::endl;
    exchangeChains(fd);
    close(fd);
}

// ---------------------------------------------------------------------------
// exchangeChains — the sync handshake (same logic as Layer 4's syncWith)
// ---------------------------------------------------------------------------

bool NetworkManager::exchangeChains(int fd) {
    // Send first, then read — prevents both sides blocking on each other.
    try {
        writeLine(fd, serializeChain());
    } catch (...) {
        std::cerr << "[net] send failed\n";
        return false;
    }

    std::string raw = readLine(fd);
    if (raw.empty()) return false;

    try {
        Blockchain peer = blockchain_from_json(json::parse(raw));
        int peerLen = peer.length();
        adoptIfBetter(std::move(peer));
        return chainLength() == peerLen;  // true if we switched to their chain
    } catch (const std::exception& e) {
        std::cerr << "[net] parse error: " << e.what() << std::endl;
        return false;
    }
}

// ---------------------------------------------------------------------------
// broadcast — push our current chain to every known peer
// ---------------------------------------------------------------------------

void NetworkManager::broadcast() {
    std::vector<std::pair<std::string,int>> snapshot;
    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        snapshot = peers_;
    }

    for (const auto& peer : snapshot) {
        // Each broadcast is a full sync — the peer applies the same
        // longest-chain rule and will adopt ours if it's better.
        std::thread([this, peer]() {
            syncWithPeer(peer.first, peer.second);
        }).detach();
    }
}

// ---------------------------------------------------------------------------
// submitTransaction — mine + broadcast
// ---------------------------------------------------------------------------

void NetworkManager::submitTransaction(Transaction tx) {
    // Mine outside the lock, but verify the tip hasn't changed before appending.
    // If it has (peer adoption arrived mid-mine), discard and retry on the new tip.
    while (true) {
        std::string prevHash;
        int nextHeight, difficulty;
        {
            std::lock_guard<std::mutex> lock(chainMutex_);
            const Block& tip = chain_.blocks().back();
            prevHash   = tip.hash;
            nextHeight = tip.height + 1;
            difficulty = chain_.difficulty();
        }

        stopMining = false;
        Block b(BlockData{{ tx }}, prevHash, nextHeight);
        b.mine(difficulty, stopMining);

        {
            std::lock_guard<std::mutex> lock(chainMutex_);
            if (chain_.blocks().back().hash == prevHash) {
                chain_.appendBlock(std::move(b));
                break;
            }
            std::cout << "[net] stale block discarded (chain changed during mining), retrying" << std::endl;
        }
    }
    std::cout << "[net] mined block " << chainLength() << std::endl;
    broadcast();
}

bool NetworkManager::submitManualTransaction(Transaction tx) {
    {
        std::lock_guard<std::mutex> lock(chainMutex_);
        int64_t balance = chain_.getBalance(tx.from);
        int64_t cost    = tx.amount + tx.fee;
        if (balance < cost) {
            std::cerr << "[cli] rejected: " << tx.from
                      << " has balance " << balance
                      << ", needs " << cost << "\n";
            return false;
        }
    }
    submitTransaction(std::move(tx));
    return true;
}

int64_t NetworkManager::getBalance(const std::string& address) const {
    std::lock_guard<std::mutex> lock(chainMutex_);
    return chain_.getBalance(address);
}

std::map<std::string, int64_t> NetworkManager::allBalances() const {
    std::lock_guard<std::mutex> lock(chainMutex_);
    return chain_.allBalances();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string NetworkManager::serializeChain() const {
    std::lock_guard<std::mutex> lock(chainMutex_);
    return json(chain_).dump();
}

void NetworkManager::adoptIfBetter(Blockchain incoming) {
    std::lock_guard<std::mutex> lock(chainMutex_);
    if (verbose_) {
        std::cout << "[net] -- local  --\n" << chain_   << std::endl;
        std::cout << "[net] -- peer   --\n" << incoming << std::endl;
    }
    if (incoming.isValid() && incoming.length() > chain_.length()) {
        stopMining = true;
        std::cout << "[net] adopted peer chain (" << incoming.length()
                  << " > " << chain_.length() << ")" << std::endl;
        chain_ = std::move(incoming);
    } else {
        std::cout << "[net] kept own chain (" << chain_.length()
                  << " >= " << incoming.length() << ")" << std::endl;
    }
}

void NetworkManager::printChain() const {
    std::lock_guard<std::mutex> lock(chainMutex_);
    std::cout << chain_;
}

int NetworkManager::chainLength() const {
    std::lock_guard<std::mutex> lock(chainMutex_);
    return chain_.length();
}
