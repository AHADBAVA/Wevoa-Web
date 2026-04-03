#include "runtime/template_engine.h"

#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "interpreter/environment.h"
#include "interpreter/interpreter.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "utils/error.h"

namespace wevoaweb {

namespace {

std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open template file: " + path.string());
    }

    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool startsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

std::optional<std::filesystem::path> parseExtendLine(const std::string& line) {
    const std::string trimmed = trim(line);
    if (!startsWith(trimmed, "extend ")) {
        return std::nullopt;
    }

    const auto firstQuote = trimmed.find('"');
    const auto lastQuote = trimmed.find_last_of('"');
    if (firstQuote == std::string::npos || lastQuote == std::string::npos || firstQuote == lastQuote) {
        throw std::runtime_error("Invalid extend directive: " + line);
    }

    return std::filesystem::path(trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1));
}

std::optional<std::string> parseSectionLine(const std::string& line) {
    const std::string trimmed = trim(line);
    if (!startsWith(trimmed, "section ")) {
        return std::nullopt;
    }

    if (trimmed.empty() || trimmed.back() != '{') {
        throw std::runtime_error("Invalid section directive: " + line);
    }

    std::string name = trim(trimmed.substr(8, trimmed.size() - 9));
    if (name.empty()) {
        throw std::runtime_error("Section name must not be empty.");
    }

    return name;
}

}  // namespace

TemplateEngine::TemplateEngine(std::filesystem::path viewsRoot) : viewsRoot_(std::move(viewsRoot)) {}

std::string TemplateEngine::render(const std::string& templatePath,
                                   const Value& data,
                                   Interpreter& interpreter,
                                   const SourceSpan& span) const {
    auto environment = makeTemplateEnvironment(data, interpreter, span);
    return renderTemplateFile(resolveTemplatePath(templatePath), interpreter, std::move(environment));
}

std::string TemplateEngine::renderInline(const std::string& source, Interpreter& interpreter) const {
    return renderTemplateString(source, interpreter, interpreter.currentEnvironment());
}

const std::filesystem::path& TemplateEngine::viewsRoot() const {
    return viewsRoot_;
}

TemplateEngine::ParsedTemplate TemplateEngine::parseTemplate(const std::filesystem::path& path) const {
    const std::string source = readFileContents(path);
    std::istringstream stream(source);

    ParsedTemplate parsed;
    std::ostringstream body;
    std::ostringstream sectionBody;
    std::string currentSection;
    bool firstNonEmptyProcessed = false;

    std::string line;
    while (std::getline(stream, line)) {
        if (!firstNonEmptyProcessed) {
            const std::string trimmed = trim(line);
            if (!trimmed.empty()) {
                firstNonEmptyProcessed = true;
                if (const auto extendPath = parseExtendLine(line); extendPath.has_value()) {
                    parsed.layoutPath = resolveRelativeTemplatePath(path, *extendPath);
                    continue;
                }
            }
        }

        if (currentSection.empty()) {
            if (const auto sectionName = parseSectionLine(line); sectionName.has_value()) {
                currentSection = *sectionName;
                sectionBody.str("");
                sectionBody.clear();
                continue;
            }

            body << line;
            if (!stream.eof()) {
                body << '\n';
            }
            continue;
        }

        if (trim(line) == "}") {
            parsed.sections.insert_or_assign(currentSection, sectionBody.str());
            currentSection.clear();
            sectionBody.str("");
            sectionBody.clear();
            continue;
        }

        sectionBody << line;
        if (!stream.eof()) {
            sectionBody << '\n';
        }
    }

    if (!currentSection.empty()) {
        throw std::runtime_error("Unclosed section '" + currentSection + "' in template " + path.string());
    }

    parsed.body = body.str();
    return parsed;
}

std::string TemplateEngine::renderTemplateFile(const std::filesystem::path& path,
                                               Interpreter& interpreter,
                                               std::shared_ptr<Environment> environment) const {
    const auto parsed = parseTemplate(path);

    if (parsed.layoutPath.has_value()) {
        auto layoutEnvironment = std::make_shared<Environment>(environment);
        if (!trim(parsed.body).empty() && parsed.sections.find("content") == parsed.sections.end()) {
            layoutEnvironment->define("content", Value(renderTemplateString(parsed.body, interpreter, environment)), true);
        }

        for (const auto& [name, sectionSource] : parsed.sections) {
            layoutEnvironment->define(name,
                                      Value(renderTemplateString(sectionSource, interpreter, environment)),
                                      true);
        }

        return renderTemplateFile(*parsed.layoutPath, interpreter, std::move(layoutEnvironment));
    }

    if (!parsed.sections.empty() && parsed.body.empty()) {
        const auto found = parsed.sections.find("content");
        if (found != parsed.sections.end()) {
            return renderTemplateString(found->second, interpreter, environment);
        }
    }

    return renderTemplateString(parsed.body, interpreter, environment);
}

std::string TemplateEngine::renderTemplateString(const std::string& source,
                                                 Interpreter& interpreter,
                                                 std::shared_ptr<Environment> environment) const {
    std::string output;
    std::size_t cursor = 0;

    while (cursor < source.size()) {
        const auto open = source.find("{{", cursor);
        if (open == std::string::npos) {
            output.append(source.substr(cursor));
            break;
        }

        output.append(source.substr(cursor, open - cursor));
        const auto close = source.find("}}", open + 2);
        if (close == std::string::npos) {
            throw std::runtime_error("Unclosed template expression.");
        }

        const std::string expressionSource = trim(source.substr(open + 2, close - open - 2));
        if (!expressionSource.empty()) {
            try {
                Lexer lexer(expressionSource);
                const auto tokens = lexer.scanTokens();
                Parser parser(tokens, expressionSource);
                auto expression = parser.parseExpressionOnly();
                output += interpreter.evaluateInEnvironment(*expression, environment).toString();
            } catch (const WevoaError& error) {
                throw std::runtime_error("Template expression '{{ " + expressionSource + " }}' failed: " +
                                         std::string(error.what()));
            }
        }

        cursor = close + 2;
    }

    return output;
}

std::filesystem::path TemplateEngine::resolveTemplatePath(const std::string& templatePath) const {
    const std::filesystem::path candidate = viewsRoot_ / templatePath;
    std::error_code error;
    const auto normalized = std::filesystem::weakly_canonical(candidate, error);
    return error ? candidate : normalized;
}

std::filesystem::path TemplateEngine::resolveRelativeTemplatePath(const std::filesystem::path& currentTemplate,
                                                                  const std::filesystem::path& relativeTemplate) const {
    const auto candidate = currentTemplate.parent_path() / relativeTemplate;
    std::error_code error;
    const auto normalized = std::filesystem::weakly_canonical(candidate, error);
    return error ? candidate : normalized;
}

std::shared_ptr<Environment> TemplateEngine::makeTemplateEnvironment(const Value& data,
                                                                     Interpreter& interpreter,
                                                                     const SourceSpan& span) const {
    auto environment = std::make_shared<Environment>(interpreter.currentEnvironment());

    if (data.isNil()) {
        return environment;
    }

    if (!data.isObject()) {
        throw RuntimeError("view() context must be an object.", span);
    }

    for (const auto& [name, value] : data.asObject()) {
        environment->define(name, value, true);
    }

    return environment;
}

}  // namespace wevoaweb
