# X07 Website

## Agent Entrypoint

Start here: https://x07lang.org/docs/getting-started/agent-quickstart

This repo hosts the X07 public website and documentation site.

Support: see `SUPPORT.md`.

Community:

- Discord: https://discord.gg/59xuEuPN47
- Email: support@x07lang.org

**Single canonical source of docs content:** `x07lang/x07/docs/`.

This repo contains the website layer for the X07 ecosystem: the public homepage, the rendered docs site, the agent portal, and the deployment-ready static site output path. It consumes released docs content from `x07lang/x07` instead of inventing a second documentation source of truth.

## Vision

The vision for `x07-website` is straightforward:

- one clear public entry point for people learning X07
- one canonical docs experience for both end users and coding agents
- one place that explains the whole ecosystem in simple language, then links people to the right repo or product surface

This repo is where the "whole" of X07 becomes visible. The core toolchain, MCP kit, WASM stack, platform story, registry, and docs all meet here in one public-facing experience.

## How It Fits The X07 Ecosystem

`x07-website` is the presentation layer for the rest of the system:

- [`x07`](https://github.com/x07lang/x07) is the source of truth for released docs and installer assets
- [`x07-mcp`](https://github.com/x07lang/x07-mcp) provides the MCP kit and official MCP server content that the site should explain
- [`x07-wasm-backend`](https://github.com/x07lang/x07-wasm-backend) and [`x07-web-ui`](https://github.com/x07lang/x07-web-ui) provide the WASM and app-building story surfaced on the site
- [`x07-platform`](https://github.com/x07lang/x07-platform) provides the lifecycle platform story surfaced on the site
- [`x07-registry`](https://github.com/x07lang/x07-registry) and [`x07-registry-web`](https://github.com/x07lang/x07-registry-web) back the package ecosystem at [`x07.io`](https://x07.io)

The important boundary is that this repo presents and publishes those stories, but does not become the authorship source for toolchain docs.

## Practical Usage

Use this repo when you need to:

- update the public homepage and navigation for the X07 ecosystem
- publish synced docs from a released X07 docs bundle
- regenerate the Docusaurus inputs from synced docs content
- verify that public site structure, agent endpoints, and versioned docs stay consistent

## Install And Use It Standalone

If you only want to work on the website layer:

1. clone this repo and the sibling `x07` repo
2. sync docs from a released or locally built X07 docs bundle
3. generate Docusaurus inputs
4. run the site checks and build the site

For website-only work, the main things you need are Python 3 and Node.js.

## Repo layout

- `docs/`: website-rendered human docs (synced + versioned)
- `agent/`: machine-first docs (synced + versioned; JSON files are served as-is)
- `site/`: static site source (Docusaurus)
- `versions/toolchain_versions.json`: maps toolchain versions to synced bundles (and marks `latest`)

## Use It As Part Of The X07 Ecosystem

The normal ecosystem workflow is:

1. update docs and installer assets in [`x07`](https://github.com/x07lang/x07)
2. build a docs bundle from the core repo
3. sync that bundle into this repo with the provided scripts
4. verify the generated website inputs and publish the updated site

That keeps the public site aligned with the released toolchain instead of letting docs drift across repos.

## Common Commands

Sync from a docs bundle:

```sh
python3 scripts/sync_from_bundle.py \
  --toolchain-version X.Y.Z \
  --bundle /path/to/x07-docs-bundle.tar.gz \
  --toolchain-repo ../x07
python3 scripts/site/gen_docusaurus_inputs.py --repo-root .
```

Validate the repo structure:

```sh
python3 scripts/check_site.py --check
```

That check also rejects installer drift. If `site/static/install/channels.json` says stable points to one toolchain and `site/static/install/channels/stable.json` points to another, CI fails before publish.

Run the site locally:

```sh
cd site
npm ci
npm run start
```

## Related repositories

- https://github.com/x07lang/x07 (toolchain + canonical docs source)
- https://github.com/x07lang/x07-mcp (MCP kit tooling + templates)
- https://github.com/x07lang/x07-registry (registry API)
- https://github.com/x07lang/x07-registry-web (x07.io UI)
