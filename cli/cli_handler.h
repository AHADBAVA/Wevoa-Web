#pragma once

#include <stdexcept>
#include <cstdint>
#include <string>

namespace wevoaweb {

class CLIUsageError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

struct StartCommandOptions {
    std::uint16_t port = 3000;
    std::string viewsDirectory = "views";
    std::string publicDirectory = "public";
    bool debugAst = false;
};

class CLIHandler {
  public:
    enum class CommandType {
        Start,
        Create,
        Build,
        Help
    };

    struct Command {
        CommandType type = CommandType::Help;
        StartCommandOptions startOptions {};
        std::string projectName;
    };

    Command parse(int argc, char** argv) const;
    void printHelp(const std::string& executableName) const;

  private:
    std::uint16_t parsePort(const std::string& value) const;
};

}  // namespace wevoaweb
