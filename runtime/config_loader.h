#pragma once

#include <filesystem>

#include "interpreter/value.h"

namespace wevoaweb {

Value loadConfigFile(const std::filesystem::path& path);

}  // namespace wevoaweb
