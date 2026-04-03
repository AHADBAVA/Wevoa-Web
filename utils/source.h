#pragma once

#include <cstddef>
#include <string>

namespace wevoaweb {

struct SourceLocation {
    std::size_t index = 0;
    int line = 1;
    int column = 1;
};

struct SourceSpan {
    SourceLocation start {};
    SourceLocation end {};
};

inline std::string formatLocation(const SourceLocation& location) {
    return std::to_string(location.line) + ":" + std::to_string(location.column);
}

inline std::string formatSpan(const SourceSpan& span) {
    return formatLocation(span.start);
}

}  // namespace wevoaweb
