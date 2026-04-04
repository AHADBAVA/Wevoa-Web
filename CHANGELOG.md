# Changelog

All notable changes to WevoaWeb will be documented in this file.

## [786.0.0] - 2026-04-04

This release turns WevoaWeb into a much more complete installable developer platform.

### Added

- unified runtime versioning around `786.0.0`
- official starter packs: `app` and `dashboard`
- template-based project creation system
- package discovery and registry-backed package tooling
- official core packages: `@auth`, `@db`, `@utils`
- request JSON support, response helpers, route params, middleware, sessions, and CSRF
- compiled template caching and snapshot-based runtime startup
- worker-pool HTTP concurrency with bounded queue protection
- Windows installer and distribution workflow
- versioned platform documentation in `docs/v786.md`

### Changed

- default runtime port is now `786`
- public docs now reflect installer-first distribution
- README has been rewritten for clearer onboarding and public open-source use
- security, support, and contribution docs are more explicit
- starter flow now replaces scattered demo projects with official templates

### Removed

- old tracked demo projects under `examples/auth-demo`
- old tracked demo projects under `examples/dashboard-demo`
- old tracked demo projects under `examples/blog-demo`

## [0.1.0] - 2026-04-03

Initial public open-source release.

### Added

- original WevoaWeb language core in modern C++17
- lexer, parser, AST, interpreter, and runtime session architecture
- functions, closures, routes, loops, conditionals, and constants
- built-in `print()` and `input()`
- HTTP development server with HTML-only responses
- static file serving from `public/`
- developer CLI with `start`, `create`, `build`, and `help`
- file watching, manual reload, and graceful shutdown
- starter project generator
- open-source community files and contributor docs
