#pragma once

// ---------------------------------------------------------------------------
// JSON serialization for all blockchain types.
//
// nlohmann/json uses ADL for to_json/from_json: define the free functions
// in the same namespace as the type and the library finds them automatically.
//
// Exception: Block has no default constructor (it always requires data),
// so nlohmann's default vector deserialization can't construct a temporary.
// We use adl_serializer<Block> instead — its static from_json() returns
// Block by value, sidestepping the default-constructor requirement.
// ---------------------------------------------------------------------------

#include "block.hpp"
#include "chain.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- Transaction ---
// Simple aggregate: all fields serialized directly.

inline void to_json(json& j, const Transaction& t) {
    j = json{
        {"from",      t.from},
        {"to",        t.to},
        {"amount",    t.amount},
        {"fee",       t.fee},
        {"timestamp", t.timestamp},
        {"signature", t.signature},
    };
}

inline void from_json(const json& j, Transaction& t) {
    j.at("from")     .get_to(t.from);
    j.at("to")       .get_to(t.to);
    j.at("amount")   .get_to(t.amount);
    j.at("fee")      .get_to(t.fee);
    j.at("timestamp").get_to(t.timestamp);
    j.at("signature").get_to(t.signature);
}

// --- BlockData ---

inline void to_json(json& j, const BlockData& bd) {
    j = json{ {"transactions", bd.transactions} };
}

inline void from_json(const json& j, BlockData& bd) {
    j.at("transactions").get_to(bd.transactions);
}

// --- Block ---
// Defined as an adl_serializer specialization inside the nlohmann namespace
// because Block has no default constructor.  The static from_json() constructs
// directly via the restore constructor (no mining, no timestamp mutation).

namespace nlohmann {
    template<>
    struct adl_serializer<Block> {
        static void to_json(json& j, const Block& b) {
            j = json{
                {"height",       b.height},
                {"timestamp",    b.timestamp},
                {"pow",          b.pow},
                {"hash",         b.hash},
                {"previousHash", b.previousHash},
                {"data",         b.data},
            };
        }

        static Block from_json(const json& j) {
            return Block(
                j.at("data")        .get<BlockData>(),
                j.at("previousHash").get<std::string>(),
                j.at("height")      .get<int>(),
                j.at("timestamp")   .get<int64_t>(),
                j.at("pow")         .get<int>(),
                j.at("hash")        .get<std::string>()
            );
        }
    };
}

// --- Blockchain ---

inline void to_json(json& j, const Blockchain& bc) {
    j = json{
        {"difficulty", bc.difficulty()},
        {"blocks",     bc.blocks()},
    };
}

// Reconstruct a full Blockchain from JSON (e.g. received over the network).
// Blocks are loaded as-is — no mining. Call isValid() after to verify.
inline Blockchain blockchain_from_json(const json& j) {
    return Blockchain(
        j.at("blocks")    .get<std::vector<Block>>(),
        j.at("difficulty").get<int>()
    );
}
