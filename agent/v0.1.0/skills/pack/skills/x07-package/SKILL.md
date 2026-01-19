---
name: x07-package
description: Manage X07 project dependencies and lockfiles for reproducible builds (lock, publish). Designed for autonomous agents.
metadata:
  short-description: X07 deps + lockfile management
  version: 0.1.0
  kind: docs
---

# x07-package

This skill documents the single canonical workflow for dependency management in X07 using the built-in package manager (`x07 pkg ...`).

## Official registry onboarding (x07.io)

The official registry is hosted at `https://x07.io`:

- Browse packages: `https://x07.io/packages`
- Index URL: `sparse+https://registry.x07.io/index/`

To publish to the official registry, you need a bearer token:

1. Sign in with GitHub at `https://x07.io/settings/tokens`
2. Create a token with the `publish` scope (copy it now; you won’t be able to see it again)
3. Ensure your GitHub account has a verified email (publishing requires it)

Do not share tokens between people.

## When to use

Use this skill when:
- adding/updating dependencies,
- generating/verifying lockfiles,
- publishing a package to an index/registry.

## Canonical commands

- Add a dependency (edits `x07.json`):
  - `x07 pkg add <name>@<version>`

- Generate or update a project lockfile:
  - `x07 pkg lock --project x07.json --index <url>`

- Verify a lockfile is up to date (CI mode):
  - `x07 pkg lock --project x07.json --check`

- Pack a package directory deterministically:
  - `x07 pkg pack --package <dir> --out <out.tar>`

- Login (store credentials for an index):
  - `x07 pkg login --index <url>`
  - `printf '%s' "$X07_TOKEN" | x07 pkg login --index <url> --token-stdin`

- Publish:
  - `x07 pkg publish --package <dir> --index <url>`

## Notes

- `x07 pkg login` stores credentials in `~/.x07/credentials.json` (or in `X07_PKG_HOME/credentials.json`).
- The lockfile path is controlled by `x07.json` (`lockfile`) and defaults to `x07.lock.json`.
- If dependencies are already present on disk, `x07 pkg lock` can run without `--index` using `--offline`.
