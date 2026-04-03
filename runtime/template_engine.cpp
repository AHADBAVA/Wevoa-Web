#include "runtime/template_engine.h"

#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "interpreter/environment.h"
#include "interpreter/interpreter.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "utils/error.h"

namespace wevoaweb {

namespace {

struct TemplateNode {
    enum class Type {
        Text,
        Expression,
        ForBlock,
        IfBlock,
    };

    Type type = Type::Text;
    std::string value;
    std::string secondaryValue;
    std::vector<TemplateNode> children;
    std::vector<TemplateNode> elseChildren;
};

struct ParseResult {
    std::vector<TemplateNode> nodes;
    std::optional<std::string> terminator;
};

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

Value evaluateTemplateExpressionValue(const std::string& expressionSource,
                                     Interpreter& interpreter,
                                     const std::shared_ptr<Environment>& environment) {
    try {
        Lexer lexer(expressionSource);
        const auto tokens = lexer.scanTokens();
        Parser parser(tokens, expressionSource);
        auto expression = parser.parseExpressionOnly();
        return interpreter.evaluateInEnvironment(*expression, environment);
    } catch (const WevoaError& error) {
        throw std::runtime_error("Template expression '{{ " + expressionSource + " }}' failed: " +
                                 std::string(error.what()));
    }
}

std::size_t findNextTemplateMarker(const std::string& source, std::size_t cursor) {
    const auto expressionPos = source.find("{{", cursor);
    const auto directivePos = source.find("{%", cursor);

    if (expressionPos == std::string::npos) {
        return directivePos;
    }

    if (directivePos == std::string::npos) {
        return expressionPos;
    }

    return std::min(expressionPos, directivePos);
}

bool matchesAnyTerminator(const std::string& directive, const std::vector<std::string>& terminators) {
    for (const auto& terminator : terminators) {
        if (directive == terminator) {
            return true;
        }
    }

    return false;
}

bool isIdentifierStart(char ch) {
    const auto unsignedChar = static_cast<unsigned char>(ch);
    return std::isalpha(unsignedChar) || ch == '_';
}

bool isIdentifierContinue(char ch) {
    const auto unsignedChar = static_cast<unsigned char>(ch);
    return std::isalnum(unsignedChar) || ch == '_';
}

TemplateNode makeTextNode(std::string text) {
    TemplateNode node;
    node.type = TemplateNode::Type::Text;
    node.value = std::move(text);
    return node;
}

TemplateNode makeExpressionNode(std::string expression) {
    TemplateNode node;
    node.type = TemplateNode::Type::Expression;
    node.value = std::move(expression);
    return node;
}

TemplateNode makeForNode(std::string variableName,
                         std::string iterableExpression,
                         std::vector<TemplateNode> children) {
    TemplateNode node;
    node.type = TemplateNode::Type::ForBlock;
    node.value = std::move(variableName);
    node.secondaryValue = std::move(iterableExpression);
    node.children = std::move(children);
    return node;
}

TemplateNode makeIfNode(std::string conditionExpression,
                        std::vector<TemplateNode> children,
                        std::vector<TemplateNode> elseChildren) {
    TemplateNode node;
    node.type = TemplateNode::Type::IfBlock;
    node.value = std::move(conditionExpression);
    node.children = std::move(children);
    node.elseChildren = std::move(elseChildren);
    return node;
}

std::pair<std::string, std::string> parseForDirective(const std::string& directive) {
    const std::string remainder = trim(directive.substr(4));
    if (remainder.empty()) {
        throw std::runtime_error("Invalid for directive: " + directive);
    }

    std::size_t cursor = 0;
    if (!isIdentifierStart(remainder[cursor])) {
        throw std::runtime_error("Invalid loop variable in directive: " + directive);
    }

    ++cursor;
    while (cursor < remainder.size() && isIdentifierContinue(remainder[cursor])) {
        ++cursor;
    }

    const std::string variableName = remainder.substr(0, cursor);
    while (cursor < remainder.size() && std::isspace(static_cast<unsigned char>(remainder[cursor]))) {
        ++cursor;
    }

    if (cursor + 1 >= remainder.size() || remainder.substr(cursor, 2) != "in") {
        throw std::runtime_error("For directive must use 'in': " + directive);
    }

    cursor += 2;
    while (cursor < remainder.size() && std::isspace(static_cast<unsigned char>(remainder[cursor]))) {
        ++cursor;
    }

    const std::string iterableExpression = trim(remainder.substr(cursor));
    if (iterableExpression.empty()) {
        throw std::runtime_error("For directive iterable expression must not be empty.");
    }

    return {variableName, iterableExpression};
}

ParseResult parseTemplateNodes(const std::string& source,
                               std::size_t& cursor,
                               const std::vector<std::string>& terminators) {
    ParseResult result;

    while (cursor < source.size()) {
        const auto marker = findNextTemplateMarker(source, cursor);
        if (marker == std::string::npos) {
            if (cursor < source.size()) {
                result.nodes.push_back(makeTextNode(source.substr(cursor)));
            }
            cursor = source.size();
            return result;
        }

        if (marker > cursor) {
            result.nodes.push_back(makeTextNode(source.substr(cursor, marker - cursor)));
        }

        if (source.compare(marker, 2, "{{") == 0) {
            const auto close = source.find("}}", marker + 2);
            if (close == std::string::npos) {
                throw std::runtime_error("Unclosed template expression.");
            }

            const std::string expressionSource = trim(source.substr(marker + 2, close - marker - 2));
            result.nodes.push_back(makeExpressionNode(expressionSource));
            cursor = close + 2;
            continue;
        }

        const auto close = source.find("%}", marker + 2);
        if (close == std::string::npos) {
            throw std::runtime_error("Unclosed template directive.");
        }

        const std::string directive = trim(source.substr(marker + 2, close - marker - 2));
        cursor = close + 2;

        if (matchesAnyTerminator(directive, terminators)) {
            result.terminator = directive;
            return result;
        }

        if (startsWith(directive, "for ")) {
            const auto [variableName, iterableExpression] = parseForDirective(directive);
            auto body = parseTemplateNodes(source, cursor, {"end"});
            if (!body.terminator.has_value() || *body.terminator != "end") {
                throw std::runtime_error("For directive must be closed with {% end %}.");
            }

            result.nodes.push_back(makeForNode(variableName, iterableExpression, std::move(body.nodes)));
            continue;
        }

        if (startsWith(directive, "if ")) {
            const std::string conditionExpression = trim(directive.substr(3));
            if (conditionExpression.empty()) {
                throw std::runtime_error("If directive condition must not be empty.");
            }

            auto thenBranch = parseTemplateNodes(source, cursor, {"else", "end"});
            std::vector<TemplateNode> elseBranch;
            if (thenBranch.terminator == std::optional<std::string> {"else"}) {
                auto elseResult = parseTemplateNodes(source, cursor, {"end"});
                if (!elseResult.terminator.has_value() || *elseResult.terminator != "end") {
                    throw std::runtime_error("If directive else branch must be closed with {% end %}.");
                }
                elseBranch = std::move(elseResult.nodes);
            } else if (!thenBranch.terminator.has_value() || *thenBranch.terminator != "end") {
                throw std::runtime_error("If directive must be closed with {% end %}.");
            }

            result.nodes.push_back(
                makeIfNode(conditionExpression, std::move(thenBranch.nodes), std::move(elseBranch)));
            continue;
        }

        if (directive == "else" || directive == "end") {
            throw std::runtime_error("Unexpected template directive {% " + directive + " %}.");
        }

        throw std::runtime_error("Unknown template directive {% " + directive + " %}.");
    }

    return result;
}

std::string renderNodes(const std::vector<TemplateNode>& nodes,
                        Interpreter& interpreter,
                        const std::shared_ptr<Environment>& environment) {
    std::string output;

    for (const auto& node : nodes) {
        switch (node.type) {
            case TemplateNode::Type::Text:
                output += node.value;
                break;

            case TemplateNode::Type::Expression:
                if (!node.value.empty()) {
                    output += evaluateTemplateExpressionValue(node.value, interpreter, environment).toString();
                }
                break;

            case TemplateNode::Type::ForBlock: {
                const Value iterable = evaluateTemplateExpressionValue(node.secondaryValue, interpreter, environment);

                if (iterable.isNil()) {
                    break;
                }

                if (iterable.isArray()) {
                    std::int64_t index = 0;
                    for (const auto& item : iterable.asArray()) {
                        auto loopEnvironment = std::make_shared<Environment>(environment);
                        loopEnvironment->define(node.value, item, true);
                        loopEnvironment->define(node.value + "_index", Value(index), true);
                        output += renderNodes(node.children, interpreter, loopEnvironment);
                        ++index;
                    }
                    break;
                }

                if (iterable.isObject()) {
                    for (const auto& [key, item] : iterable.asObject()) {
                        auto loopEnvironment = std::make_shared<Environment>(environment);
                        loopEnvironment->define(node.value, item, true);
                        loopEnvironment->define(node.value + "_key", Value(key), true);
                        output += renderNodes(node.children, interpreter, loopEnvironment);
                    }
                    break;
                }

                throw std::runtime_error("Template for-loop expression must evaluate to an array, object, or nil.");
            }

            case TemplateNode::Type::IfBlock: {
                const Value condition = evaluateTemplateExpressionValue(node.value, interpreter, environment);
                if (condition.isTruthy()) {
                    output += renderNodes(node.children, interpreter, environment);
                } else {
                    output += renderNodes(node.elseChildren, interpreter, environment);
                }
                break;
            }
        }
    }

    return output;
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
    std::size_t cursor = 0;
    auto parsed = parseTemplateNodes(source, cursor, {});
    if (parsed.terminator.has_value()) {
        throw std::runtime_error("Unexpected template terminator {% " + *parsed.terminator + " %}.");
    }

    return renderNodes(parsed.nodes, interpreter, environment);
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
