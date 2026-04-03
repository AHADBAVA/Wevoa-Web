#pragma once

#include <mutex>
#include <ostream>
#include <string>

namespace wevoaweb {

enum class LogLevel {
    Wevoa,
    Info,
    Watch,
    Error,
    Success
};

class Logger {
  public:
    explicit Logger(std::ostream& stream);

    void wevoa(const std::string& message);
    void info(const std::string& message);
    void watch(const std::string& message);
    void error(const std::string& message);
    void success(const std::string& message);

  private:
    void log(LogLevel level, const std::string& message);
    const char* tag(LogLevel level) const;
    const char* color(LogLevel level) const;
    bool useColor() const;

    std::ostream& stream_;
    mutable std::mutex mutex_;
};

}  // namespace wevoaweb
