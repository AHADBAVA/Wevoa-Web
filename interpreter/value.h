#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace wevoaweb {

class Callable;

class Value {
  public:
    using Storage = std::variant<std::monostate, std::int64_t, std::string, bool, std::shared_ptr<Callable>>;

    Value() = default;
    Value(std::int64_t value);
    Value(std::string value);
    Value(const char* value);
    Value(bool value);
    Value(std::shared_ptr<Callable> value);

    bool isNil() const;
    bool isInteger() const;
    bool isString() const;
    bool isBoolean() const;
    bool isCallable() const;

    std::int64_t asInteger() const;
    const std::string& asString() const;
    bool asBoolean() const;
    const std::shared_ptr<Callable>& asCallable() const;

    std::string typeName() const;
    std::string toString() const;
    bool isTruthy() const;

    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

  private:
    Storage storage_ {};
};

}  // namespace wevoaweb
