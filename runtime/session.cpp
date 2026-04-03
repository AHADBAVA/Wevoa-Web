#include "runtime/session.h"

#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "runtime/ast_printer.h"
#include "utils/error.h"

namespace wevoaweb {

namespace {

std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open file: " + path.string());
    }

    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::string extractLine(const std::string& source, int lineNumber) {
    std::istringstream stream(source);
    std::string line;
    for (int currentLine = 1; std::getline(stream, line); ++currentLine) {
        if (currentLine == lineNumber) {
            return line;
        }
    }

    return "";
}

std::string pointerLine(int column) {
    std::string line;
    if (column > 1) {
        line.append(static_cast<std::size_t>(column - 1), ' ');
    }
    line += '^';
    return line;
}

std::string formatDiagnostic(const std::string& sourceName,
                             const std::string& source,
                             const WevoaError& error) {
    std::ostringstream stream;
    stream << sourceName << ":" << formatSpan(error.span()) << ": " << error.what();

    const std::string snippet = extractLine(source, error.span().start.line);
    if (!snippet.empty()) {
        stream << "\n\n" << snippet << "\n" << pointerLine(error.span().start.column);
    }

    return stream.str();
}

std::filesystem::path normalizePath(const std::filesystem::path& path) {
    std::error_code error;
    const auto absolutePath = std::filesystem::absolute(path, error);
    if (error) {
        return path;
    }

    const auto canonicalPath = std::filesystem::weakly_canonical(absolutePath, error);
    return error ? absolutePath : canonicalPath;
}

}  // namespace

RuntimeSession::RuntimeSession(std::istream& input, std::ostream& output, bool debugAst)
    : debugAst_(debugAst), interpreter_(output, input) {
    interpreter_.setImportLoader([this](const std::string& path, const SourceSpan& span) { importFile(path, span); });
}

void RuntimeSession::setDebugAst(bool enabled) {
    debugAst_ = enabled;
}

void RuntimeSession::defineGlobal(std::string name, Value value, bool isConstant) {
    interpreter_.defineGlobal(std::move(name), std::move(value), isConstant);
}

void RuntimeSession::setCurrentRequest(Value request) {
    interpreter_.setCurrentRequest(std::move(request));
}

void RuntimeSession::clearCurrentRequest() {
    interpreter_.clearCurrentRequest();
}

void RuntimeSession::runSource(const std::string& sourceName, const std::string& source) {
    if (!sourceName.empty()) {
        sourceCache_[sourceName] = source;
    }

    try {
        Lexer lexer(source);
        const auto tokens = lexer.scanTokens();

        Parser parser(tokens, source);
        auto parsedProgram = parser.parse();
        auto program = std::make_shared<Program>(std::move(parsedProgram));

        if (debugAst_) {
            AstPrinter printer;
            interpreter_.output() << printer.printProgram(*program);
        }

        programs_.push_back(program);
        if (!sourceName.empty()) {
            sourceStack_.push_back(sourceName);
        }

        try {
            interpreter_.interpret(*program);
        } catch (...) {
            if (!sourceName.empty()) {
                sourceStack_.pop_back();
            }
            programs_.pop_back();
            throw;
        }

        if (!sourceName.empty()) {
            sourceStack_.pop_back();
        }
    } catch (const WevoaError& error) {
        const std::string label = sourceName.empty() ? "<memory>" : sourceName;
        throw std::runtime_error(formatDiagnostic(label, source, error));
    }
}

void RuntimeSession::runFile(const std::string& path) {
    const auto normalized = normalizePath(path);
    runSource(normalized.string(), readFileContents(normalized));
}

bool RuntimeSession::hasRoute(const std::string& path, const std::string& method) const {
    return interpreter_.hasRoute(path, method);
}

std::string RuntimeSession::renderRoute(const std::string& path, const std::string& method) {
    return interpreter_.renderRoute(path, method);
}

std::vector<std::string> RuntimeSession::routePaths() const {
    return interpreter_.routePaths();
}

Interpreter& RuntimeSession::interpreter() {
    return interpreter_;
}

void RuntimeSession::importFile(const std::string& path, const SourceSpan& span) {
    std::filesystem::path resolvedPath(path);

    if (resolvedPath.is_relative()) {
        if (!sourceStack_.empty()) {
            resolvedPath = std::filesystem::path(sourceStack_.back()).parent_path() / resolvedPath;
        } else {
            resolvedPath = std::filesystem::current_path() / resolvedPath;
        }
    }

    resolvedPath = normalizePath(resolvedPath);
    const std::string cacheKey = resolvedPath.string();
    if (!importedFiles_.insert(cacheKey).second) {
        return;
    }

    try {
        runSource(cacheKey, readFileContents(resolvedPath));
    } catch (const std::exception&) {
        importedFiles_.erase(cacheKey);
        throw;
    } catch (...) {
        importedFiles_.erase(cacheKey);
        throw RuntimeError("Import failed for '" + cacheKey + "'.", span);
    }
}

}  // namespace wevoaweb
