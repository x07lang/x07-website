# X07 Website

This repo hosts the X07 public website and documentation site.

**Single canonical source of docs content:** `x07lang/x07/docs/`.

This repo should contain the website build system and deployment configuration, and it should consume released docs content from `x07lang/x07` (release artifacts or an automated sync).

## Repo layout

- `docs/`: website-rendered human docs (synced from `x07lang/x07/docs/`)
- `agent/`: machine-first docs (schemas/contracts/skills; synced)
- `versions.json`: maps toolchain versions to docs bundles

