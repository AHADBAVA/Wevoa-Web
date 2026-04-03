#pragma once

#include <string>
#include <string_view>

#include "utils/source.h"

namespace wevoaweb {

enum class TokenType {
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Minus,
    Plus,
    Slash,
    Star,
    Semicolon,
    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Identifier,
    String,
    Integer,
    Let,
    Const,
    Func,
    Route,
    If,
    Else,
    Loop,
    Return,
    True,
    False,
    Newline,
    Eof
};

struct Token {
    TokenType type {};
    std::string lexeme;
    SourceSpan span {};
};

inline std::string_view tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::LeftParen:
            return "LeftParen";
        case TokenType::RightParen:
            return "RightParen";
        case TokenType::LeftBrace:
            return "LeftBrace";
        case TokenType::RightBrace:
            return "RightBrace";
        case TokenType::Comma:
            return "Comma";
        case TokenType::Minus:
            return "Minus";
        case TokenType::Plus:
            return "Plus";
        case TokenType::Slash:
            return "Slash";
        case TokenType::Star:
            return "Star";
        case TokenType::Semicolon:
            return "Semicolon";
        case TokenType::Bang:
            return "Bang";
        case TokenType::BangEqual:
            return "BangEqual";
        case TokenType::Equal:
            return "Equal";
        case TokenType::EqualEqual:
            return "EqualEqual";
        case TokenType::Greater:
            return "Greater";
        case TokenType::GreaterEqual:
            return "GreaterEqual";
        case TokenType::Less:
            return "Less";
        case TokenType::LessEqual:
            return "LessEqual";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::String:
            return "String";
        case TokenType::Integer:
            return "Integer";
        case TokenType::Let:
            return "Let";
        case TokenType::Const:
            return "Const";
        case TokenType::Func:
            return "Func";
        case TokenType::Route:
            return "Route";
        case TokenType::If:
            return "If";
        case TokenType::Else:
            return "Else";
        case TokenType::Loop:
            return "Loop";
        case TokenType::Return:
            return "Return";
        case TokenType::True:
            return "True";
        case TokenType::False:
            return "False";
        case TokenType::Newline:
            return "Newline";
        case TokenType::Eof:
            return "Eof";
    }

    return "Unknown";
}

}  // namespace wevoaweb
