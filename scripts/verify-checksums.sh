#!/usr/bin/env bash
set -euo pipefail

CHECKSUM_FILE="${1:-dist/SHA256SUMS.txt}"
BASE_DIR="${2:-dist}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "$CHECKSUM_FILE" = /* ]]; then
  RESOLVED_CHECKSUM_FILE="$CHECKSUM_FILE"
else
  RESOLVED_CHECKSUM_FILE="$ROOT/$CHECKSUM_FILE"
fi

if [[ "$BASE_DIR" = /* ]]; then
  RESOLVED_BASE_DIR="$BASE_DIR"
else
  RESOLVED_BASE_DIR="$ROOT/$BASE_DIR"
fi

if [[ ! -f "$RESOLVED_CHECKSUM_FILE" ]]; then
  echo "Checksum file not found: $RESOLVED_CHECKSUM_FILE" >&2
  exit 1
fi

if [[ ! -d "$RESOLVED_BASE_DIR" ]]; then
  echo "Base directory not found: $RESOLVED_BASE_DIR" >&2
  exit 1
fi

(
  cd "$RESOLVED_BASE_DIR"
  sha256sum -c "$RESOLVED_CHECKSUM_FILE"
)

echo "[Wevoa] Checksum verification passed for $RESOLVED_CHECKSUM_FILE"
