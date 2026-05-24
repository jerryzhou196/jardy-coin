#pragma once
#include <string>

// Returns the hex-encoded SHA-256 digest of `input`.
std::string sha256(const std::string& input);
