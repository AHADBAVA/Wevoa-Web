#pragma once

#include <filesystem>

#include "interpreter/value.h"

namespace wevoaweb {

Value loadConfigFile(const std::filesystem::path& path);
std::string serializeJson(const Value& value);

}  // namespace wevoaweb
