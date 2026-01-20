# Agent portal contract

The `/agent` surface is **machine-first**: agents should consume JSON endpoints, and the HTML pages are only for discoverability.

## Stable entrypoints

- `GET /agent/latest/entrypoints.json`
- `GET /agent/latest/index.json`
- `GET /agent/latest/manifest.json`
- `GET /agent/latest/schemas/index.json`
- `GET /agent/latest/skills/index.json`
- `GET /agent/latest/examples/index.json`
- `GET /agent/latest/stdlib/index.json`

## Human docs

- [Agent workflow](/docs/getting-started/agent-workflow/)
- [Diagnostics & repair](/docs/toolchain/diagnostics-repair/)
