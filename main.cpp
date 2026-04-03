#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

#include "cli/build_pipeline.h"
#include "cli/cli_handler.h"
#include "cli/project_creator.h"
#include "server/dev_server.h"
#include "server/serve_server.h"
#include "utils/logger.h"
#include "utils/version.h"

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

            case wevoaweb::CLIHandler::CommandType::Build: {
                wevoaweb::BuildPipeline pipeline;
                const auto result = pipeline.build(command.buildOptions, std::cin, std::cout);
                logger.success("Build complete");
                logger.info("Output: " + result.outputRoot.string());
                logger.info("Routes bundled: " + std::to_string(result.routes.size()));
                logger.info("Assets bundled: " + std::to_string(result.assets.size()));
                return 0;
            }

            case wevoaweb::CLIHandler::CommandType::Version:
                std::cout << wevoaweb::kWevoaRuntimeName << ' ' << wevoaweb::kWevoaVersion << '\n';
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

            case wevoaweb::CLIHandler::CommandType::Serve: {
                std::atomic<bool> interruptRequested = false;
                gInterruptRequested = &interruptRequested;
                const auto previousHandler = std::signal(SIGINT, handleInterruptSignal);

                wevoaweb::server::ServeServer server(command.serveOptions,
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
