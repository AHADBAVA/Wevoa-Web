#pragma once

#include <cstddef>
#include <string>

namespace wevoaweb {

std::string sha256Hex(const std::string& value);
std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& password, const std::string& encodedHash);
std::string generateSecureToken(std::size_t byteCount = 32);

}  // namespace wevoaweb
