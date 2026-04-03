#include "utils/file_writer.h"

#include <fstream>
#include <stdexcept>

namespace wevoaweb {

void FileWriter::createDirectory(const std::filesystem::path& path) const {
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        if (error) {
            throw std::runtime_error("Failed to inspect path: " + path.string());
        }
        if (!std::filesystem::is_directory(path, error) || error) {
            throw std::runtime_error("Expected a directory path but found a file: " + path.string());
        }
        return;
    }

    if (!std::filesystem::create_directories(path, error) || error) {
        throw std::runtime_error("Failed to create directory: " + path.string());
    }
}

void FileWriter::writeTextFile(const std::filesystem::path& path, const std::string& contents) const {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        createDirectory(parent);
    }

    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }

    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!stream) {
        throw std::runtime_error("Failed to write file: " + path.string());
    }
}

}  // namespace wevoaweb
