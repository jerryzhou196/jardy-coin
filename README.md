# jardy-coin 🪙

a peer-to-peer blockchain built from scratch in c++. no libraries, no frameworks, no shortcuts. just raw tcp sockets, sha-256 proof-of-work, and vibes.

---

## what is this

jardy-coin is a fully functional distributed ledger that runs across multiple machines on a local network. every node mines, every node validates, and the network converges on a single canonical chain through nakamoto consensus (longest chain wins, no exceptions, no feelings).

it was built layer by layer — block → chain → json serialization → tcp sync → mesh network → transactions — so every piece is understood before the next one is added.

---

## how it works

**proof of work** — each block requires a sha-256 hash with `n` leading zeros. the miner increments a nonce until it finds one. difficulty is the rate limiter.

**longest chain consensus** — when two nodes disagree, whoever has more blocks wins. ties go to the incumbent. forks resolve themselves within one block.

**mining interruption** — if a longer chain arrives mid-mine, an `std::atomic<bool>` flag stops the current pow immediately and restarts on the new tip. no wasted blocks appended to a stale fork.

**mesh networking** — each node is both a tcp server and client simultaneously. new blocks broadcast to all peers, who broadcast to theirs. chains propagate in seconds.

**balance tracking** — balances are computed by scanning the full chain on every query. no utxo set, no shortcuts. coinbase transactions (mined rewards) are the only source of new coins.

---

## run it

**docker (easiest):**
```bash
# local 3-node mesh
docker compose up --build

# lan — run on each machine
docker run -p 8333:8333 jardy-coin --port 8333 --peer <other-ip>:8333
```

**from source:**
```bash
cmake -S . -B build -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3
cmake --build build
./build/blockchain --port 8333
./build/blockchain --port 8334 --peer 127.0.0.1:8333
```

---

## cli commands

```
tx <from> <to> <amount>   send coins (validated against chain balance)
balance                   show all balances
balance <address>         show one address
chain                     print the full local chain
help                      you're looking at it
```

---

## stack

- **c++17** — raw posix sockets, std::thread, std::atomic
- **openssl** — sha-256 only
- **nlohmann/json** — chain serialization over tcp
- **cmake** — build system
- **docker** — multi-stage debian:bookworm-slim image

---

*named after its creator. not financial advice.*
