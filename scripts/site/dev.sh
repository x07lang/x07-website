#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SITE_DIR="${ROOT}/site"
STATIC_DIR="${SITE_DIR}/static"

python3 "${ROOT}/scripts/site/gen_docusaurus_inputs.py" --repo-root "${ROOT}"

rm -rf "${STATIC_DIR}/agent" "${STATIC_DIR}/versions"
cp -R "${ROOT}/agent" "${STATIC_DIR}/agent"
cp -R "${ROOT}/versions" "${STATIC_DIR}/versions"

cd "${SITE_DIR}"
npm start
