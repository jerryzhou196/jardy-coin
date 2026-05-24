#include "crypto.hpp"

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

std::string sha256(const std::string& input) {
    unsigned char hashBytes[SHA256_DIGEST_LENGTH];

    SHA256(
        reinterpret_cast<const unsigned char*>(input.c_str()),
        input.size(),
        hashBytes
    );

    std::ostringstream oss;
    for (unsigned char byte : hashBytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    return oss.str();
}
