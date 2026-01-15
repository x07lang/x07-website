#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

"${ROOT}/scripts/ci/check_site_inputs.sh"
"${ROOT}/scripts/ci/check_site_build.sh"
printf 'OK %s\n' "$(basename "$0")"

