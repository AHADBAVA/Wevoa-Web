#pragma once

#include <initializer_list>
#include <memory>
#include <vector>

#include "ast/ast.h"

namespace wevoaweb {

class Parser {
  public:
    explicit Parser(const std::vector<Token>& tokens);

    std::vector<std::unique_ptr<Stmt>> parse();

  private:
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> functionDeclaration();
    std::unique_ptr<Stmt> routeDeclaration();
    std::unique_ptr<Stmt> variableDeclaration(const Token& keyword, bool expectTerminator);

    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> ifStatement(const Token& keyword);
    std::unique_ptr<Stmt> loopStatement(const Token& keyword);
    std::unique_ptr<Stmt> returnStatement(const Token& keyword);
    std::unique_ptr<Stmt> expressionStatement(bool expectTerminator = true);
    std::unique_ptr<BlockStmt> blockStatement(const Token& leftBrace);

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> callee);
    std::unique_ptr<Expr> primary();

    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    const Token& consume(TokenType type, const std::string& message);
    void consumeStatementTerminator(const std::string& message);
    void skipSeparators();
    bool isStatementBoundary() const;
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();

    const std::vector<Token>& tokens_;
    std::size_t current_ = 0;
};

}  // namespace wevoaweb
