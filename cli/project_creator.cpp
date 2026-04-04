#include "cli/project_creator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "utils/defaults.h"

namespace wevoaweb {

namespace {

using PlaceholderMap = std::unordered_map<std::string, std::string>;

std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to read template file: " + path.string());
    }

    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::string humanizeProjectName(const std::filesystem::path& projectPath) {
    const std::string rawName = projectPath.filename().string();
    if (rawName.empty()) {
        return "WevoaWeb App";
    }

    std::string displayName;
    displayName.reserve(rawName.size());

    bool capitalizeNext = true;
    for (const char ch : rawName) {
        if (ch == '-' || ch == '_' || ch == ' ') {
            if (!displayName.empty() && displayName.back() != ' ') {
                displayName.push_back(' ');
            }
            capitalizeNext = true;
            continue;
        }

        const auto unsignedChar = static_cast<unsigned char>(ch);
        displayName.push_back(static_cast<char>(capitalizeNext ? std::toupper(unsignedChar)
                                                               : std::tolower(unsignedChar)));
        capitalizeNext = false;
    }

    return displayName.empty() ? "WevoaWeb App" : displayName;
}

std::string replaceAll(std::string value, const std::string& needle, const std::string& replacement) {
    if (needle.empty()) {
        return value;
    }

    std::size_t start = 0;
    while ((start = value.find(needle, start)) != std::string::npos) {
        value.replace(start, needle.size(), replacement);
        start += replacement.size();
    }
    return value;
}

class PlaceholderProcessor {
  public:
    explicit PlaceholderProcessor(PlaceholderMap values) : values_(std::move(values)) {}

    std::string process(std::string content) const {
        for (const auto& [key, value] : values_) {
            content = replaceAll(std::move(content), "{{" + key + "}}", value);
        }
        return content;
    }

  private:
    PlaceholderMap values_;
};

class TemplateLoader {
  public:
    explicit TemplateLoader(std::filesystem::path root) : root_(std::move(root)) {}

    std::filesystem::path templatePath(const std::string& templateName) const {
        if (templateName.empty()) {
            throw std::runtime_error("Template name must not be empty.");
        }

        const auto path = root_ / templateName;
        std::error_code error;
        if (!std::filesystem::exists(path, error) || error || !std::filesystem::is_directory(path, error)) {
            throw std::runtime_error("Template not found: " + templateName);
        }

        return path;
    }

  private:
    std::filesystem::path root_;
};

class FileCopySystem {
  public:
    explicit FileCopySystem(const FileWriter& writer) : writer_(writer) {}

    void copyTemplate(const std::filesystem::path& sourceRoot,
                      const std::filesystem::path& destinationRoot,
                      const PlaceholderProcessor& processor) const {
        namespace fs = std::filesystem;

        std::error_code error;
        for (fs::recursive_directory_iterator iterator(sourceRoot, fs::directory_options::skip_permission_denied, error),
             end;
             iterator != end;
             iterator.increment(error)) {
            if (error) {
                throw std::runtime_error("Failed to read template directory: " + sourceRoot.string());
            }

            const auto relative = fs::relative(iterator->path(), sourceRoot, error);
            if (error) {
                throw std::runtime_error("Failed to resolve template path: " + iterator->path().string());
            }

            const auto destination = destinationRoot / relative;
            if (iterator->is_directory(error) && !error) {
                writer_.createDirectory(destination);
                continue;
            }

            if (!iterator->is_regular_file(error) || error) {
                continue;
            }

            writer_.writeTextFile(destination, processor.process(readFileContents(iterator->path())));
        }
    }

  private:
    const FileWriter& writer_;
};

class TemplateManager {
  public:
    TemplateManager(const FileWriter& writer, std::filesystem::path root)
        : loader_(std::move(root)), copier_(writer) {}

    void scaffoldProject(const std::string& templateName,
                         const std::filesystem::path& destinationRoot,
                         const PlaceholderMap& placeholders) const {
        const PlaceholderProcessor processor(placeholders);
        copier_.copyTemplate(loader_.templatePath(templateName), destinationRoot, processor);
    }

  private:
    TemplateLoader loader_;
    FileCopySystem copier_;
};

}  // namespace

ProjectCreator::ProjectCreator(std::filesystem::path runtimeRoot, FileWriter writer)
    : runtimeRoot_(runtimeRoot.empty() ? std::filesystem::current_path() : std::move(runtimeRoot)),
      writer_(std::move(writer)) {}

std::filesystem::path ProjectCreator::createProject(const std::string& templateName,
                                                    const std::string& projectName) const {
    const auto targetPath = resolveTargetPath(projectName);

    std::error_code error;
    if (std::filesystem::exists(targetPath, error)) {
        if (error) {
            throw std::runtime_error("Failed to inspect target path: " + targetPath.string());
        }

        throw std::runtime_error("Project directory already exists: " + targetPath.string());
    }

    writer_.createDirectory(targetPath);

    const auto templateRoot = resolveTemplateRoot();
    TemplateManager manager(writer_, templateRoot);

    PlaceholderMap placeholders;
    placeholders.insert_or_assign("APP_NAME", targetPath.filename().string());
    placeholders.insert_or_assign("PROJECT_TITLE", humanizeProjectName(targetPath));
    placeholders.insert_or_assign("PORT", std::to_string(kDefaultPort));

    manager.scaffoldProject(templateName.empty() ? "app" : templateName, targetPath, placeholders);
    return targetPath;
}

std::filesystem::path ProjectCreator::resolveTargetPath(const std::string& projectName) const {
    if (projectName.empty()) {
        throw std::runtime_error("Project name must not be empty.");
    }

    const std::filesystem::path projectPath(projectName);
    if (projectPath.has_root_path()) {
        throw std::runtime_error("Project name must be a relative folder name.");
    }

    return std::filesystem::current_path() / projectPath;
}

std::filesystem::path ProjectCreator::resolveTemplateRoot() const {
    const auto currentPathTemplates = std::filesystem::current_path() / "templates";
    std::error_code currentError;
    if (std::filesystem::exists(currentPathTemplates, currentError) &&
        std::filesystem::is_directory(currentPathTemplates, currentError) && !currentError) {
        return currentPathTemplates;
    }

    const auto runtimeTemplates = runtimeRoot_ / "templates";
    std::error_code runtimeError;
    if (std::filesystem::exists(runtimeTemplates, runtimeError) &&
        std::filesystem::is_directory(runtimeTemplates, runtimeError) && !runtimeError) {
        return runtimeTemplates;
    }

    throw std::runtime_error("Unable to locate starter templates. Expected a templates/ directory next to the runtime or in the current working directory.");
}

}  // namespace wevoaweb
