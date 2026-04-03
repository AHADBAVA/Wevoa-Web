# WevoaWeb Architecture

This document gives contributors a high-level map of the codebase.

## High-Level Flow

```text
Source Code
  -> Lexer
  -> Parser
  -> AST
  -> Interpreter
  -> Route Registry / Runtime Session
  -> HTTP Server / CLI
```

## Main Layers

### `lexer/`

The lexer scans raw source text into tokens. It handles:

- keywords
- identifiers
- integer literals
- string literals
- operators and punctuation
- newline tokens for statement boundaries
- line and column tracking

### `parser/`

The parser is a recursive-descent parser. It produces AST nodes for:

- variable declarations
- expressions
- functions
- route declarations
- blocks
- loops
- conditionals
- returns

### `ast/`

The AST layer defines all expression and statement node types using a visitor-based design and `std::unique_ptr` ownership.

### `interpreter/`

The interpreter is a tree-walk runtime. It is responsible for:

- evaluating expressions
- executing statements
- managing lexical scopes
- enforcing constants
- calling native and user-defined functions
- registering and rendering routes

### `runtime/`

The runtime layer manages session lifetime and built-ins. A `RuntimeSession` retains parsed programs so closures and routes can safely reference AST-backed declarations.

### `server/`

The server layer loads view files, serves HTTP responses, and integrates static files from `public/`. It keeps the browser-side model deliberately simple: HTML plus static assets only.

### `cli/`

The CLI layer provides:

- `start`
- `create`
- `build`
- `help`

It also coordinates the dev server startup workflow.

### `watcher/`

The file watcher polls project directories and triggers reloads when project files change.

### `utils/`

Shared helpers live here, including source spans, error types, logging, keyboard input, and file-writing utilities.

## Design Principles

- Keep the language original and easy to understand.
- Prefer small, composable runtime pieces.
- Use RAII and standard library facilities where possible.
- Keep the browser delivery model server-first and HTML-first.
- Make the codebase extensible toward a VM or more advanced runtime later.
