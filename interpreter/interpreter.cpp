#include "interpreter/interpreter.h"

#include <utility>

#include "runtime/builtins.h"
#include "utils/error.h"

namespace wevoaweb {

ReturnSignal::ReturnSignal(Value value, SourceSpan span) : value(std::move(value)), span(span) {}

Interpreter::Interpreter(std::ostream& output, std::istream& input)
    : output_(output),
      input_(input),
      globals_(std::make_shared<Environment>()),
      environment_(globals_) {
    registerBuiltins(*this);
}

void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& statement : statements) {
        try {
            execute(*statement);
        } catch (const ReturnSignal& signal) {
            throw RuntimeError("return may only be used inside a function.", signal.span);
        }
    }
}

void Interpreter::execute(const Stmt& stmt) {
    stmt.accept(*this);
}

Value Interpreter::evaluate(const Expr& expr) {
    return expr.accept(*this);
}

void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements,
                               std::shared_ptr<Environment> environment) {
    const auto previous = environment_;
    environment_ = std::move(environment);

    try {
        for (const auto& statement : statements) {
            execute(*statement);
        }
    } catch (...) {
        environment_ = previous;
        throw;
    }

    environment_ = previous;
}

void Interpreter::registerNative(std::string name, std::optional<std::size_t> arity, NativeCallback callback) {
    const std::string bindingName = name;
    auto nativeFunction = std::make_shared<NativeFunction>(std::move(name), arity, std::move(callback));
    globals_->define(bindingName, Value(std::move(nativeFunction)), true);
}

void Interpreter::registerRoute(std::string path, std::shared_ptr<RouteHandler> route, const SourceSpan& span) {
    if (path.empty() || path.front() != '/') {
        throw RuntimeError("Route paths must start with '/'.", span);
    }

    routes_.addRoute(std::move(route), span);
}

std::shared_ptr<Environment> Interpreter::globals() const {
    return globals_;
}

bool Interpreter::hasRoute(const std::string& path) const {
    return routes_.hasRoute(path);
}

std::string Interpreter::renderRoute(const std::string& path, const SourceSpan& span) {
    return routes_.render(path, *this, span);
}

std::vector<std::string> Interpreter::routePaths() const {
    return routes_.routePaths();
}

std::ostream& Interpreter::output() {
    return output_;
}

std::istream& Interpreter::input() {
    return input_;
}

Value Interpreter::visitLiteralExpr(const LiteralExpr& expr) {
    return expr.value;
}

Value Interpreter::visitVariableExpr(const VariableExpr& expr) {
    return environment_->get(expr.name);
}

Value Interpreter::visitAssignExpr(const AssignExpr& expr) {
    const Value value = evaluate(*expr.value);
    environment_->assign(expr.name, value);
    return value;
}

Value Interpreter::visitUnaryExpr(const UnaryExpr& expr) {
    const Value right = evaluate(*expr.right);

    switch (expr.op.type) {
        case TokenType::Minus:
            return Value(-expectInteger(right, expr.op.span, "Unary '-'"));
        case TokenType::Bang:
            return Value(!right.isTruthy());
        default:
            throw RuntimeError("Unsupported unary operator '" + expr.op.lexeme + "'.", expr.op.span);
    }
}

Value Interpreter::visitBinaryExpr(const BinaryExpr& expr) {
    const Value left = evaluate(*expr.left);
    const Value right = evaluate(*expr.right);

    switch (expr.op.type) {
        case TokenType::Plus:
            if (left.isInteger() && right.isInteger()) {
                return Value(left.asInteger() + right.asInteger());
            }
            if (left.isString() || right.isString()) {
                return Value(left.toString() + right.toString());
            }
            throw RuntimeError("Operator '+' expects integers or at least one string operand.", expr.op.span);
        case TokenType::Minus:
            return Value(expectInteger(left, expr.op.span, "Left operand of '-'") -
                         expectInteger(right, expr.op.span, "Right operand of '-'"));
        case TokenType::Star:
            return Value(expectInteger(left, expr.op.span, "Left operand of '*'") *
                         expectInteger(right, expr.op.span, "Right operand of '*'"));
        case TokenType::Slash: {
            const auto divisor = expectInteger(right, expr.op.span, "Right operand of '/'");
            if (divisor == 0) {
                throw RuntimeError("Division by zero.", expr.op.span);
            }
            return Value(expectInteger(left, expr.op.span, "Left operand of '/'") / divisor);
        }
        case TokenType::Greater:
            return Value(expectInteger(left, expr.op.span, "Left operand of '>'") >
                         expectInteger(right, expr.op.span, "Right operand of '>'"));
        case TokenType::GreaterEqual:
            return Value(expectInteger(left, expr.op.span, "Left operand of '>='") >=
                         expectInteger(right, expr.op.span, "Right operand of '>='"));
        case TokenType::Less:
            return Value(expectInteger(left, expr.op.span, "Left operand of '<'") <
                         expectInteger(right, expr.op.span, "Right operand of '<'"));
        case TokenType::LessEqual:
            return Value(expectInteger(left, expr.op.span, "Left operand of '<='") <=
                         expectInteger(right, expr.op.span, "Right operand of '<='"));
        case TokenType::EqualEqual:
            return Value(left == right);
        case TokenType::BangEqual:
            return Value(left != right);
        default:
            throw RuntimeError("Unsupported binary operator '" + expr.op.lexeme + "'.", expr.op.span);
    }
}

Value Interpreter::visitGroupingExpr(const GroupingExpr& expr) {
    return evaluate(*expr.expression);
}

Value Interpreter::visitCallExpr(const CallExpr& expr) {
    const Value callee = evaluate(*expr.callee);
    const auto& callable = expectCallable(callee, expr.paren.span, "Function call");

    std::vector<Value> arguments;
    arguments.reserve(expr.arguments.size());
    for (const auto& argument : expr.arguments) {
        arguments.push_back(evaluate(*argument));
    }

    if (const auto arity = callable->arity(); arity.has_value() && arguments.size() != *arity) {
        throw RuntimeError("Expected " + std::to_string(*arity) + " argument(s) but got " +
                               std::to_string(arguments.size()) + ".",
                           expr.paren.span);
    }

    return callable->call(*this, arguments, expr.paren.span);
}

void Interpreter::visitExpressionStmt(const ExpressionStmt& stmt) {
    static_cast<void>(evaluate(*stmt.expression));
}

void Interpreter::visitVarDeclStmt(const VarDeclStmt& stmt) {
    Value value;
    if (stmt.initializer) {
        value = evaluate(*stmt.initializer);
    }

    environment_->define(stmt.name, std::move(value), stmt.isConstant);
}

void Interpreter::visitBlockStmt(const BlockStmt& stmt) {
    executeBlock(stmt.statements, std::make_shared<Environment>(environment_));
}

void Interpreter::visitIfStmt(const IfStmt& stmt) {
    if (evaluate(*stmt.condition).isTruthy()) {
        execute(*stmt.thenBranch);
    } else if (stmt.elseBranch) {
        execute(*stmt.elseBranch);
    }
}

void Interpreter::visitLoopStmt(const LoopStmt& stmt) {
    const auto previous = environment_;
    environment_ = std::make_shared<Environment>(environment_);

    try {
        if (stmt.initializer) {
            prepareLoopInitializer(*stmt.initializer);
            execute(*stmt.initializer);
        }

        while (!stmt.condition || evaluate(*stmt.condition).isTruthy()) {
            execute(*stmt.body);

            if (stmt.increment) {
                static_cast<void>(evaluate(*stmt.increment));
            }
        }
    } catch (...) {
        environment_ = previous;
        throw;
    }

    environment_ = previous;
}

void Interpreter::visitFuncDeclStmt(const FuncDeclStmt& stmt) {
    environment_->define(stmt.name, Value(std::make_shared<WevoaFunction>(&stmt, environment_)), true);
}

void Interpreter::visitRouteDeclStmt(const RouteDeclStmt& stmt) {
    const Value pathValue = evaluate(*stmt.path);
    if (!pathValue.isString()) {
        throw RuntimeError("Route path must evaluate to a string.", stmt.path->span);
    }

    const std::string& path = pathValue.asString();
    registerRoute(path, std::make_shared<RouteHandler>(path, &stmt, environment_), stmt.span);
}

void Interpreter::visitReturnStmt(const ReturnStmt& stmt) {
    Value value;
    if (stmt.value) {
        value = evaluate(*stmt.value);
    }

    throw ReturnSignal(std::move(value), stmt.keyword.span);
}

std::int64_t Interpreter::expectInteger(const Value& value,
                                        const SourceSpan& span,
                                        std::string_view context) const {
    if (!value.isInteger()) {
        throw RuntimeError(std::string(context) + " must be an integer, got " + value.typeName() + ".", span);
    }

    return value.asInteger();
}

const std::shared_ptr<Callable>& Interpreter::expectCallable(const Value& value,
                                                             const SourceSpan& span,
                                                             std::string_view context) const {
    if (!value.isCallable()) {
        throw RuntimeError(std::string(context) + " requires a callable value, got " + value.typeName() + ".", span);
    }

    return value.asCallable();
}

void Interpreter::prepareLoopInitializer(const Stmt& initializer) {
    const auto* expressionStmt = dynamic_cast<const ExpressionStmt*>(&initializer);
    if (expressionStmt == nullptr) {
        return;
    }

    const auto* assignExpr = dynamic_cast<const AssignExpr*>(expressionStmt->expression.get());
    if (assignExpr == nullptr) {
        return;
    }

    if (!environment_->exists(assignExpr->name.lexeme)) {
        environment_->define(assignExpr->name, Value {}, false);
    }
}

}  // namespace wevoaweb
