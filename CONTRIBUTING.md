# Contributing to WevoaWeb

Thanks for your interest in contributing.

## Project Goals

WevoaWeb is building an original server-side language and web platform with a small, understandable implementation. Good contributions usually improve one of these areas:

- language design
- parser or interpreter correctness
- runtime behavior
- HTTP and template system quality
- starter templates and developer experience
- documentation
- tests and reproducibility

## Before You Start

- Read the [README](README.md), [Quick Start](docs/quickstart.md), and [Roadmap](ROADMAP.md).
- Check whether the feature already exists or is already planned.
- Prefer focused pull requests instead of large unrelated changes.
- If your change affects syntax, runtime behavior, or starter output, update docs with it.

## Development Setup

Build a release binary on Windows:

```powershell
.\scripts\build-release.ps1
```

Build a release binary on Linux:

```bash
./scripts/build-release.sh
```

If you want the direct compiler commands, use the current commands in the [README](README.md) or [Quick Start](docs/quickstart.md), since those are kept closer to the active runtime surface.

## Local Verification

Useful verification commands:

```powershell
.\wevoa.exe --version
.\wevoa.exe create my-app
cd .\my-app
..\wevoa.exe doctor
..\wevoa.exe start
```

For the richer starter:

```powershell
.\wevoa.exe create dashboard admin-panel
```

## Contribution Guidelines

- Keep code readable and modular.
- Prefer modern C++17 style and RAII-based resource management.
- Avoid introducing heavy dependencies.
- Preserve the language's original design goals.
- Update documentation when behavior changes.
- Update starter templates when framework behavior changes affect first-run UX.
- Do not commit generated databases, WAL files, SHM files, logs, or local runtime output.

## Pull Requests

Good pull requests usually include:

- a clear summary of the change
- the motivation for the change
- notes about behavior changes
- build or test steps used to verify it

If your change is large or changes the language surface area, open an issue or discussion first if possible.

## Security

Do not disclose security-sensitive details in a public issue or PR.

Use the process documented in [SECURITY.md](SECURITY.md).
