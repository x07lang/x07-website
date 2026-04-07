# x07-website

This repo powers [x07lang.org](https://x07lang.org): the public homepage, docs site, agent portal, versioned docs, and installer/update surfaces for the X07 ecosystem.

**Start here:** [x07lang.org](https://x07lang.org) · [Support](SUPPORT.md) · [x07 core repo](https://github.com/x07lang/x07)

## What This Repo Is

`x07-website` is the publishing layer for the X07 ecosystem. It contains:

- the public site shell and landing pages
- rendered and versioned documentation
- machine-readable agent indexes and skill packs
- install channel files and static download surfaces

The important boundary is simple: canonical toolchain docs are authored in [`x07lang/x07`](https://github.com/x07lang/x07), not here. This repo publishes and presents released content; it should not become a second source of truth for the language docs.

## When To Use This Repo

Use this repo when you need to:

- update the public homepage, navigation, or ecosystem landing pages
- sync released docs from `x07`
- regenerate Docusaurus inputs and agent indexes
- validate install channels and public site structure before publish

If the content is really a toolchain doc, schema explanation, or CLI guide, it should usually start in `x07/docs/`.

## How Content Flows

The normal release flow is:

1. update docs and installer assets in [`x07`](https://github.com/x07lang/x07)
2. build a docs bundle from that repo
3. sync the bundle into this repo
4. generate Docusaurus inputs and agent indexes
5. run the site checks and publish

That keeps `x07lang.org` aligned with released toolchain content instead of letting docs drift across repos.

## Quick Start

### Work on website-only pages

If you are editing the homepage, navigation, or other site-layer code:

```bash
cd site
npm ci
npm run start
```

### Sync docs from a toolchain bundle

```bash
python3 scripts/sync_from_bundle.py \
  --toolchain-version X.Y.Z \
  --bundle /path/to/x07-docs-bundle.tar.gz \
  --toolchain-repo ../x07
python3 scripts/site/gen_docusaurus_inputs.py --repo-root .
```

### Validate the repo

```bash
python3 scripts/check_site.py --check
bash scripts/ci/check_all.sh
```

`check_site.py` also rejects installer drift, including mismatches between `site/static/install/channels.json` and the channel-specific files under `site/static/install/channels/`.

## Repository Layout

- `site/`: Docusaurus app, landing pages, and frontend assets
- `docs/`: rendered and versioned human docs
- `agent/`: rendered and versioned machine-first docs and skill indexes
- `versions/toolchain_versions.json`: published toolchain-to-doc-bundle map
- `site/static/install/`: installer script and channel metadata served by the site
- `scripts/`: sync, generation, validation, and publish helpers

## How It Fits The X07 Ecosystem

- [`x07`](https://github.com/x07lang/x07) is the source of truth for released docs and installer assets
- [`x07-mcp`](https://github.com/x07lang/x07-mcp) provides the MCP kit and official MCP server content surfaced on the site
- [`x07-wasm-backend`](https://github.com/x07lang/x07-wasm-backend), [`x07-web-ui`](https://github.com/x07lang/x07-web-ui), and [`x07-device-host`](https://github.com/x07lang/x07-device-host) provide the browser and device application story
- [`x07-platform`](https://github.com/x07lang/x07-platform) provides the workload and lifecycle platform story
- [`x07-registry`](https://github.com/x07lang/x07-registry) and [`x07-registry-web`](https://github.com/x07lang/x07-registry-web) back the package ecosystem at [x07.io](https://x07.io)

## Related Repos

- https://github.com/x07lang/x07
- https://github.com/x07lang/x07-mcp
- https://github.com/x07lang/x07-registry
- https://github.com/x07lang/x07-registry-web
