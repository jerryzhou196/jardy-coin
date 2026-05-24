#include "chain.hpp"
#include "serialization.hpp"

#include <fstream>
#include <iostream>

int main() {
    constexpr int DIFFICULTY = 4;

    // -----------------------------------------------------------------------
    // Build a small chain on "Node A".
    // -----------------------------------------------------------------------
    Blockchain nodeA(DIFFICULTY);
    nodeA.addBlock({{ Transaction{"Alice", "Bob",     500, 1, 1716500100, "sig1"} }});
    nodeA.addBlock({{ Transaction{"Bob",   "Charlie", 200, 1, 1716500200, "sig2"} }});

    std::cout << "=== Node A chain (" << nodeA.length() << " blocks) ===\n";
    for (const Block& b : nodeA.blocks()) {
        std::cout << "  [" << b.height << "] " << b.hash.substr(0, 20) << "...\n";
    }

    // -----------------------------------------------------------------------
    // Serialize to a JSON string — this is exactly what will travel over the
    // TCP socket in Layer 4.  The whole chain becomes one UTF-8 string.
    // -----------------------------------------------------------------------
    json j = nodeA;
    std::string wire = j.dump(2);   // pretty-print with 2-space indent

    std::cout << "\n=== wire payload (first 300 chars) ===\n"
              << wire.substr(0, 300) << "...\n";

    // -----------------------------------------------------------------------
    // Write to disk so you can inspect it in the editor.
    // -----------------------------------------------------------------------
    {
        std::ofstream f("chain.json");
        f << wire;
    }
    std::cout << "\nWrote chain.json\n";

    // -----------------------------------------------------------------------
    // Deserialize on "Node B" — parse the string back into a Blockchain.
    // This is the receiving side of the sync handshake in Layer 4.
    // -----------------------------------------------------------------------
    Blockchain nodeB = blockchain_from_json(json::parse(wire));

    std::cout << "\n=== Node B (deserialized) (" << nodeB.length() << " blocks) ===\n";
    for (const Block& b : nodeB.blocks()) {
        std::cout << "  [" << b.height << "] " << b.hash.substr(0, 20) << "...\n";
    }

    // -----------------------------------------------------------------------
    // Verify: both chains should be valid and their hashes identical.
    // -----------------------------------------------------------------------
    bool aValid = nodeA.isValid();
    bool bValid = nodeB.isValid();
    bool match  = (json(nodeA).dump() == json(nodeB).dump());

    std::cout << "\nNode A valid : " << (aValid ? "YES" : "NO") << "\n";
    std::cout << "Node B valid : " << (bValid ? "YES" : "NO") << "\n";
    std::cout << "Chains match : " << (match  ? "YES" : "NO") << "\n";

    // -----------------------------------------------------------------------
    // Tamper the deserialized chain and confirm isValid() catches it.
    // -----------------------------------------------------------------------
    const_cast<Block&>(nodeB.blocks()[1]).data.transactions[0].amount = 9999;
    std::cout << "\nAfter tampering Node B block 1:\n";
    std::cout << "Node B valid : " << (nodeB.isValid() ? "YES" : "NO") << "\n";

    return 0;
}
