# WevoaWeb Framework and Language Overview

This document is the public-facing A-to-Z overview of WevoaWeb as it exists today.

It explains:

- what WevoaWeb is
- what the language can do
- what the framework can do
- what the runtime and CLI already provide
- what the strongest parts of the platform are
- what the current issues, limits, and tradeoffs still are

This is not a roadmap document. It is a current-state description of the platform.

---

## 1. Core Identity

### 1.1 What WevoaWeb Is

WevoaWeb is a server-first web platform built in modern C++17. It combines:

- a custom programming language
- a tree-walk interpreter
- a server-side template engine
- an HTTP server
- a project CLI
- a build and serve workflow
- a Windows installer path

In practical terms, WevoaWeb is not only a language and not only a web framework. It is one integrated platform where a single runtime executable handles project creation, app startup, request handling, template rendering, production output, and installation.

### 1.2 What Kind of Platform It Is

WevoaWeb sits at the intersection of:

- language runtime
- backend framework
- HTML rendering engine
- CLI-based developer platform

That makes it different from stacks where the runtime, framework, and templating system all come from separate tools.

### 1.3 What Problem It Solves

WevoaWeb is designed to reduce stack fragmentation.

Instead of forcing developers to combine:

- one language for backend logic
- one framework for routing
- one template system for HTML
- one tool for project scaffolding
- one separate runtime for execution

WevoaWeb provides one coherent model:

- define routes in `app/`
- render templates from `views/`
- serve assets from `public/`
- run the platform with `wevoa`

### 1.4 Short Definition

WevoaWeb is an original server-side scripting language and web runtime that lets developers build HTML-first web applications through one integrated platform, without depending on browser-side JavaScript for the normal application flow.

---

## 2. Current Platform Status

### 2.1 Current Version

Current runtime version:

- `786.0.0`

Version source of truth:

- `utils/version.h`

That version is already wired through:

- `wevoa --version`
- startup logs
- build manifests
- installer metadata

### 2.2 Current Maturity

WevoaWeb is now beyond the "toy prototype" stage.

It already has:

- a working language pipeline
- a working server runtime
- a compiled template system
- a CLI
- a build path
- a production serve path
- a Windows installer path

At the same time, it is still intentionally compact and still growing. It should be viewed as a serious small platform, not yet as a giant mature ecosystem.

### 2.3 Best Description of Its Current State

The best honest description today is:

WevoaWeb is a compact, original, installable web platform with a real full-stack development flow, but it is still early compared with large production runtimes that have years of ecosystem and VM work behind them.

---

## 3. What the Language Can Do

### 3.1 Core Syntax

The language currently supports:

- `let`
- `const`
- `func`
- `route`
- `component`
- `if / else`
- `loop (...)`
- `while`
- `return`
- `break`
- `continue`
- `import`
- `html { ... }`

### 3.2 Data Types

Current runtime value types:

- `integer`
- `string`
- `boolean`
- `array`
- `object`
- `nil`
- callable values

### 3.3 Operators and Expressions

Supported operators include:

- arithmetic: `+`, `-`, `*`, `/`
- comparison: `>`, `<`, `>=`, `<=`, `==`, `!=`
- logical: `&&`, `||`, `!`
- assignment: `=`
- property access: `.`
- index access: `[]`
- calls: `()`

Behavior notes:

- `+` supports string concatenation
- logical operators short-circuit
- arrays and objects participate in truthy/falsy checks

### 3.4 Control Flow

The language already supports the control flow needed for practical route logic:

- conditionals
- counted loops
- `while`
- `break`
- `continue`
- `return`

### 3.5 Functions and Closures

Functions support:

- named declarations
- parameters
- return values
- lexical closures

Closures matter because helper functions, route-adjacent logic, and middleware can all capture surrounding state naturally.

### 3.6 Scope Model

WevoaWeb uses lexical scoping.

That means:

- blocks create child environments
- functions capture the environment where they are declared
- routes capture their surrounding scope
- globals live in the root environment

### 3.7 Imports

Imports are part of the language/runtime workflow.

Supported forms:

- relative file imports
- package imports from `packages/` with `@name`
- registry-backed discovery through `wevoa search` and `wevoa info <package>`

Imports execute once per runtime session and contribute to one shared application world.

### 3.8 Components

The language also supports component declarations for template reuse:

- declared in backend source
- rendered in templates
- backed by inline HTML source

This is still a compact component model, but it is already useful for reusable view fragments.

---

## 4. What the Framework Can Do

### 4.1 Routing

Routes are first-class language constructs.

Current routing capabilities:

- `GET`
- `POST`
- `PUT`
- `PATCH`
- `DELETE`
- dynamic segments like `:id`
- route-local middleware

This is already strong enough to express normal page routes, form handlers, JSON endpoints, and protected routes.

### 4.2 Request Handling

Routes can currently access:

- request method
- request path
- request target
- query params
- form data
- JSON request bodies
- uploaded file metadata
- CSRF token

This makes WevoaWeb capable of handling normal web forms and API-style request input without needing a second request abstraction layer outside the runtime.

### 4.3 Response Handling

Routes can return:

- plain HTML/text
- rendered templates
- JSON responses
- redirects
- explicit status responses

Response helpers currently include:

- `json(...)`
- `redirect(...)`
- `status(...)`

### 4.4 Templates

The template system already supports:

- file-based rendering with `view(...)`
- `{{ expression }}`
- `{% if %}`
- `{% else %}`
- `{% for %}`
- `include`
- `extend`
- `section`
- inline `html { ... }`
- reusable components

This is a real templating system, not just string concatenation.

### 4.5 Layouts

Layouts are available through:

- `extend "layout.wev"`
- `section content { ... }`

That gives WevoaWeb a clean, reusable page-shell model that works well for server-rendered apps.

### 4.6 Sessions and Security

Framework-level security/runtime features already include:

- cookie-backed sessions
- automatic CSRF validation for mutating methods
- password hashing helpers
- password verification helpers
- secure cookie defaults with `HttpOnly` and `SameSite=Lax`

### 4.7 Static Assets

The framework serves static assets from:

- `public/`

This supports CSS, icons, images, JSON, and common static resource types.

### 4.8 Database Usage

WevoaWeb already supports practical SQL-backed app behavior through SQLite.

That means the framework can already power:

- form-backed apps
- small dashboards
- CRUD pages
- session-backed protected pages

---

## 5. What the Runtime Can Do

### 5.1 Full Language Pipeline

WevoaWeb already has the full language pipeline:

```text
source
  -> lexer
  -> parser
  -> AST
  -> interpreter
  -> runtime effects
```

### 5.2 Request Startup Model

The application is loaded once at startup.

After startup:

- the runtime snapshot is reused
- request execution happens in an isolated per-request runtime clone

This avoids replaying the entire app source for every request.

### 5.3 Template Compilation and Caching

Templates are compiled into internal node trees and cached.

This covers:

- file templates
- inline HTML fragments
- component bodies

The runtime supports bounded template and fragment caches.

### 5.4 Concurrency Model

The HTTP runtime now uses:

- a worker pool
- a bounded request queue
- `503` queue overflow protection

That makes the platform substantially more stable under load than a one-request-at-a-time prototype.

### 5.5 Session Lifecycle Control

The session system now includes:

- in-memory storage
- mutex protection
- TTL cleanup
- maximum session count
- eviction behavior

### 5.6 Performance Instrumentation

The runtime can already track:

- request time
- route time
- template render time
- database time

This gives WevoaWeb a useful observability baseline for development and production checks.

### 5.7 Production Build Path

The runtime already supports:

- `wevoa build`
- `wevoa serve`

with production metadata artifacts such as:

- `wevoa.build.json`
- `wevoa.snapshot.json`
- `wevoa.templates.json`

That is an important sign that WevoaWeb is now a platform, not just a dev-only interpreter.

---

## 6. What the CLI Can Do

Current commands:

- `wevoa create`
- `wevoa start`
- `wevoa build`
- `wevoa serve`
- `wevoa doctor`
- `wevoa info`
- `wevoa migrate`
- `wevoa make:migration`
- `wevoa install`
- `wevoa remove`
- `wevoa list`
- `wevoa search`
- `wevoa info <package>`
- `wevoa --version`

### 6.1 Why This Matters

This means the CLI is already the operational control surface of the platform.

It handles:

- project creation
- development runtime
- production packaging
- production serving
- migration workflows
- package workflows
- diagnostics
- version reporting

That is a strong product-level capability set.

---

## 7. What the Distribution Story Looks Like

WevoaWeb is now distributable as:

- `wevoa.exe`
- `WevoaSetup.exe`

Recommended user path:

1. install through `WevoaSetup.exe`
2. open a new terminal
3. run `wevoa --version`
4. run `wevoa create my-app`
5. run `wevoa start`

This gives WevoaWeb a real installable runtime story rather than a "clone the repo and build it yourself" story only.

---

## 8. Positive Points and Strengths

### 8.1 Architectural Strengths

- Original implementation from lexer to installer path
- Clear subsystem separation
- One executable controls most of the platform surface
- Language, runtime, framework, CLI, and installer stay internally consistent
- Modular C++ structure that is readable and extendable

### 8.2 Developer Experience Strengths

- Starter project generation
- Built-in dev server
- Build and serve commands
- File watching and reload
- Project inspection through `doctor` and `info`
- Global installation path through the Windows installer

### 8.3 Framework Strengths

- HTML-first model is simple and practical
- Routes are language-native
- Templates are integrated into the runtime
- Layouts, includes, loops, conditions, and components already exist
- Sessions, middleware, request helpers, and response helpers already exist

### 8.4 Runtime Strengths

- Snapshot-based request startup
- Compiled template caches
- Worker-pool concurrency
- Bounded queue control
- Worker-safe SQLite access
- Source-aware diagnostics

### 8.5 Product Strengths

- Single installable runtime
- Build output structure
- Installer support
- Unified versioning
- Global CLI usability after install

---

## 9. Current Issues, Limits, and Tradeoffs

### 9.1 Language Limits

- no floating-point type yet
- no classes or methods
- no user-defined exception system
- no optional route parameters

### 9.2 Runtime Limits

- still a tree-walk interpreter
- no bytecode VM
- no JIT
- no serialized runtime snapshot restore

### 9.3 Server Limits

- not async
- worker pool is fixed-size rather than adaptive
- extremely high-throughput workloads will still scale worse than async or VM-optimized runtimes

### 9.4 Data Layer Limits

- SQLite is the only built-in database backend
- heavy concurrent write workloads are still limited by SQLite's design
- no ORM or query builder
- no rollback command in migrations yet

### 9.5 Ecosystem Limits

- no remote package registry
- no version manager
- no official plugin marketplace
- no official network package ecosystem yet

### 9.6 Frontend and Template Limits

- component model is intentionally small
- no client-side reactivity model
- no browser auto-refresh built in yet

These are not fatal problems, but they are real boundaries that define where the platform is today.

---

## 10. Best Use Cases Right Now

WevoaWeb is especially well suited for:

- internal dashboards
- admin panels
- server-rendered product websites
- CRUD-style backoffice apps
- small to medium web products
- educational language/runtime projects
- experimental platform work

It is less well suited today for:

- extremely high-throughput APIs
- highly reactive SPA-style frontend apps
- massive plugin-driven ecosystems
- apps that require many production-grade database adapters immediately

---

## 11. Why It Is Already a Real Platform

WevoaWeb already has the key things that move a project from "interesting prototype" to "real platform":

- a real CLI
- a real server runtime
- a real template engine
- a real project structure
- a real build path
- a real serve path
- a real install path
- a real documentation base

That means the developer workflow is already complete:

1. install runtime
2. create project
3. write routes and views
4. run locally
5. build production output
6. serve production output

That is a serious platform capability.

---

## 12. Current Bottom Line

WevoaWeb today is a compact, original, installable web platform built around an integrated language/runtime/framework model.

Its biggest strengths are:

- simplicity
- clarity
- tight integration
- product-style distribution
- strong architectural coherence

Its biggest current limitations are:

- no VM/JIT
- no async runtime
- no broad remote package ecosystem
- no multiple built-in database backends

So the honest conclusion is:

WevoaWeb is already a serious, usable platform for real development work, but it is still a growing platform rather than a finished large-scale ecosystem.

---

## 13. Where to Read Next

- [Quick Start](quickstart.md)
- [Language Reference](language-reference.md)
- [Architecture](architecture.md)
- [Distribution Strategy](distribution-strategy.md)
- [Internal Platform Reference](internal-platform-reference.md)
