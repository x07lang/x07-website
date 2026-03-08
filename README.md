# X07 Website

This repo hosts the X07 public website and documentation site.

Support: see `SUPPORT.md`.

Community:

- Discord: https://discord.gg/59xuEuPN47
- Email: support@x07lang.org

**Single canonical source of docs content:** `x07lang/x07/docs/`.

This repo should contain the website build system and deployment configuration, and it should consume released docs content from `x07lang/x07` (release artifacts or an automated sync).

## Repo layout

- `docs/`: website-rendered human docs (synced + versioned)
- `agent/`: machine-first docs (synced + versioned; JSON files are served as-is)
- `site/`: static site source (Docusaurus)
- `versions/toolchain_versions.json`: maps toolchain versions to synced bundles (and marks `latest`)

## Related repositories

- https://github.com/x07lang/x07 (toolchain + canonical docs source)
- https://github.com/x07lang/x07-mcp (MCP kit tooling + templates)
- https://github.com/x07lang/x07-registry (registry API)
- https://github.com/x07lang/x07-registry-web (x07.io UI)
