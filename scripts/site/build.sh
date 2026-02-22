#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SITE_DIR="${ROOT}/site"
STATIC_DIR="${SITE_DIR}/static"

log() { printf '%s\n' "$*" >&2; }
die() { log "error: $*"; exit 2; }

[[ -d "${SITE_DIR}" ]] || die "missing site dir: ${SITE_DIR}"
[[ -f "${SITE_DIR}/package.json" ]] || die "missing ${SITE_DIR}/package.json"
[[ -f "${SITE_DIR}/package-lock.json" ]] || die "missing ${SITE_DIR}/package-lock.json (run npm install once)"
[[ -d "${STATIC_DIR}" ]] || die "missing ${STATIC_DIR}"

log "site: generate docusaurus inputs"
python3 "${ROOT}/scripts/site/gen_docusaurus_inputs.py" --repo-root "${ROOT}"

rm -rf "${STATIC_DIR}/agent" "${STATIC_DIR}/versions"
cp -R "${ROOT}/agent" "${STATIC_DIR}/agent"
cp -R "${ROOT}/versions" "${STATIC_DIR}/versions"

log "site: npm ci"
cd "${SITE_DIR}"
npm ci

log "site: npm run build"
if [[ -z "${NODE_OPTIONS:-}" ]]; then
  export NODE_OPTIONS="--max-old-space-size=6144"
else
  export NODE_OPTIONS="${NODE_OPTIONS} --max-old-space-size=6144"
fi
npm run build

[[ -d "${SITE_DIR}/build" ]] || die "missing build output: ${SITE_DIR}/build"
log "ok: site built at ${SITE_DIR}/build"
