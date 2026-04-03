# Quick Start

This guide gets you from source checkout to a running WevoaWeb app.

The repository also includes a bundled sample app in `Test_project/`, while generated projects use the same layout inside their own folder.

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
    runtime/template_engine.cpp runtime/config_loader.cpp runtime/sqlite_module.cpp `
    server/http_types.cpp server/web_app.cpp server/http_server.cpp server/dev_server.cpp `
    watcher/file_watcher.cpp `
    utils/logger.cpp utils/keyboard.cpp utils/file_writer.cpp `
    -o wevoa.exe -lws2_32 -lsqlite3
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
    runtime/template_engine.cpp runtime/config_loader.cpp runtime/sqlite_module.cpp \
    server/http_types.cpp server/web_app.cpp server/http_server.cpp server/dev_server.cpp \
    watcher/file_watcher.cpp \
    utils/logger.cpp utils/keyboard.cpp utils/file_writer.cpp \
    -o wevoa -lsqlite3
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

To run the bundled sample app from this repository:

```powershell
cd .\Test_project
..\wevoa.exe start
```

## 4. Edit Backend and Frontend

Generated projects use:

- `app/` for backend route and database code
- `views/` for templates and layouts
- `public/` for static assets
- `storage/` for SQLite files

Routes now live in `app/main.wev`.

`app/main.wev`

```text
import "shared.wev"

route "/" {
return view("home.wev", {
  title: "Home",
  site: site,
  env: config.env
})
}
```

`views/home.wev`

```text
extend "layout.wev"

section content {
<section class="hero-panel">
  <h1>{{ title }}</h1>
  <p>{{ message }}</p>
</section>
}
```

## 5. Reload

While the dev server is running:

- press `R` to reload manually
- press `Q` to quit
- use `Ctrl+C` for graceful shutdown

The file watcher also reloads when files in `app/`, `views/`, or `public/` change.
