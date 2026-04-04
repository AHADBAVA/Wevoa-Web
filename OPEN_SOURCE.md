# WevoaWeb Open Source Policy

This document explains the public open-source posture of WevoaWeb.

## Source Code License

WevoaWeb source code is released under the terms of the [MIT License](LICENSE).

That means people may:

- use the code
- study the code
- modify the code
- redistribute the code
- use the code commercially

Subject to the MIT license requirements:

- preserve the license text
- preserve copyright notices

## Community Files

The project also ships with public community health files:

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)
- [SECURITY.md](SECURITY.md)
- [SUPPORT.md](SUPPORT.md)

These documents define how contributors, users, and maintainers are expected to collaborate.

## Security Handling

Security-sensitive reports should not be filed publicly.

Follow the process described in [SECURITY.md](SECURITY.md).

If GitHub private vulnerability reporting is enabled for the repository, that should be the preferred path.

## Release Policy

The public source repository is the source of truth for code and documentation.

Runtime binaries and installers should be distributed as release assets, not treated as normal tracked source files.

Recommended release outputs:

- `wevoa.exe`
- `WevoaSetup.exe`
- checksums
- release notes

## Data Hygiene

Generated runtime data should not be committed.

That includes:

- SQLite databases
- WAL files
- SHM files
- logs
- temp files
- user-specific local runtime artifacts

## Contribution Expectations

By contributing code, documentation, or fixes to WevoaWeb, contributors are expected to submit work they have the right to share under the repository license.

Contributors should not submit:

- code copied from incompatible sources
- private credentials
- proprietary assets without permission
- security-sensitive material in public issues or pull requests

## Product Identity

The code license does not automatically define branding policy.

If the project owner wants to reserve the use of the product name, logo, or installer branding, that policy should be read together with [TRADEMARK.md](TRADEMARK.md).

## Maintainer Review

Before calling the project "safely open source," maintainers should verify:

- license is correct
- contributors had the right to submit their code
- no secrets are in the repository or its history
- no generated databases or local state files are tracked
- public docs accurately describe support and security expectations
