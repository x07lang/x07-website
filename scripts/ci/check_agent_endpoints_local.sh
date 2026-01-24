#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT}/site/build"

[[ -d "${BUILD_DIR}" ]] || { echo "missing build dir: ${BUILD_DIR}" >&2; exit 2; }

PORT="${X07_WEBSITE_LOCAL_PORT:-8123}"

cd "${BUILD_DIR}"
SERVER_LOG="$(mktemp -t x07-website-httpserver.XXXXXX)"
python3 -m http.server "${PORT}" --bind 127.0.0.1 >"${SERVER_LOG}" 2>&1 &
SERVER_PID="$!"
cleanup() {
  kill "${SERVER_PID}" >/dev/null 2>&1 || true
  wait "${SERVER_PID}" >/dev/null 2>&1 || true
  rm -f "${SERVER_LOG}" >/dev/null 2>&1 || true
}
trap cleanup EXIT

BASE="http://127.0.0.1:${PORT}"

for _ in {1..50}; do
  if curl -fsSL "${BASE}/agent/latest/index.json" >/dev/null 2>&1; then
    break
  fi
  if ! kill -0 "${SERVER_PID}" >/dev/null 2>&1; then
    echo "local server exited early; log:" >&2
    tail -n 50 "${SERVER_LOG}" >&2 || true
    exit 2
  fi
  sleep 0.1
done

if ! curl -fsSL "${BASE}/agent/latest/index.json" >/dev/null 2>&1; then
  echo "local server did not start; log:" >&2
  tail -n 50 "${SERVER_LOG}" >&2 || true
  exit 2
fi

curl -fsSL "${BASE}/agent/latest/entrypoints.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/manifest.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/schemas/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/skills/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/examples/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/packages/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/catalog/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/catalog/capabilities.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/stdlib/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/skills/x07-build-run/SKILL.md" >/dev/null

printf 'OK %s\n' "$(basename "$0")"
