#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

bash "${ROOT}/scripts/ci/check_site_generated.sh"
"${ROOT}/scripts/site/build.sh"
printf 'OK %s\n' "$(basename "$0")"
