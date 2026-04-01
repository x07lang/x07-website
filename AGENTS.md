# Repository Guide

## Build and check

- `python3 scripts/check_site.py --check`
- `bash scripts/ci/check_all.sh`
- `python3 scripts/check_site.py --check` also validates that `site/static/install/channels.json` and `site/static/install/channels/*.json` point at the same toolchain and minimum `x07up` version.

## Docs and installer sync workflow

- `x07` is the canonical source for released docs and installer assets. Do not hand-edit synced docs unless the website layer itself is changing.
- Preferred sync entry point for toolchain releases:
  - `python3 ../x07/scripts/open_pr_website_update.py --tag vX.Y.Z ...`
- Docs-only sync (no new `x07` release/tag):
  - `python3 scripts/sync_docs_from_toolchain.py --toolchain-repo ../x07`
  - This updates `docs/v<latest_toolchain_version>/` and `docs/latest/` (to satisfy `python3 scripts/check_site.py --check`) and regenerates `site/` inputs.
- Keep these published surfaces aligned with the released `x07` bundle:
  - `docs/`
  - `agent/`
  - `site/static/install.sh`
  - `site/static/install/channels.json`
  - `site/static/install/channels/*.json`
- After sync, run the site checks before pushing.
