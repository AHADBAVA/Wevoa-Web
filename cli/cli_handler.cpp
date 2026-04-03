#include "cli/cli_handler.h"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace wevoaweb {

CLIHandler::Command CLIHandler::parse(int argc, char** argv) const {
    Command command;

    if (argc <= 1) {
        command.type = CommandType::Help;
        return command;
    }

    const std::string subcommand = argv[1];
    if (subcommand == "help" || subcommand == "--help" || subcommand == "-h") {
        command.type = CommandType::Help;
        return command;
    }

    if (subcommand == "build") {
        command.type = CommandType::Build;
        return command;
    }

    if (subcommand == "create") {
        if (argc < 3) {
            throw CLIUsageError("create requires a project name.");
        }

        if (argc > 3) {
            throw CLIUsageError("create accepts exactly one project name.");
        }

        command.type = CommandType::Create;
        command.projectName = argv[2];
        return command;
    }

    if (subcommand != "start") {
        throw std::runtime_error("Unknown command: " + subcommand);
    }

    command.type = CommandType::Start;
    for (int index = 2; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--port") {
            if (index + 1 >= argc) {
                throw CLIUsageError("--port requires a value.");
            }
            command.startOptions.port = parsePort(argv[++index]);
            command.startOptions.portSpecified = true;
        } else if (argument == "--views") {
            if (index + 1 >= argc) {
                throw CLIUsageError("--views requires a value.");
            }
            command.startOptions.viewsDirectory = argv[++index];
        } else if (argument == "--public") {
            if (index + 1 >= argc) {
                throw CLIUsageError("--public requires a value.");
            }
            command.startOptions.publicDirectory = argv[++index];
        } else if (argument == "--debug-ast") {
            command.startOptions.debugAst = true;
        } else if (argument == "--help" || argument == "-h") {
            command.type = CommandType::Help;
            return command;
        } else {
            throw CLIUsageError("Unknown option for start: " + argument);
        }
    }

    return command;
}

void CLIHandler::printHelp(const std::string& executableName) const {
    std::cout << "WevoaWeb Developer CLI\n";
    std::cout << '\n';
    std::cout << "Usage:\n";
    std::cout << "  " << executableName << " start [--port 3000] [--views views] [--public public]\n";
    std::cout << "  " << executableName << " create <project-name>\n";
    std::cout << "  " << executableName << " build\n";
    std::cout << "  " << executableName << " help\n";
    std::cout << '\n';
    std::cout << "Commands:\n";
    std::cout << "  start        Start the development server and file watcher\n";
    std::cout << "  create       Generate a new WevoaWeb project scaffold\n";
    std::cout << "  build        Placeholder for a future production build pipeline\n";
    std::cout << "  help         Show this help message\n";
    std::cout << '\n';
    std::cout << "While running `start`:\n";
    std::cout << "  R            Reload views and runtime state\n";
    std::cout << "  Q            Quit the development server\n";
    std::cout << "  Ctrl+C       Gracefully stop the server\n";
}

std::uint16_t CLIHandler::parsePort(const std::string& value) const {
    std::size_t parsedCharacters = 0;
    const unsigned long parsedPort = std::stoul(value, &parsedCharacters, 10);
    if (parsedCharacters != value.size() || parsedPort > std::numeric_limits<std::uint16_t>::max()) {
        throw CLIUsageError("Invalid port: " + value);
    }

    return static_cast<std::uint16_t>(parsedPort);
}

}  // namespace wevoaweb
