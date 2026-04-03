#include "runtime/session.h"

#include <fstream>
#include <iterator>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "runtime/ast_printer.h"

namespace wevoaweb {

RuntimeSession::RuntimeSession(std::istream& input, std::ostream& output, bool debugAst)
    : debugAst_(debugAst), interpreter_(output, input) {}

namespace {

std::string readFileContents(const std::string& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open file: " + path);
    }

    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

}  // namespace

void RuntimeSession::setDebugAst(bool enabled) {
    debugAst_ = enabled;
}

void RuntimeSession::runSource(const std::string& /*sourceName*/, const std::string& source) {
    Lexer lexer(source);
    const auto tokens = lexer.scanTokens();

    Parser parser(tokens);
    auto parsedProgram = parser.parse();
    auto program = std::make_shared<Program>(std::move(parsedProgram));

    if (debugAst_) {
        AstPrinter printer;
        interpreter_.output() << printer.printProgram(*program);
    }

    programs_.push_back(program);
    try {
        interpreter_.interpret(*program);
    } catch (...) {
        programs_.pop_back();
        throw;
    }
}

void RuntimeSession::runFile(const std::string& path) {
    runSource(path, readFileContents(path));
}

bool RuntimeSession::hasRoute(const std::string& path) const {
    return interpreter_.hasRoute(path);
}

std::string RuntimeSession::renderRoute(const std::string& path) {
    return interpreter_.renderRoute(path);
}

std::vector<std::string> RuntimeSession::routePaths() const {
    return interpreter_.routePaths();
}

Interpreter& RuntimeSession::interpreter() {
    return interpreter_;
}

}  // namespace wevoaweb
