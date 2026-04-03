#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

#include "cli/cli_handler.h"
#include "cli/project_creator.h"
#include "server/dev_server.h"
#include "utils/logger.h"

namespace {

std::atomic<bool>* gInterruptRequested = nullptr;

void handleInterruptSignal(int /*signalNumber*/) {
    if (gInterruptRequested != nullptr) {
        gInterruptRequested->store(true);
    }
}

}  // namespace

int main(int argc, char** argv) {
    wevoaweb::CLIHandler cli;
    wevoaweb::Logger logger(std::cout);

    try {
        const auto command = cli.parse(argc, argv);

        switch (command.type) {
            case wevoaweb::CLIHandler::CommandType::Help:
                cli.printHelp(argc > 0 ? argv[0] : "wevoa");
                return 0;

            case wevoaweb::CLIHandler::CommandType::Create: {
                wevoaweb::ProjectCreator creator;
                const auto projectPath = creator.createProject(command.projectName);
                logger.wevoa("Project created successfully!");
                logger.wevoa("Run:");
                std::cout << "cd " << projectPath.filename().string() << '\n';
                std::cout << "wevoa start\n";
                return 0;
            }

            case wevoaweb::CLIHandler::CommandType::Build:
                logger.info("Build pipeline is reserved for a future release.");
                return 0;

            case wevoaweb::CLIHandler::CommandType::Start: {
                std::atomic<bool> interruptRequested = false;
                gInterruptRequested = &interruptRequested;
                const auto previousHandler = std::signal(SIGINT, handleInterruptSignal);

                wevoaweb::server::DevServer server(command.startOptions,
                                                   logger,
                                                   interruptRequested,
                                                   std::cin,
                                                   std::cout);
                const int exitCode = server.run();

                std::signal(SIGINT, previousHandler);
                gInterruptRequested = nullptr;
                return exitCode;
            }
        }
    } catch (const wevoaweb::CLIUsageError& error) {
        logger.error(error.what());
        std::cout << '\n';
        cli.printHelp(argc > 0 ? argv[0] : "wevoa");
        return 1;
    } catch (const std::exception& error) {
        logger.error(error.what());
        return 1;
    }

    return 0;
}
