<p align="center">
  <img src="assets/repo-banner.svg" alt="WevoaWeb banner" width="100%" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/c%2B%2B-17-0f172a" alt="C++17" />
  <img src="https://img.shields.io/badge/status-experimental-1b8a52" alt="Experimental" />
</p>

<p align="center">
  An original server-side scripting language and HTML-first web engine built in modern C++17.
</p>

WevoaWeb combines a small programming language, a tree-walk interpreter, a built-in HTTP dev server, and a CLI for creating and running server-rendered apps. The browser receives plain HTML and static assets only.

## Why It Stands Out

- Original implementation from lexer to server runtime
- HTML-first web model with native `route` declarations
- Lightweight codebase that is easy to read and extend
- Modern C++17 architecture with clear language layers
- Built-in developer workflow with `wevoa start` and `wevoa create`
- Template control flow with `{% if %}` and `{% for %}`
- Clean starter UI for new projects

## Quick Start

Build:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -I. main.cpp `
    cli/cli_handler.cpp `
    cli/build_pipeline.cpp `
    cli/project_creator.cpp `
    lexer/lexer.cpp parser/parser.cpp ast/ast.cpp `
    interpreter/value.cpp interpreter/environment.cpp `
    interpreter/callable.cpp interpreter/interpreter.cpp interpreter/route.cpp `
    runtime/builtins.cpp runtime/ast_printer.cpp runtime/session.cpp `
    runtime/template_engine.cpp runtime/config_loader.cpp runtime/sqlite_module.cpp `
    server/http_types.cpp server/web_app.cpp server/http_server.cpp server/dev_server.cpp server/serve_server.cpp `
    watcher/file_watcher.cpp `
    utils/logger.cpp utils/keyboard.cpp utils/file_writer.cpp utils/project_layout.cpp `
    -o wevoa.exe -lws2_32 -lsqlite3
```

Create and run an app:

```powershell
.\wevoa.exe create my-app
cd .\my-app
..\wevoa.exe start
```

Open:

```text
http://localhost:3000
```

Build and serve a production bundle:

```powershell
..\wevoa.exe build
..\wevoa.exe serve
```

Check the runtime version:

```powershell
.\wevoa.exe --version
```

## Release Builds

Windows:

```powershell
.\scripts\build-release.ps1
```

Linux:

```bash
./scripts/build-release.sh
```

Optional static-runtime attempt:

- PowerShell: `.\scripts\build-release.ps1 -StaticRuntime`
- Bash: `STATIC_RUNTIME=1 ./scripts/build-release.sh`

These produce release binaries under `dist/`.

## Language Snapshot

```text
const site = {
  framework_name: "WevoaWeb"
}

route "/" {
return view("index.wev", {
  title: "Home",
  site: site
})
}
```

Current language features:

- `let` and `const`
- integers, strings, booleans, arrays, objects, and `nil`
- arithmetic, comparisons, `&&`, `||`, indexing, and property access
- blocks, `if / else`, `loop`, `while`, `break`, and `continue`
- functions, `return`, lexical closures, and `import`
- built-in `print()`, `input()`, `view()`, `len()`, and `append()`
- `html { ... }` inline templates and `.wev` file views with `{{ expression }}`
- template control flow with `{% if %}`, `{% else %}`, `{% for %}`, and `{% end %}`
- `route` declarations with `GET` and `POST`
- native SQLite access through `sqlite.open()`, `db.exec()`, `db.query()`, and `db.scalar()`
- source-aware lexer, parser, template, and runtime diagnostics

## Runtime Flow

<p align="center">
  <img src="assets/runtime-flow.svg" alt="WevoaWeb runtime flow" width="92%" />
</p>

## What You Get In This Repo

- `lexer/`, `parser/`, `ast/`, `interpreter/`, and `runtime/`
- `server/` for route loading, HTTP handling, and static assets
- `cli/` for `start`, `create`, `build`, `serve`, and `version`
- `scripts/` for release builds and installation helpers
- `watcher/` for development reload support
- `examples/` for language samples and smoke-test scripts
- framework docs and developer references

## Installation

Windows:

```powershell
.\scripts\build-release.ps1
.\scripts\install-wevoa.ps1
```

Default install target:

- `%USERPROFILE%\bin\wevoa.exe`

Linux:

```bash
./scripts/build-release.sh
./scripts/install-wevoa.sh
```

Default install targets:

- `/usr/local/bin/wevoa` when writable
- otherwise `$HOME/.local/bin/wevoa`

After installation:

```text
wevoa --version
wevoa create my-app
```

## Documentation

- [Quick Start](docs/quickstart.md)
- [Language Reference](docs/language-reference.md)
- [Architecture](docs/architecture.md)
- [Roadmap](ROADMAP.md)
- [Changelog](CHANGELOG.md)

## Current Status

WevoaWeb is working and developer-ready, but still intentionally small. The current implementation already supports:

- an original interpreted language
- server-side HTML rendering with layouts and interpolation
- route-based web apps
- polished starter UI
- GET and POST request handling
- arrays, objects, indexing, and imports
- native SQLite-backed persistence
- static file serving from `public/`
- config loading from `wevoa.config.json`
- file watching and manual reload controls
- production build output through `wevoa build`
- built-app serving through `wevoa serve`
- runtime version reporting with `wevoa --version`
- release build scripts for Windows and Linux
- PATH installation helpers for Windows and Linux
- project scaffolding with clean backend/frontend separation

The current limitations are:

- no floating-point numbers yet
- no classes or methods yet
- no automatic browser refresh yet
- no bytecode VM or JIT yet

## Roadmap Direction

Planned areas of growth include:

- bytecode VM work
- stronger standard library primitives
- better templating and route ergonomics
- config-driven server behavior
- improved testing and diagnostics

See the full plan in [ROADMAP.md](ROADMAP.md).
