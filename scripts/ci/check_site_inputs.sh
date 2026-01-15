#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

python3 "${ROOT}/scripts/check_site.py" --check
printf 'OK %s\n' "$(basename "$0")"

