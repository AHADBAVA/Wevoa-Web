#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "utils/file_writer.h"

namespace wevoaweb {

struct InstalledPackageInfo {
    std::string packageName;
    std::string version;
    std::string source;
    std::string description;
    std::string repo;
    std::string usageSnippet;
    std::vector<std::string> dependencies;
    bool isOfficial = false;
    std::filesystem::path path;
};

struct PackageInstallResult {
    std::string packageName;
    std::string version;
    std::filesystem::path destination;
    std::filesystem::path cachePath;
    bool createdPlaceholder = false;
    bool installedFromCore = false;
    std::vector<std::string> installedDependencies;
    std::vector<std::string> warnings;
};

struct PackageRemoveResult {
    std::string packageName;
    std::filesystem::path destination;
};

class PackageManager {
  public:
    explicit PackageManager(FileWriter writer = {});

    PackageInstallResult install(const std::string& packageSpec, bool globalMode = false) const;
    PackageRemoveResult remove(const std::string& packageName) const;
    std::vector<InstalledPackageInfo> listInstalled() const;
    std::vector<InstalledPackageInfo> listAvailableCore() const;
    std::vector<InstalledPackageInfo> search(const std::string& query) const;
    InstalledPackageInfo packageInfo(const std::string& packageName) const;

  private:
    std::string packageNameFromSpec(const std::string& packageSpec) const;
    void copyDirectoryContents(const std::filesystem::path& source, const std::filesystem::path& destination) const;

    FileWriter writer_;
};

}  // namespace wevoaweb
