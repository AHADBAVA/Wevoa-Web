#pragma once

#include <filesystem>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "runtime/session.h"

namespace wevoaweb::server {

// WebApplication loads all .wev files from the views directory into one
// runtime session and exposes the resulting routes to the HTTP server.
class WebApplication {
  public:
    struct StaticAsset {
        std::string contentType;
        std::string body;
    };

    WebApplication(std::string viewsDirectory,
                   std::string publicDirectory,
                   std::istream& input,
                   std::ostream& output,
                   bool debugAst = false);

    void loadViews();
    bool hasRoute(const std::string& path) const;
    std::string render(const std::string& path);
    std::optional<StaticAsset> loadStaticAsset(const std::string& path) const;
    std::vector<std::string> routePaths() const;
    const std::string& viewsDirectory() const;
    const std::string& publicDirectory() const;

  private:
    static std::string contentTypeForPath(const std::filesystem::path& path);
    std::optional<std::filesystem::path> resolveStaticPath(const std::string& requestPath) const;

    std::string viewsDirectory_;
    std::string publicDirectory_;
    RuntimeSession session_;
};

}  // namespace wevoaweb::server
