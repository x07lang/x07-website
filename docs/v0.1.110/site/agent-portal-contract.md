# Agent portal contract

The `/agent` surface is **machine-first**: agents should consume JSON endpoints, and the HTML pages are only for discoverability.

Humans looking for conceptual docs should start at `/docs/` and use `/agent` when they need the stable machine contracts directly.

## Stable entrypoints

- `GET /agent/latest/entrypoints.json`
- `GET /agent/latest/index.json`
- `GET /agent/latest/manifest.json`
- `GET /agent/latest/schemas/index.json`
- `GET /agent/latest/skills/index.json`
- `GET /agent/latest/examples/index.json` (compact starter set)
- `GET /agent/latest/examples/catalog.json` (full recursive `docs/examples/**` catalog)
- `GET /agent/latest/packages/index.json`
- `GET /agent/latest/catalog/index.json`
- `GET /agent/latest/catalog/capabilities.json`
- `GET /agent/latest/stdlib/index.json`

## Human docs

- [Agent workflow](../getting-started/agent-workflow.md)
- [Diagnostics & repair](../toolchain/diagnostics-repair.md)
