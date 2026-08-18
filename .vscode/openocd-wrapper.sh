#!/usr/bin/env bash
set -euo pipefail

if [[ "${OSTYPE:-}" == linux* ]]; then
  exec /usr/bin/openocd "$@"
fi

if command -v openocd >/dev/null 2>&1; then
  exec openocd "$@"
fi

echo "OpenOCD was not found on this system." >&2
exit 127
