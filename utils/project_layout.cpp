#include "utils/project_layout.h"

#include <stdexcept>

namespace wevoaweb {

namespace {

void ensureDirectoryExists(const std::filesystem::path& path, const std::string& label) {
    std::error_code error;
    if (!std::filesystem::exists(path, error) || !std::filesystem::is_directory(path, error) || error) {
        throw std::runtime_error("Expected " + label + " directory at: " + path.string());
    }
}

}  // namespace

SourceProjectLayout detectSourceProjectLayout(const std::filesystem::path& root,
                                              const std::string& appDirectory,
                                              const std::string& viewsDirectory,
                                              const std::string& publicDirectory) {
    SourceProjectLayout layout {
        root,
        root / "wevoa.config.json",
        root / appDirectory,
        root / viewsDirectory,
        root / publicDirectory,
    };

    ensureDirectoryExists(layout.appDirectory, "app");
    ensureDirectoryExists(layout.viewsDirectory, "views");
    return layout;
}

BuiltProjectLayout detectBuiltProjectLayout(const std::filesystem::path& root, const std::string& buildDirectory) {
    BuiltProjectLayout layout {
        root,
        root / buildDirectory,
        root / buildDirectory / "wevoa.config.json",
        root / buildDirectory / "app",
        root / buildDirectory / "views",
        root / buildDirectory / "public",
        root / buildDirectory / "wevoa.build.json",
    };

    ensureDirectoryExists(layout.buildRoot, "build output");
    ensureDirectoryExists(layout.appDirectory, "built app");
    ensureDirectoryExists(layout.viewsDirectory, "built views");
    return layout;
}

}  // namespace wevoaweb
