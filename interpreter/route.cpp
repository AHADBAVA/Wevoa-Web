#include "interpreter/route.h"

#include <algorithm>
#include <utility>

#include "ast/ast.h"
#include "interpreter/environment.h"
#include "interpreter/interpreter.h"
#include "utils/error.h"

namespace wevoaweb {

RouteHandler::RouteHandler(std::string path, const RouteDeclStmt* declaration, std::shared_ptr<Environment> closure)
    : path_(std::move(path)), declaration_(declaration), closure_(std::move(closure)) {}

const std::string& RouteHandler::path() const {
    return path_;
}

std::string RouteHandler::render(Interpreter& interpreter) const {
    auto environment = std::make_shared<Environment>(closure_);

    try {
        interpreter.executeBlock(declaration_->body->statements, environment);
    } catch (const ReturnSignal& signal) {
        if (signal.value.isNil()) {
            return "";
        }
        return signal.value.toString();
    }

    return "";
}

void RouteRegistry::addRoute(std::shared_ptr<RouteHandler> route, const SourceSpan& span) {
    const auto [found, inserted] = routes_.emplace(route->path(), route);
    if (!inserted) {
        throw RuntimeError("Route '" + route->path() + "' is already defined.", span);
    }
}

bool RouteRegistry::hasRoute(const std::string& path) const {
    return routes_.find(path) != routes_.end();
}

std::string RouteRegistry::render(const std::string& path, Interpreter& interpreter, const SourceSpan& span) const {
    const auto found = routes_.find(path);
    if (found == routes_.end()) {
        throw RuntimeError("No route matched '" + path + "'.", span);
    }

    return found->second->render(interpreter);
}

std::vector<std::string> RouteRegistry::routePaths() const {
    std::vector<std::string> paths;
    paths.reserve(routes_.size());
    for (const auto& entry : routes_) {
        paths.push_back(entry.first);
    }

    std::sort(paths.begin(), paths.end());
    return paths;
}

}  // namespace wevoaweb
