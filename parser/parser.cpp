#include "parser/parser.h"

#include <cstdint>
#include <utility>

#include "utils/error.h"

namespace wevoaweb {

namespace {

SourceSpan combine(const SourceSpan& first, const SourceSpan& second) {
    return SourceSpan {first.start, second.end};
}

SourceSpan combine(const Token& first, const Token& second) {
    return SourceSpan {first.span.start, second.span.end};
}

}  // namespace

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    skipSeparators();

    while (!isAtEnd()) {
        statements.push_back(declaration());
        skipSeparators();
    }

    return statements;
}

std::unique_ptr<Stmt> Parser::declaration() {
    skipSeparators();

    if (match({TokenType::Func})) {
        return functionDeclaration();
    }

    if (match({TokenType::Route})) {
        return routeDeclaration();
    }

    if (match({TokenType::Let, TokenType::Const})) {
        return variableDeclaration(previous(), true);
    }

    return statement();
}

std::unique_ptr<Stmt> Parser::functionDeclaration() {
    const Token keyword = previous();
    const Token& name = consume(TokenType::Identifier, "Expected function name.");
    consume(TokenType::LeftParen, "Expected '(' after function name.");

    std::vector<Token> params;
    if (!check(TokenType::RightParen)) {
        do {
            params.push_back(consume(TokenType::Identifier, "Expected parameter name."));
        } while (match({TokenType::Comma}));
    }

    consume(TokenType::RightParen, "Expected ')' after parameter list.");
    skipSeparators();
    const Token& leftBrace = consume(TokenType::LeftBrace, "Expected '{' before function body.");
    auto body = blockStatement(leftBrace);
    const SourceSpan bodySpan = body->span;
    return std::make_unique<FuncDeclStmt>(name, std::move(params), std::move(body), combine(keyword.span, bodySpan));
}

std::unique_ptr<Stmt> Parser::routeDeclaration() {
    const Token keyword = previous();
    auto path = expression();
    skipSeparators();
    const Token& leftBrace = consume(TokenType::LeftBrace, "Expected '{' before route body.");
    auto body = blockStatement(leftBrace);
    return std::make_unique<RouteDeclStmt>(keyword,
                                           std::move(path),
                                           std::move(body),
                                           SourceSpan {keyword.span.start, body->span.end});
}

std::unique_ptr<Stmt> Parser::variableDeclaration(const Token& keyword, bool expectTerminator) {
    const Token& name = consume(TokenType::Identifier, "Expected variable name.");
    std::unique_ptr<Expr> initializer;

    if (match({TokenType::Equal})) {
        initializer = expression();
    } else if (keyword.type == TokenType::Const) {
        throw ParseError("const variables must be initialized.", name.span);
    }

    const SourceSpan span = initializer ? combine(keyword.span, initializer->span) : combine(keyword.span, name.span);

    if (expectTerminator) {
        consumeStatementTerminator("Expected newline or ';' after variable declaration.");
    }

    return std::make_unique<VarDeclStmt>(name, std::move(initializer), keyword.type == TokenType::Const, span);
}

std::unique_ptr<Stmt> Parser::statement() {
    skipSeparators();

    if (match({TokenType::If})) {
        return ifStatement(previous());
    }

    if (match({TokenType::Loop})) {
        return loopStatement(previous());
    }

    if (match({TokenType::Return})) {
        return returnStatement(previous());
    }

    if (match({TokenType::LeftBrace})) {
        return blockStatement(previous());
    }

    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::ifStatement(const Token& keyword) {
    consume(TokenType::LeftParen, "Expected '(' after if.");
    auto condition = expression();
    consume(TokenType::RightParen, "Expected ')' after if condition.");

    auto thenBranch = statement();
    skipSeparators();

    std::unique_ptr<Stmt> elseBranch;
    if (match({TokenType::Else})) {
        elseBranch = statement();
    }

    const SourceSpan endSpan = elseBranch ? elseBranch->span : thenBranch->span;
    return std::make_unique<IfStmt>(std::move(condition),
                                    std::move(thenBranch),
                                    std::move(elseBranch),
                                    combine(keyword.span, endSpan));
}

std::unique_ptr<Stmt> Parser::loopStatement(const Token& keyword) {
    consume(TokenType::LeftParen, "Expected '(' after loop.");

    std::unique_ptr<Stmt> initializer;
    if (!check(TokenType::Semicolon)) {
        if (match({TokenType::Let, TokenType::Const})) {
            initializer = variableDeclaration(previous(), false);
        } else {
            initializer = expressionStatement(false);
        }
    }
    consume(TokenType::Semicolon, "Expected ';' after loop initializer.");

    std::unique_ptr<Expr> condition;
    if (!check(TokenType::Semicolon)) {
        condition = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after loop condition.");

    std::unique_ptr<Expr> increment;
    if (!check(TokenType::RightParen)) {
        increment = expression();
    }
    consume(TokenType::RightParen, "Expected ')' after loop header.");

    auto body = statement();
    const SourceSpan bodySpan = body->span;
    return std::make_unique<LoopStmt>(std::move(initializer),
                                      std::move(condition),
                                      std::move(increment),
                                      std::move(body),
                                      combine(keyword.span, bodySpan));
}

std::unique_ptr<Stmt> Parser::returnStatement(const Token& keyword) {
    std::unique_ptr<Expr> value;
    if (!isStatementBoundary()) {
        value = expression();
    }

    consumeStatementTerminator("Expected newline or ';' after return statement.");
    const SourceSpan span = value ? combine(keyword.span, value->span) : keyword.span;
    return std::make_unique<ReturnStmt>(keyword, std::move(value), span);
}

std::unique_ptr<Stmt> Parser::expressionStatement(bool expectTerminator) {
    auto expr = expression();
    const SourceSpan span = expr->span;

    if (expectTerminator) {
        consumeStatementTerminator("Expected newline or ';' after expression.");
    }

    return std::make_unique<ExpressionStmt>(std::move(expr), span);
}

std::unique_ptr<BlockStmt> Parser::blockStatement(const Token& leftBrace) {
    std::vector<std::unique_ptr<Stmt>> statements;
    skipSeparators();

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        statements.push_back(declaration());
        skipSeparators();
    }

    const Token& rightBrace = consume(TokenType::RightBrace, "Expected '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements), combine(leftBrace, rightBrace));
}

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = equality();

    if (match({TokenType::Equal})) {
        const Token equals = previous();
        auto value = assignment();

        if (const auto* variable = dynamic_cast<VariableExpr*>(expr.get())) {
            const SourceLocation start = expr->span.start;
            const SourceLocation end = value->span.end;
            return std::make_unique<AssignExpr>(variable->name,
                                                std::move(value),
                                                SourceSpan {start, end});
        }

        throw ParseError("Invalid assignment target.", equals.span);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();

    while (match({TokenType::EqualEqual, TokenType::BangEqual})) {
        const Token op = previous();
        auto right = comparison();
        const SourceLocation start = expr->span.start;
        const SourceLocation end = right->span.end;
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right), SourceSpan {start, end});
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        const Token op = previous();
        auto right = term();
        const SourceLocation start = expr->span.start;
        const SourceLocation end = right->span.end;
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right), SourceSpan {start, end});
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();

    while (match({TokenType::Plus, TokenType::Minus})) {
        const Token op = previous();
        auto right = factor();
        const SourceLocation start = expr->span.start;
        const SourceLocation end = right->span.end;
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right), SourceSpan {start, end});
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();

    while (match({TokenType::Star, TokenType::Slash})) {
        const Token op = previous();
        auto right = unary();
        const SourceLocation start = expr->span.start;
        const SourceLocation end = right->span.end;
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right), SourceSpan {start, end});
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::Bang, TokenType::Minus})) {
        const Token op = previous();
        auto right = unary();
        const SourceLocation end = right->span.end;
        return std::make_unique<UnaryExpr>(op, std::move(right), SourceSpan {op.span.start, end});
    }

    return call();
}

std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();

    while (true) {
        if (match({TokenType::LeftParen})) {
            expr = finishCall(std::move(expr));
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> callee) {
    const SourceLocation start = callee->span.start;
    std::vector<std::unique_ptr<Expr>> arguments;
    if (!check(TokenType::RightParen)) {
        do {
            arguments.push_back(expression());
        } while (match({TokenType::Comma}));
    }

    const Token& paren = consume(TokenType::RightParen, "Expected ')' after arguments.");
    return std::make_unique<CallExpr>(std::move(callee), paren, std::move(arguments), SourceSpan {start, paren.span.end});
}

std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::False})) {
        return std::make_unique<LiteralExpr>(Value(false), previous().span);
    }

    if (match({TokenType::True})) {
        return std::make_unique<LiteralExpr>(Value(true), previous().span);
    }

    if (match({TokenType::Integer})) {
        return std::make_unique<LiteralExpr>(Value(static_cast<std::int64_t>(std::stoll(previous().lexeme))),
                                             previous().span);
    }

    if (match({TokenType::String})) {
        return std::make_unique<LiteralExpr>(Value(previous().lexeme), previous().span);
    }

    if (match({TokenType::Identifier})) {
        return std::make_unique<VariableExpr>(previous(), previous().span);
    }

    if (match({TokenType::LeftParen})) {
        const Token leftParen = previous();
        auto expr = expression();
        const Token& rightParen = consume(TokenType::RightParen, "Expected ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr), combine(leftParen, rightParen));
    }

    throw ParseError("Expected expression.", peek().span);
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (const TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }

    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return type == TokenType::Eof;
    }

    return peek().type == type;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }

    throw ParseError(message, peek().span);
}

void Parser::consumeStatementTerminator(const std::string& message) {
    if (match({TokenType::Semicolon, TokenType::Newline})) {
        skipSeparators();
        return;
    }

    if (check(TokenType::RightBrace) || check(TokenType::Eof)) {
        return;
    }

    throw ParseError(message, peek().span);
}

void Parser::skipSeparators() {
    while (match({TokenType::Newline, TokenType::Semicolon})) {
    }
}

bool Parser::isStatementBoundary() const {
    return check(TokenType::Semicolon) || check(TokenType::Newline) || check(TokenType::RightBrace) ||
           check(TokenType::Eof);
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::Eof;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        ++current_;
    }

    return previous();
}

}  // namespace wevoaweb
