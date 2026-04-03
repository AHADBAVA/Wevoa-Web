#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

#include "ast/ast.h"
#include "interpreter/callable.h"
#include "interpreter/environment.h"
#include "interpreter/route.h"

namespace wevoaweb {

class ReturnSignal final {
  public:
    ReturnSignal(Value value, SourceSpan span);

    Value value;
    SourceSpan span;
};

class Interpreter final : public ExprVisitor, public StmtVisitor {
  public:
    Interpreter(std::ostream& output, std::istream& input);

    void interpret(const std::vector<std::unique_ptr<Stmt>>& statements);
    void execute(const Stmt& stmt);
    Value evaluate(const Expr& expr);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, std::shared_ptr<Environment> environment);

    void registerNative(std::string name, std::optional<std::size_t> arity, NativeCallback callback);
    void registerRoute(std::string path, std::shared_ptr<RouteHandler> route, const SourceSpan& span);

    std::shared_ptr<Environment> globals() const;
    bool hasRoute(const std::string& path) const;
    std::string renderRoute(const std::string& path, const SourceSpan& span = SourceSpan {});
    std::vector<std::string> routePaths() const;
    std::ostream& output();
    std::istream& input();

    Value visitLiteralExpr(const LiteralExpr& expr) override;
    Value visitVariableExpr(const VariableExpr& expr) override;
    Value visitAssignExpr(const AssignExpr& expr) override;
    Value visitUnaryExpr(const UnaryExpr& expr) override;
    Value visitBinaryExpr(const BinaryExpr& expr) override;
    Value visitGroupingExpr(const GroupingExpr& expr) override;
    Value visitCallExpr(const CallExpr& expr) override;

    void visitExpressionStmt(const ExpressionStmt& stmt) override;
    void visitVarDeclStmt(const VarDeclStmt& stmt) override;
    void visitBlockStmt(const BlockStmt& stmt) override;
    void visitIfStmt(const IfStmt& stmt) override;
    void visitLoopStmt(const LoopStmt& stmt) override;
    void visitFuncDeclStmt(const FuncDeclStmt& stmt) override;
    void visitRouteDeclStmt(const RouteDeclStmt& stmt) override;
    void visitReturnStmt(const ReturnStmt& stmt) override;

  private:
    std::int64_t expectInteger(const Value& value, const SourceSpan& span, std::string_view context) const;
    const std::shared_ptr<Callable>& expectCallable(const Value& value,
                                                    const SourceSpan& span,
                                                    std::string_view context) const;
    void prepareLoopInitializer(const Stmt& initializer);

    std::ostream& output_;
    std::istream& input_;
    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> environment_;
    RouteRegistry routes_;
};

}  // namespace wevoaweb
