#include "node.hpp"

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Layer 4 driver
//
// Run as a listener (no --peer flag):
//   ./blockchain --port 8333
//
// Run as a connector (with --peer):
//   ./blockchain --port 8334 --peer 127.0.0.1:8333
//
// The connecting node mines one extra block so we can observe the
// longest-chain rule fire on the listening side.
// ---------------------------------------------------------------------------

static void usage(const char* argv0) {
    std::cerr << "usage: " << argv0
              << " --port <port> [--peer <ip>:<port>]\n";
}

int main(int argc, char* argv[]) {
    int         port     = 0;
    std::string peerHost;
    int         peerPort = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--peer" && i + 1 < argc) {
            std::string peer = argv[++i];
            auto colon = peer.rfind(':');
            if (colon == std::string::npos) { usage(argv[0]); return 1; }
            peerHost = peer.substr(0, colon);
            peerPort = std::stoi(peer.substr(colon + 1));
        }
    }

    if (port == 0) { usage(argv[0]); return 1; }

    constexpr int DIFFICULTY = 3;   // lower for fast demo; bump to 4+ for real use
    Node node(DIFFICULTY);

    if (peerHost.empty()) {
        // -----------------------------------------------------------------------
        // Listener: mine 2 blocks, then wait for a peer to connect and sync.
        // -----------------------------------------------------------------------
        std::cout << "[main] mining initial blocks...\n";
        node.submitTransaction({"Alice", "Bob",     500, 1, 1716500100, "sig1"});
        node.submitTransaction({"Bob",   "Charlie", 200, 1, 1716500200, "sig2"});

        std::cout << "[main] chain before sync (" << node.chainLength() << " blocks):\n";
        node.printChain();

        node.listen(port);

        std::cout << "[main] chain after sync (" << node.chainLength() << " blocks):\n";
        node.printChain();

    } else {
        // -----------------------------------------------------------------------
        // Connector: mine 3 blocks (longer chain), connect, and sync.
        // The listener should adopt our chain because it's longer.
        // -----------------------------------------------------------------------
        std::cout << "[main] mining initial blocks...\n";
        node.submitTransaction({"Alice", "Bob",     500, 1, 1716500100, "sig1"});
        node.submitTransaction({"Bob",   "Charlie", 200, 1, 1716500200, "sig2"});
        node.submitTransaction({"Charlie", "Dave",  100, 1, 1716500300, "sig3"});

        std::cout << "[main] chain before sync (" << node.chainLength() << " blocks):\n";
        node.printChain();

        node.connect(peerHost, peerPort);

        std::cout << "[main] chain after sync (" << node.chainLength() << " blocks):\n";
        node.printChain();
    }

    return 0;
}
