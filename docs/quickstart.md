# Quick Start

This guide gets you from source checkout to a running WevoaWeb app.

## 1. Build the CLI

### Windows with MinGW

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

### Linux / macOS

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

## 2. Create a Project

```powershell
.\wevoa.exe create my-app
cd .\my-app
```

## 3. Start the Dev Server

```powershell
..\wevoa.exe start
```

Open:

```text
http://localhost:3000
```

## 4. Edit a Route

Open `views/index.wev` and change the returned HTML.

Example:

```text
let app_name = "My App"

route "/" {
return "<h1>Hello from " + app_name + "</h1>"
}
```

## 5. Reload

While the dev server is running:

- press `R` to reload manually
- press `Q` to quit
- use `Ctrl+C` for graceful shutdown

The file watcher also reloads when files in `views/` or `public/` change.
