#pragma once

#include <filesystem>
#include <string>

namespace wevoaweb {

class FileWriter {
  public:
    void createDirectory(const std::filesystem::path& path) const;
    void writeTextFile(const std::filesystem::path& path, const std::string& contents) const;
};

}  // namespace wevoaweb
