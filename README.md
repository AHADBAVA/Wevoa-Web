# WevoaWeb

WevoaWeb is an open-source server-side scripting language and web engine written in modern C++17.

It combines:

- a small original programming language
- a tree-walk interpreter
- a built-in HTTP development server
- a CLI for creating and running WevoaWeb apps

The browser receives plain HTML and static assets only. No JavaScript runtime is required in the browser.

## Status

WevoaWeb is currently experimental but working. The repository already includes:

- a lexer, parser, AST, and interpreter
- lexical scoping, functions, constants, loops, and conditionals
- native `route` declarations for server-rendered pages
- a dev server with file watching and reload controls
- a project scaffolder with a clean starter template

This is a serious foundation for a future language and framework, but it is still early-stage software.

## Why WevoaWeb

- Original design: it is not a wrapper around another scripting engine.
- Server-first model: routes render HTML on the server.
- Lightweight implementation: modern C++ with no heavy framework dependency.
- Easy to extend: the codebase is structured around a clear language pipeline.
- Friendly developer workflow: `wevoa start` and `wevoa create` are built in.

## Core Capabilities

### Language

- Variables with `let` and `const`
- Integers, strings, booleans, and `nil`
- Arithmetic and comparison operators
- String concatenation
- Block scope with `{ }`
- `if / else`
- `loop (init; condition; increment)`
- Functions with parameters and `return`
- Lexical closures
- Built-in `print()` and `input()`
- Source-aware lexer, parser, and runtime errors with line/column spans

Example:

```text
let title = "WevoaWeb"

func welcome(name) {
return "<h1>Hello " + name + "</h1>"
}

route "/" {
return welcome(title)
}
```

### Web Engine

- Route declarations inside `.wev` files
- Dynamic HTML generation from language expressions
- HTTP server for `GET` requests
- Static file serving from `public/`
- 404 and 405 responses
- Plain HTML responses only

Example route:

```text
let app_name = "WevoaWeb"

route "/" {
return "<h1>Welcome to " + app_name + "</h1>"
}
```

### Developer CLI

- `wevoa start`
- `wevoa create <project-name>`
- `wevoa build`
- `wevoa help`

Dev server features:

- port configuration
- file watching for `views/` and `public/`
- manual reload with `R`
- quit with `Q`
- graceful shutdown with `Ctrl+C`

## Repository Layout

```text
ast/
cli/
examples/
interpreter/
lexer/
parser/
public/
runtime/
server/
utils/
views/
watcher/
main.cpp
```

## Architecture

WevoaWeb is split into a clean execution pipeline:

1. `lexer/`
   Turns source text into tokens with line/column tracking.
2. `parser/`
   Builds an AST with recursive-descent parsing and operator precedence.
3. `ast/`
   Defines expression and statement node types using `std::unique_ptr`.
4. `interpreter/`
   Evaluates the AST, manages environments, functions, closures, and routes.
5. `runtime/`
   Manages sessions, built-ins, and optional AST debug output.
6. `server/`
   Loads views, serves routes, and handles HTTP requests.
7. `cli/`
   Provides the developer-facing command system.

For a more contributor-focused overview, see [docs/architecture.md](docs/architecture.md).

## Build

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

## Getting Started

### Run the framework demo

```powershell
.\wevoa.exe start
```

Then open:

```text
http://localhost:3000
```

### Create a new project

```powershell
.\wevoa.exe create my-app
cd .\my-app
..\wevoa.exe start
```

## Generated Project Structure

```text
my-app/
  views/
    index.wev
    layout.wev
  public/
    style.css
  wevoa.config.json
```

The scaffold includes a simple centered starter page with a minimal navbar and a green-and-white theme.

## Examples

Sample scripts live in `examples/`.

The repository also includes demo routes in `views/` used by the local dev server.

## Current Limitations

WevoaWeb is intentionally small right now. Some things are not implemented yet:

- module/import system
- arrays, maps, or object literals
- floating-point numbers
- classes and methods
- `break` and `continue`
- production build pipeline
- full template/layout engine
- automatic use of `wevoa.config.json` at runtime

## Roadmap

See [ROADMAP.md](ROADMAP.md).

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening major pull requests.

Community expectations are described in [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

Security reporting guidance is available in [SECURITY.md](SECURITY.md).

## License

WevoaWeb is released under the MIT License.

See [LICENSE](LICENSE).
