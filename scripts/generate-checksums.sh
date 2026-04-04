#!/usr/bin/env bash
set -euo pipefail

DIST_DIR="${1:-dist}"
OUTPUT_FILE="${2:-SHA256SUMS.txt}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "$DIST_DIR" = /* ]]; then
  RESOLVED_DIST_DIR="$DIST_DIR"
else
  RESOLVED_DIST_DIR="$ROOT/$DIST_DIR"
fi

if [[ ! -d "$RESOLVED_DIST_DIR" ]]; then
  echo "Distribution directory not found: $RESOLVED_DIST_DIR" >&2
  exit 1
fi

OUTPUT_PATH="$RESOLVED_DIST_DIR/$OUTPUT_FILE"
TMP_FILE="$(mktemp)"
trap 'rm -f "$TMP_FILE"' EXIT

find "$RESOLVED_DIST_DIR" -type f ! -path "$OUTPUT_PATH" | sort | while read -r file; do
  relative_path="${file#$RESOLVED_DIST_DIR/}"
  sha256sum "$file" | awk -v rel="$relative_path" '{print tolower($1) "  " rel}'
done > "$TMP_FILE"

mv "$TMP_FILE" "$OUTPUT_PATH"
echo "[Wevoa] Wrote checksums to $OUTPUT_PATH"
