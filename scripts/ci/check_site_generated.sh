#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "[check] generate site docusaurus inputs"
python3 "$ROOT/scripts/site/gen_docusaurus_inputs.py" --repo-root "$ROOT"

echo "[check] verify site docusaurus inputs are idempotent"
python3 "$ROOT/scripts/site/gen_docusaurus_inputs.py" --repo-root "$ROOT" --check

echo "[ok] site generated inputs"

