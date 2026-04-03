#include "ast/ast.h"

#include <utility>

namespace wevoaweb {

Expr::Expr(SourceSpan span) : span(span) {}

Stmt::Stmt(SourceSpan span) : span(span) {}

LiteralExpr::LiteralExpr(Value value, SourceSpan span) : Expr(span), value(std::move(value)) {}

Value LiteralExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitLiteralExpr(*this);
}

VariableExpr::VariableExpr(Token name, SourceSpan span) : Expr(span), name(std::move(name)) {}

Value VariableExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitVariableExpr(*this);
}

AssignExpr::AssignExpr(Token name, std::unique_ptr<Expr> value, SourceSpan span)
    : Expr(span), name(std::move(name)), value(std::move(value)) {}

Value AssignExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitAssignExpr(*this);
}

UnaryExpr::UnaryExpr(Token op, std::unique_ptr<Expr> right, SourceSpan span)
    : Expr(span), op(std::move(op)), right(std::move(right)) {}

Value UnaryExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitUnaryExpr(*this);
}

BinaryExpr::BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right, SourceSpan span)
    : Expr(span), left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

Value BinaryExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitBinaryExpr(*this);
}

GroupingExpr::GroupingExpr(std::unique_ptr<Expr> expression, SourceSpan span)
    : Expr(span), expression(std::move(expression)) {}

Value GroupingExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitGroupingExpr(*this);
}

CallExpr::CallExpr(std::unique_ptr<Expr> callee,
                   Token paren,
                   std::vector<std::unique_ptr<Expr>> arguments,
                   SourceSpan span)
    : Expr(span), callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}

Value CallExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitCallExpr(*this);
}

ExpressionStmt::ExpressionStmt(std::unique_ptr<Expr> expression, SourceSpan span)
    : Stmt(span), expression(std::move(expression)) {}

void ExpressionStmt::accept(StmtVisitor& visitor) const {
    visitor.visitExpressionStmt(*this);
}

VarDeclStmt::VarDeclStmt(Token name, std::unique_ptr<Expr> initializer, bool isConstant, SourceSpan span)
    : Stmt(span), name(std::move(name)), initializer(std::move(initializer)), isConstant(isConstant) {}

void VarDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitVarDeclStmt(*this);
}

BlockStmt::BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, SourceSpan span)
    : Stmt(span), statements(std::move(statements)) {}

void BlockStmt::accept(StmtVisitor& visitor) const {
    visitor.visitBlockStmt(*this);
}

IfStmt::IfStmt(std::unique_ptr<Expr> condition,
               std::unique_ptr<Stmt> thenBranch,
               std::unique_ptr<Stmt> elseBranch,
               SourceSpan span)
    : Stmt(span),
      condition(std::move(condition)),
      thenBranch(std::move(thenBranch)),
      elseBranch(std::move(elseBranch)) {}

void IfStmt::accept(StmtVisitor& visitor) const {
    visitor.visitIfStmt(*this);
}

LoopStmt::LoopStmt(std::unique_ptr<Stmt> initializer,
                   std::unique_ptr<Expr> condition,
                   std::unique_ptr<Expr> increment,
                   std::unique_ptr<Stmt> body,
                   SourceSpan span)
    : Stmt(span),
      initializer(std::move(initializer)),
      condition(std::move(condition)),
      increment(std::move(increment)),
      body(std::move(body)) {}

void LoopStmt::accept(StmtVisitor& visitor) const {
    visitor.visitLoopStmt(*this);
}

FuncDeclStmt::FuncDeclStmt(Token name,
                           std::vector<Token> params,
                           std::unique_ptr<BlockStmt> body,
                           SourceSpan span)
    : Stmt(span), name(std::move(name)), params(std::move(params)), body(std::move(body)) {}

void FuncDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitFuncDeclStmt(*this);
}

RouteDeclStmt::RouteDeclStmt(Token keyword, std::unique_ptr<Expr> path, std::unique_ptr<BlockStmt> body, SourceSpan span)
    : Stmt(span), keyword(std::move(keyword)), path(std::move(path)), body(std::move(body)) {}

void RouteDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitRouteDeclStmt(*this);
}

ReturnStmt::ReturnStmt(Token keyword, std::unique_ptr<Expr> value, SourceSpan span)
    : Stmt(span), keyword(std::move(keyword)), value(std::move(value)) {}

void ReturnStmt::accept(StmtVisitor& visitor) const {
    visitor.visitReturnStmt(*this);
}

}  // namespace wevoaweb
