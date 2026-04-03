# Contributing to WevoaWeb

Thanks for your interest in contributing.

## Project Goals

WevoaWeb is building an original server-side language and web engine with a small, understandable implementation. Good contributions usually improve one of these areas:

- language design
- runtime correctness
- parser or interpreter quality
- documentation
- developer tooling
- tests and examples

## Before You Start

- Read the current README and roadmap.
- Check whether the feature already exists or is already planned.
- Prefer focused pull requests instead of large unrelated changes.

## Development Setup

Build on Windows with MinGW:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -I. main.cpp `
    cli/cli_handler.cpp `
    cli/project_creator.cpp `
    lexer/lexer.cpp parser/parser.cpp ast/ast.cpp `
    interpreter/value.cpp interpreter/environment.cpp `
    interpreter/callable.cpp interpreter/interpreter.cpp interpreter/route.cpp `
    runtime/builtins.cpp runtime/ast_printer.cpp runtime/session.cpp `
    server/http_types.cpp server/web_app.cpp server/http_server.cpp server/dev_server.cpp `
    watcher/file_watcher.cpp `
    utils/logger.cpp utils/keyboard.cpp utils/file_writer.cpp `
    -o wevoa.exe -lws2_32
```

Build on Linux or macOS:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -I. main.cpp \
    cli/cli_handler.cpp \
    cli/project_creator.cpp \
    lexer/lexer.cpp parser/parser.cpp ast/ast.cpp \
    interpreter/value.cpp interpreter/environment.cpp \
    interpreter/callable.cpp interpreter/interpreter.cpp interpreter/route.cpp \
    runtime/builtins.cpp runtime/ast_printer.cpp runtime/session.cpp \
    server/http_types.cpp server/web_app.cpp server/http_server.cpp server/dev_server.cpp \
    watcher/file_watcher.cpp \
    utils/logger.cpp utils/keyboard.cpp utils/file_writer.cpp \
    -o wevoa
```

## Contribution Guidelines

- Keep code readable and modular.
- Prefer modern C++17 style and RAII-based resource management.
- Avoid introducing heavy dependencies.
- Preserve the language's original design goals.
- Update documentation when behavior changes.
- Add or adjust examples when new syntax or features are introduced.

## Pull Requests

Good pull requests usually include:

- a clear summary of the change
- the motivation for the change
- notes about behavior changes
- build or test steps used to verify it

If your change is large or changes the language surface area, open an issue or discussion first if possible.
