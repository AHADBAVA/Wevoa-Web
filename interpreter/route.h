#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/source.h"

namespace wevoaweb {

class Environment;
class Interpreter;
class RouteDeclStmt;

// RouteHandler keeps a route body plus its captured lexical scope so the
// server can render HTML later without re-parsing the view source.
class RouteHandler final {
  public:
    RouteHandler(std::string path, const RouteDeclStmt* declaration, std::shared_ptr<Environment> closure);

    const std::string& path() const;
    std::string render(Interpreter& interpreter) const;

  private:
    std::string path_;
    const RouteDeclStmt* declaration_;
    std::shared_ptr<Environment> closure_;
};

class RouteRegistry final {
  public:
    void addRoute(std::shared_ptr<RouteHandler> route, const SourceSpan& span);
    bool hasRoute(const std::string& path) const;
    std::string render(const std::string& path, Interpreter& interpreter, const SourceSpan& span) const;
    std::vector<std::string> routePaths() const;

  private:
    std::unordered_map<std::string, std::shared_ptr<RouteHandler>> routes_;
};

}  // namespace wevoaweb
