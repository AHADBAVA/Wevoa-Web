#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

namespace wevoaweb {

class CLIUsageError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

struct ProjectCommandOptions {
    std::string appDirectory = "app";
    std::string viewsDirectory = "views";
    std::string publicDirectory = "public";
};

struct StartCommandOptions : ProjectCommandOptions {
    std::uint16_t port = 3000;
    bool debugAst = false;
    bool portSpecified = false;
};

struct BuildCommandOptions : ProjectCommandOptions {
    std::string outputDirectory = "build";
};

struct ServeCommandOptions {
    std::uint16_t port = 3000;
    std::string buildDirectory = "build";
    bool portSpecified = false;
};

class CLIHandler {
  public:
    enum class CommandType {
        Start,
        Create,
        Build,
        Serve,
        Version,
        Help
    };

    struct Command {
        CommandType type = CommandType::Help;
        StartCommandOptions startOptions {};
        BuildCommandOptions buildOptions {};
        ServeCommandOptions serveOptions {};
        std::string projectName;
    };

    Command parse(int argc, char** argv) const;
    void printHelp(const std::string& executableName) const;

  private:
    std::uint16_t parsePort(const std::string& value) const;
};

}  // namespace wevoaweb
