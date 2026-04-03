#pragma once

#include <istream>
#include <ostream>
#include <memory>
#include <string>
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
    void runSource(const std::string& sourceName, const std::string& source);
    void runFile(const std::string& path);
    bool hasRoute(const std::string& path) const;
    std::string renderRoute(const std::string& path);
    std::vector<std::string> routePaths() const;

    Interpreter& interpreter();

  private:
    bool debugAst_ = false;
    Interpreter interpreter_;
    std::vector<std::shared_ptr<Program>> programs_;
};

}  // namespace wevoaweb
