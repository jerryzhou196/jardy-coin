#include "network_manager.hpp"

#include <csignal>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// ---------------------------------------------------------------------------
// Layer 6 driver
//
// Node A (listener, no --peer):
//   ./blockchain --port 8333
//
// Node B (connects to A):
//   ./blockchain --port 8334 --peer 127.0.0.1:8333
//
// Node C (connects to A and B):
//   ./blockchain --port 8335 --peer 127.0.0.1:8333 --peer 127.0.0.1:8334
//
// Interactive commands (type while the node is running):
//   tx <from> <to> <amount>   — validated transaction (sender must have funds)
//   balance <address>         — show address balance
//   chain                     — print all blocks in the local chain
//   help                      — show this list
// ---------------------------------------------------------------------------

static NetworkManager* g_net = nullptr;

static void sigHandler(int) {
    std::cout << "\n[main] shutting down" << std::endl;
    if (g_net) g_net->stop();
}

static void usage(const char* argv0) {
    std::cerr << "usage: " << argv0
              << " --port <port> [--peer <ip>:<port>] ... [--quiet]\n";
}

static void printHelp() {
    std::cout << "commands:\n"
              << "  tx <from> <to> <amount>  validated transaction\n"
              << "  balance [address]        show all balances, or one address\n"
              << "  chain                    print local chain\n"
              << "  help                     show this list\n";
}

static void cliLoop(NetworkManager& net) {
    printHelp();
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "tx") {
            std::string from, to;
            int64_t amount = 0;
            if (!(ss >> from >> to >> amount)) {
                std::cerr << "usage: tx <from> <to> <amount>\n";
                continue;
            }
            Transaction tx{
                from, to, amount,
                /*fee=*/0,
                static_cast<int64_t>(std::time(nullptr)),
                /*signature=*/"cli"
            };
            if (net.submitManualTransaction(std::move(tx))) {
                std::cout << "[cli] chain (" << net.chainLength() << " blocks):\n";
                net.printChain();
            }
        } else if (cmd == "balance") {
            std::string addr;
            if (ss >> addr) {
                std::cout << "[cli] " << addr << " = " << net.getBalance(addr) << "\n";
            } else {
                auto balances = net.allBalances();
                if (balances.empty()) { std::cout << "[cli] no balances yet\n"; continue; }
                for (const auto& kv : balances)
                    std::cout << "  " << kv.first << " = " << kv.second << "\n";
            }
        } else if (cmd == "chain") {
            std::cout << "[cli] chain (" << net.chainLength() << " blocks):\n";
            net.printChain();
        } else if (cmd == "help") {
            printHelp();
        } else {
            std::cerr << "unknown command: " << cmd << "  (type 'help')\n";
        }
    }
}

int main(int argc, char* argv[]) {
    int  port    = 0;
    bool verbose = true;
    std::vector<std::pair<std::string,int>> peers;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--peer" && i + 1 < argc) {
            std::string peer = argv[++i];
            auto colon = peer.rfind(':');
            if (colon == std::string::npos) { usage(argv[0]); return 1; }
            peers.push_back({
                peer.substr(0, colon),
                std::stoi(peer.substr(colon + 1))
            });
        } else if (arg == "--quiet") {
            verbose = false;
        }
    }

    if (port == 0) { usage(argv[0]); return 1; }

    constexpr int DIFFICULTY = 5;
    NetworkManager net(DIFFICULTY, port, verbose);
    g_net = &net;

    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    for (const auto& peer : peers) {
        net.addPeer(peer.first, peer.second);
    }


    // Continuously mine coinbase blocks — difficulty is the rate limiter.
    std::thread miner([&net, port, verbose]() {
        const std::string minerAddr = "node-" + std::to_string(port);
        while (true) {
            Transaction coinbase{
                "network", minerAddr,
                /*amount=*/50, /*fee=*/0,
                static_cast<int64_t>(std::time(nullptr)),
                "coinbase"
            };
            net.submitTransaction(std::move(coinbase));
            if (verbose)
                std::cout << "[miner] block " << net.chainLength()
                          << "  balance=" << net.getBalance(minerAddr) << std::endl;
        }
    });
    miner.detach();

    std::thread cli([&net]() { cliLoop(net); });
    cli.detach();

    try {
        net.run();
    } catch (const std::exception& e) {
        std::cerr << "[main] fatal: " << e.what() << "\n"
                  << "       Is port " << port << " already in use?\n"
                  << "       Run: lsof -i :" << port << "\n";
        return 1;
    }

    return 0;
}
