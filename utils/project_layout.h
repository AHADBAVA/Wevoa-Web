#pragma once

#include <filesystem>
#include <string>

namespace wevoaweb {

struct SourceProjectLayout {
    std::filesystem::path root;
    std::filesystem::path configFile;
    std::filesystem::path appDirectory;
    std::filesystem::path viewsDirectory;
    std::filesystem::path publicDirectory;
    std::filesystem::path packagesDirectory;
};

struct BuiltProjectLayout {
    std::filesystem::path root;
    std::filesystem::path buildRoot;
    std::filesystem::path configFile;
    std::filesystem::path appDirectory;
    std::filesystem::path viewsDirectory;
    std::filesystem::path publicDirectory;
    std::filesystem::path packagesDirectory;
    std::filesystem::path manifestFile;
};

SourceProjectLayout detectSourceProjectLayout(const std::filesystem::path& root,
                                              const std::string& appDirectory,
                                              const std::string& viewsDirectory,
                                              const std::string& publicDirectory);

BuiltProjectLayout detectBuiltProjectLayout(const std::filesystem::path& root, const std::string& buildDirectory);

}  // namespace wevoaweb
