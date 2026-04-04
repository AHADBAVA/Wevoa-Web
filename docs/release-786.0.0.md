# WevoaWeb Runtime 786.0.0

## Release Title

**WevoaWeb Runtime 786.0.0**

## Release Summary

WevoaWeb Runtime `786.0.0` is the first polished installer-first release of the platform in its current form.

This version brings together the language, runtime, HTTP server, template engine, CLI, package system, starter packs, and distribution workflow into a cleaner public release.

## Highlights

- Apache-2.0 licensed public repository
- installer-first Windows distribution path
- official starter packs: `app` and `dashboard`
- package discovery and official core packages: `@auth`, `@db`, `@utils`
- HTML-first server-rendered workflow with layouts, includes, components, and template control flow
- sessions, CSRF, middleware, dynamic route params, JSON helpers, uploads, and SQLite integration
- worker-pool HTTP server, bounded queue protection, template caching, and snapshot-based runtime startup
- improved public docs, support docs, security policy, trademark note, and open-source policy

## Included Assets

Recommended Windows downloads:

- `WevoaSetup.exe`
- `wevoa.exe`
- `SHA256SUMS.txt`

## Quick Start

After installing the runtime:

```text
wevoa --version
wevoa create dashboard app
cd app
wevoa start
```

Open:

```text
http://localhost:786
```

Expected version output:

```text
WevoaWeb Runtime 786.0.0
```

## Notes

- Default runtime port is now `786`
- The old demo projects have been replaced by official built-in starter templates
- The repository is now prepared for public open-source sharing with Apache-2.0 licensing and cleaner policy docs

## Suggested GitHub Release Text

WevoaWeb Runtime `786.0.0` is a major platform-polish release that turns the project into a cleaner installable runtime with official starter packs, package discovery, better public docs, and a stronger open-source posture.

### What’s new

- official `app` and `dashboard` starter packs
- installer-first Windows distribution flow
- package discovery with `wevoa search` and `wevoa info`
- official core packages: `@auth`, `@db`, `@utils`
- server-rendered templates with layouts, includes, directives, and components
- sessions, CSRF, uploads, middleware, JSON helpers, and route params
- bounded worker-pool server and stronger runtime startup behavior
- improved README, support docs, security policy, and open-source project policy

### Download

- `WevoaSetup.exe`: recommended for normal Windows users
- `wevoa.exe`: raw runtime binary for advanced/manual use
- `SHA256SUMS.txt`: checksums for release verification
