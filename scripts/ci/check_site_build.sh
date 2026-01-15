#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

"${ROOT}/scripts/site/build.sh"
printf 'OK %s\n' "$(basename "$0")"

