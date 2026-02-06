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
curl -fsSL "${BASE}/agent/latest/examples/catalog.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/packages/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/catalog/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/catalog/capabilities.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/stdlib/index.json" >/dev/null
curl -fsSL "${BASE}/agent/latest/skills/x07-run/SKILL.md" >/dev/null

python3 - "${BASE}" <<'PY'
import json
import sys
import urllib.error
import urllib.request
from urllib.parse import urljoin

base = sys.argv[1].rstrip("/") + "/"
agent_latest_base = urljoin(base, "agent/latest/")


def get_json(url: str) -> dict:
    with urllib.request.urlopen(url, timeout=20) as resp:
        return json.loads(resp.read().decode("utf-8"))


def check_http_200(url: str) -> None:
    try:
        with urllib.request.urlopen(url, timeout=20) as resp:
            if resp.status != 200:
                raise RuntimeError(f"{url}: expected 200 got {resp.status}")
    except urllib.error.HTTPError as e:
        raise RuntimeError(f"{url}: expected 200 got {e.code}") from e
    except Exception as e:
        raise RuntimeError(f"{url}: request failed: {e}") from e


index = get_json(urljoin(base, "agent/latest/index.json"))

index_urls = [
    index["schemas_index_url"],
    index["skills_index_url"],
    index["examples_index_url"],
    index["examples_catalog_index_url"],
    index["packages_index_url"],
    index["catalog_index_url"],
    index["stdlib_index_url"],
]

for rel in index_urls:
    check_http_200(urljoin(agent_latest_base, rel))


def resolve_target_url(target: str) -> str:
    if target.startswith("http://") or target.startswith("https://"):
        return target
    if target.startswith("/"):
        return urljoin(base, target.lstrip("/"))
    return urljoin(agent_latest_base, target)

def iter_index_targets(doc: dict) -> list[str]:
    out: list[str] = []
    items = doc.get("items", [])
    if not isinstance(items, list):
        return out
    for it in items:
        if not isinstance(it, dict):
            continue
        url = it.get("url")
        if isinstance(url, str) and url:
            out.append(url)
        docs_url = it.get("docs_url")
        if isinstance(docs_url, str) and docs_url:
            out.append(docs_url)
        report_schema_url = it.get("report_schema_url")
        if isinstance(report_schema_url, str) and report_schema_url:
            out.append(report_schema_url)
    return out


for rel in index_urls:
    doc = get_json(urljoin(agent_latest_base, rel))
    for target in iter_index_targets(doc):
        check_http_200(resolve_target_url(target))
PY

printf 'OK %s\n' "$(basename "$0")"
