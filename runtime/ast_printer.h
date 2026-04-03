#pragma once

#include <string>
#include <vector>

#include "ast/ast.h"

namespace wevoaweb {

class AstPrinter {
  public:
    std::string printProgram(const std::vector<std::unique_ptr<Stmt>>& statements) const;

  private:
    void printStmt(const Stmt& stmt, int indent, std::string& output) const;
    void printExpr(const Expr& expr, int indent, std::string& output) const;
    static void appendLine(std::string& output, int indent, const std::string& text);
};

}  // namespace wevoaweb
