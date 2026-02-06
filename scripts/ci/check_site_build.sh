#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

bash "${ROOT}/scripts/ci/check_site_generated.sh"
"${ROOT}/scripts/site/build.sh"

echo "[check] website repo structure"
python3 "${ROOT}/scripts/check_site.py" --check

echo "[check] agent indexes are up to date"
python3 "${ROOT}/scripts/generate_agent_indexes.py" --check --agent-dir agent/latest

echo "[check] agent package indexes validate"
python3 "${ROOT}/scripts/ci/validate_agent_package_indexes.py" --check --agent-dir agent/latest

echo "[check] built agent endpoints exist"
BUILD_DIR="${ROOT}/site/build"
for rel in \
  "agent/latest/entrypoints.json" \
  "agent/latest/index.json" \
  "agent/latest/manifest.json" \
  "agent/latest/skills/index.json" \
  "agent/latest/schemas/index.json" \
  "agent/latest/examples/index.json" \
  "agent/latest/examples/catalog.json" \
  "agent/latest/examples/catalog-files/01_echo.x07.json" \
  "agent/latest/packages/index.json" \
  "agent/latest/catalog/index.json" \
  "agent/latest/catalog/capabilities.json" \
  "agent/latest/stdlib/index.json" \
  "agent/latest/skills/x07-io-streams.json" \
  "agent/latest/skills/x07-run/SKILL.md" \
; do
  [[ -f "${BUILD_DIR}/${rel}" ]] || { echo "missing ${BUILD_DIR}/${rel}" >&2; exit 2; }
done
echo "[ok] built agent endpoints"

if find "${BUILD_DIR}/agent" \( -path "*/.agent/*" -o -path "*/.codex/*" \) -print -quit | grep -q .; then
  echo "found hidden dir in build output under ${BUILD_DIR}/agent" >&2
  exit 2
fi

echo "[check] built agent index URLs resolve"
bash "${ROOT}/scripts/ci/check_agent_endpoints_local.sh"

printf 'OK %s\n' "$(basename "$0")"
