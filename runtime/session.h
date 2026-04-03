#pragma once

#include <filesystem>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "interpreter/interpreter.h"

namespace wevoaweb {

using Program = std::vector<std::unique_ptr<Stmt>>;

// RuntimeSession owns parsed programs for the full session lifetime so
// functions and routes can safely retain pointers into the AST.
class RuntimeSession {
  public:
    RuntimeSession(std::istream& input, std::ostream& output, bool debugAst = false);

    void setDebugAst(bool enabled);
    void defineGlobal(std::string name, Value value, bool isConstant = true);
    void setCurrentRequest(Value request);
    void clearCurrentRequest();
    void runSource(const std::string& sourceName, const std::string& source);
    void runFile(const std::string& path);
    bool hasRoute(const std::string& path, const std::string& method = "GET") const;
    std::string renderRoute(const std::string& path, const std::string& method = "GET");
    std::vector<std::string> routePaths() const;

    Interpreter& interpreter();

  private:
    void importFile(const std::string& path, const SourceSpan& span);

    bool debugAst_ = false;
    Interpreter interpreter_;
    std::vector<std::shared_ptr<Program>> programs_;
    std::unordered_set<std::string> importedFiles_;
    std::vector<std::string> sourceStack_;
    std::unordered_map<std::string, std::string> sourceCache_;
};

}  // namespace wevoaweb
