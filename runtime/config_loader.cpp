#include "runtime/config_loader.h"

#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace wevoaweb {

namespace {

class JsonParser {
  public:
    explicit JsonParser(std::string source) : source_(std::move(source)) {}

    Value parse() {
        skipWhitespace();
        Value value = parseValue();
        skipWhitespace();
        if (!isAtEnd()) {
            throw std::runtime_error("Unexpected trailing characters in JSON.");
        }
        return value;
    }

  private:
    Value parseValue() {
        skipWhitespace();
        if (isAtEnd()) {
            throw std::runtime_error("Unexpected end of JSON.");
        }

        const char ch = peek();
        if (ch == '{') {
            return parseObject();
        }
        if (ch == '[') {
            return parseArray();
        }
        if (ch == '"') {
            return Value(parseString());
        }
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || ch == '-') {
            return Value(parseInteger());
        }
        if (matchLiteral("true")) {
            return Value(true);
        }
        if (matchLiteral("false")) {
            return Value(false);
        }
        if (matchLiteral("null")) {
            return Value {};
        }

        throw std::runtime_error(std::string("Unexpected JSON token starting with '") + ch + "'.");
    }

    Value parseObject() {
        consume('{');
        Value::Object object;
        skipWhitespace();

        if (peek() == '}') {
            advance();
            return Value(std::move(object));
        }

        while (true) {
            skipWhitespace();
            const std::string key = parseString();
            skipWhitespace();
            consume(':');
            object.insert_or_assign(key, parseValue());
            skipWhitespace();

            if (peek() == '}') {
                advance();
                break;
            }

            consume(',');
        }

        return Value(std::move(object));
    }

    Value parseArray() {
        consume('[');
        Value::Array array;
        skipWhitespace();

        if (peek() == ']') {
            advance();
            return Value(std::move(array));
        }

        while (true) {
            array.push_back(parseValue());
            skipWhitespace();

            if (peek() == ']') {
                advance();
                break;
            }

            consume(',');
        }

        return Value(std::move(array));
    }

    std::string parseString() {
        consume('"');
        std::string value;

        while (!isAtEnd() && peek() != '"') {
            if (peek() == '\\') {
                advance();
                if (isAtEnd()) {
                    throw std::runtime_error("Unterminated JSON escape sequence.");
                }

                const char escaped = advance();
                switch (escaped) {
                    case '"':
                        value.push_back('"');
                        break;
                    case '\\':
                        value.push_back('\\');
                        break;
                    case '/':
                        value.push_back('/');
                        break;
                    case 'b':
                        value.push_back('\b');
                        break;
                    case 'f':
                        value.push_back('\f');
                        break;
                    case 'n':
                        value.push_back('\n');
                        break;
                    case 'r':
                        value.push_back('\r');
                        break;
                    case 't':
                        value.push_back('\t');
                        break;
                    default:
                        throw std::runtime_error("Unsupported JSON escape sequence.");
                }
            } else {
                value.push_back(advance());
            }
        }

        consume('"');
        return value;
    }

    std::int64_t parseInteger() {
        std::size_t start = current_;
        if (peek() == '-') {
            advance();
        }

        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
            advance();
        }

        return std::stoll(source_.substr(start, current_ - start));
    }

    bool matchLiteral(const std::string& literal) {
        if (source_.substr(current_, literal.size()) != literal) {
            return false;
        }

        current_ += literal.size();
        return true;
    }

    void skipWhitespace() {
        while (!isAtEnd() && std::isspace(static_cast<unsigned char>(peek())) != 0) {
            ++current_;
        }
    }

    void consume(char expected) {
        if (isAtEnd() || peek() != expected) {
            throw std::runtime_error(std::string("Expected '") + expected + "' in JSON.");
        }
        advance();
    }

    char peek() const {
        return isAtEnd() ? '\0' : source_[current_];
    }

    char advance() {
        return source_[current_++];
    }

    bool isAtEnd() const {
        return current_ >= source_.size();
    }

    std::string source_;
    std::size_t current_ = 0;
};

}  // namespace

Value loadConfigFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return Value(Value::Object {});
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open config file: " + path.string());
    }

    const std::string contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    Value value = JsonParser(contents).parse();
    if (!value.isObject()) {
        throw std::runtime_error("Configuration root must be a JSON object.");
    }

    return value;
}

}  // namespace wevoaweb
