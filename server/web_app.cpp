#include "server/web_app.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace wevoaweb::server {

namespace {

std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open view file: " + path.string());
    }

    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

bool shouldExecuteAsRouteScript(const std::string& contents) {
    return contents.find("route") != std::string::npos;
}

}  // namespace

WebApplication::WebApplication(std::string viewsDirectory,
                               std::string publicDirectory,
                               std::istream& input,
                               std::ostream& output,
                               bool debugAst)
    : viewsDirectory_(std::move(viewsDirectory)),
      publicDirectory_(std::move(publicDirectory)),
      session_(input, output, debugAst) {}

void WebApplication::loadViews() {
    namespace fs = std::filesystem;

    const fs::path root(viewsDirectory_);
    if (!fs::exists(root) || !fs::is_directory(root)) {
        throw std::runtime_error("Views directory not found: " + viewsDirectory_);
    }

    std::vector<fs::path> viewFiles;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file() && entry.path().extension() == ".wev") {
            viewFiles.push_back(entry.path());
        }
    }

    std::sort(viewFiles.begin(), viewFiles.end());

    for (const auto& viewFile : viewFiles) {
        const std::string contents = readFileContents(viewFile);
        if (!shouldExecuteAsRouteScript(contents)) {
            continue;
        }

        session_.runSource(viewFile.string(), contents);
    }

    if (viewFiles.empty()) {
        throw std::runtime_error("No .wev view files were found in: " + viewsDirectory_);
    }

    if (session_.routePaths().empty()) {
        throw std::runtime_error("No routes were registered from views in: " + viewsDirectory_);
    }
}

bool WebApplication::hasRoute(const std::string& path) const {
    return session_.hasRoute(path);
}

std::string WebApplication::render(const std::string& path) {
    return session_.renderRoute(path);
}

std::optional<WebApplication::StaticAsset> WebApplication::loadStaticAsset(const std::string& path) const {
    const auto assetPath = resolveStaticPath(path);
    if (!assetPath.has_value()) {
        return std::nullopt;
    }

    std::error_code error;
    if (!std::filesystem::exists(*assetPath, error) || !std::filesystem::is_regular_file(*assetPath, error) || error) {
        return std::nullopt;
    }

    return StaticAsset {contentTypeForPath(*assetPath), readFileContents(*assetPath)};
}

std::vector<std::string> WebApplication::routePaths() const {
    return session_.routePaths();
}

const std::string& WebApplication::viewsDirectory() const {
    return viewsDirectory_;
}

const std::string& WebApplication::publicDirectory() const {
    return publicDirectory_;
}

std::string WebApplication::contentTypeForPath(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();

    if (extension == ".css") {
        return "text/css; charset=utf-8";
    }
    if (extension == ".html") {
        return "text/html; charset=utf-8";
    }
    if (extension == ".txt") {
        return "text/plain; charset=utf-8";
    }
    if (extension == ".svg") {
        return "image/svg+xml";
    }
    if (extension == ".png") {
        return "image/png";
    }
    if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    }
    if (extension == ".ico") {
        return "image/x-icon";
    }

    return "application/octet-stream";
}

std::optional<std::filesystem::path> WebApplication::resolveStaticPath(const std::string& requestPath) const {
    if (requestPath.empty() || requestPath == "/") {
        return std::nullopt;
    }

    std::filesystem::path relativePath(requestPath.substr(1));
    relativePath = relativePath.lexically_normal();

    if (relativePath.empty() || relativePath.is_absolute()) {
        return std::nullopt;
    }

    for (const auto& part : relativePath) {
        if (part == "..") {
            return std::nullopt;
        }
    }

    return std::filesystem::path(publicDirectory_) / relativePath;
}

}  // namespace wevoaweb::server
