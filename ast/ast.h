#pragma once

#include <memory>
#include <vector>

#include "interpreter/value.h"
#include "runtime/token.h"

namespace wevoaweb {

class LiteralExpr;
class VariableExpr;
class AssignExpr;
class UnaryExpr;
class BinaryExpr;
class GroupingExpr;
class CallExpr;

class ExpressionStmt;
class VarDeclStmt;
class BlockStmt;
class IfStmt;
class LoopStmt;
class FuncDeclStmt;
class RouteDeclStmt;
class ReturnStmt;

class ExprVisitor {
  public:
    virtual ~ExprVisitor() = default;
    virtual Value visitLiteralExpr(const LiteralExpr& expr) = 0;
    virtual Value visitVariableExpr(const VariableExpr& expr) = 0;
    virtual Value visitAssignExpr(const AssignExpr& expr) = 0;
    virtual Value visitUnaryExpr(const UnaryExpr& expr) = 0;
    virtual Value visitBinaryExpr(const BinaryExpr& expr) = 0;
    virtual Value visitGroupingExpr(const GroupingExpr& expr) = 0;
    virtual Value visitCallExpr(const CallExpr& expr) = 0;
};

class StmtVisitor {
  public:
    virtual ~StmtVisitor() = default;
    virtual void visitExpressionStmt(const ExpressionStmt& stmt) = 0;
    virtual void visitVarDeclStmt(const VarDeclStmt& stmt) = 0;
    virtual void visitBlockStmt(const BlockStmt& stmt) = 0;
    virtual void visitIfStmt(const IfStmt& stmt) = 0;
    virtual void visitLoopStmt(const LoopStmt& stmt) = 0;
    virtual void visitFuncDeclStmt(const FuncDeclStmt& stmt) = 0;
    virtual void visitRouteDeclStmt(const RouteDeclStmt& stmt) = 0;
    virtual void visitReturnStmt(const ReturnStmt& stmt) = 0;
};

class Expr {
  public:
    explicit Expr(SourceSpan span);
    virtual ~Expr() = default;

    SourceSpan span;

    virtual Value accept(ExprVisitor& visitor) const = 0;
};

class Stmt {
  public:
    explicit Stmt(SourceSpan span);
    virtual ~Stmt() = default;

    SourceSpan span;

    virtual void accept(StmtVisitor& visitor) const = 0;
};

class LiteralExpr final : public Expr {
  public:
    LiteralExpr(Value value, SourceSpan span);

    Value value;

    Value accept(ExprVisitor& visitor) const override;
};

class VariableExpr final : public Expr {
  public:
    VariableExpr(Token name, SourceSpan span);

    Token name;

    Value accept(ExprVisitor& visitor) const override;
};

class AssignExpr final : public Expr {
  public:
    AssignExpr(Token name, std::unique_ptr<Expr> value, SourceSpan span);

    Token name;
    std::unique_ptr<Expr> value;

    Value accept(ExprVisitor& visitor) const override;
};

class UnaryExpr final : public Expr {
  public:
    UnaryExpr(Token op, std::unique_ptr<Expr> right, SourceSpan span);

    Token op;
    std::unique_ptr<Expr> right;

    Value accept(ExprVisitor& visitor) const override;
};

class BinaryExpr final : public Expr {
  public:
    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right, SourceSpan span);

    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    Value accept(ExprVisitor& visitor) const override;
};

class GroupingExpr final : public Expr {
  public:
    GroupingExpr(std::unique_ptr<Expr> expression, SourceSpan span);

    std::unique_ptr<Expr> expression;

    Value accept(ExprVisitor& visitor) const override;
};

class CallExpr final : public Expr {
  public:
    CallExpr(std::unique_ptr<Expr> callee,
             Token paren,
             std::vector<std::unique_ptr<Expr>> arguments,
             SourceSpan span);

    std::unique_ptr<Expr> callee;
    Token paren;
    std::vector<std::unique_ptr<Expr>> arguments;

    Value accept(ExprVisitor& visitor) const override;
};

class ExpressionStmt final : public Stmt {
  public:
    ExpressionStmt(std::unique_ptr<Expr> expression, SourceSpan span);

    std::unique_ptr<Expr> expression;

    void accept(StmtVisitor& visitor) const override;
};

class VarDeclStmt final : public Stmt {
  public:
    VarDeclStmt(Token name, std::unique_ptr<Expr> initializer, bool isConstant, SourceSpan span);

    Token name;
    std::unique_ptr<Expr> initializer;
    bool isConstant = false;

    void accept(StmtVisitor& visitor) const override;
};

class BlockStmt final : public Stmt {
  public:
    BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, SourceSpan span);

    std::vector<std::unique_ptr<Stmt>> statements;

    void accept(StmtVisitor& visitor) const override;
};

class IfStmt final : public Stmt {
  public:
    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch,
           SourceSpan span);

    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    void accept(StmtVisitor& visitor) const override;
};

class LoopStmt final : public Stmt {
  public:
    LoopStmt(std::unique_ptr<Stmt> initializer,
             std::unique_ptr<Expr> condition,
             std::unique_ptr<Expr> increment,
             std::unique_ptr<Stmt> body,
             SourceSpan span);

    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;

    void accept(StmtVisitor& visitor) const override;
};

class FuncDeclStmt final : public Stmt {
  public:
    FuncDeclStmt(Token name,
                 std::vector<Token> params,
                 std::unique_ptr<BlockStmt> body,
                 SourceSpan span);

    Token name;
    std::vector<Token> params;
    std::unique_ptr<BlockStmt> body;

    void accept(StmtVisitor& visitor) const override;
};

class ReturnStmt final : public Stmt {
  public:
    ReturnStmt(Token keyword, std::unique_ptr<Expr> value, SourceSpan span);

    Token keyword;
    std::unique_ptr<Expr> value;

    void accept(StmtVisitor& visitor) const override;
};

class RouteDeclStmt final : public Stmt {
  public:
    RouteDeclStmt(Token keyword, std::unique_ptr<Expr> path, std::unique_ptr<BlockStmt> body, SourceSpan span);

    Token keyword;
    std::unique_ptr<Expr> path;
    std::unique_ptr<BlockStmt> body;

    void accept(StmtVisitor& visitor) const override;
};

}  // namespace wevoaweb
