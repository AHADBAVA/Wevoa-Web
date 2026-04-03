#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "interpreter/value.h"
#include "utils/source.h"

namespace wevoaweb {

class Environment;
class Interpreter;

class TemplateEngine {
  public:
    explicit TemplateEngine(std::filesystem::path viewsRoot);

    std::string render(const std::string& templatePath,
                       const Value& data,
                       Interpreter& interpreter,
                       const SourceSpan& span) const;
    std::string renderInline(const std::string& source, Interpreter& interpreter) const;

    const std::filesystem::path& viewsRoot() const;

  private:
    struct ParsedTemplate {
        std::optional<std::filesystem::path> layoutPath;
        std::string body;
        std::unordered_map<std::string, std::string> sections;
    };

    ParsedTemplate parseTemplate(const std::filesystem::path& path) const;
    std::string renderTemplateFile(const std::filesystem::path& path,
                                   Interpreter& interpreter,
                                   std::shared_ptr<Environment> environment) const;
    std::string renderTemplateString(const std::string& source,
                                     Interpreter& interpreter,
                                     std::shared_ptr<Environment> environment) const;
    std::filesystem::path resolveTemplatePath(const std::string& templatePath) const;
    std::filesystem::path resolveRelativeTemplatePath(const std::filesystem::path& currentTemplate,
                                                      const std::filesystem::path& relativeTemplate) const;
    std::shared_ptr<Environment> makeTemplateEnvironment(const Value& data,
                                                         Interpreter& interpreter,
                                                         const SourceSpan& span) const;

    std::filesystem::path viewsRoot_;
};

}  // namespace wevoaweb
