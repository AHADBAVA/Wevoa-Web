#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY_PATH="${1:-${ROOT_DIR}/dist/linux/wevoa}"
if [[ "${BINARY_PATH}" != /* ]]; then
  BINARY_PATH="${ROOT_DIR}/${BINARY_PATH}"
fi

if [[ ! -f "${BINARY_PATH}" ]]; then
  echo "[Wevoa] Binary not found: ${BINARY_PATH}" >&2
  echo "[Wevoa] Run scripts/build-release.sh first." >&2
  exit 1
fi

if [[ -w "/usr/local/bin" ]]; then
  INSTALL_DIR="/usr/local/bin"
elif [[ -n "${HOME:-}" ]]; then
  INSTALL_DIR="${HOME}/.local/bin"
  mkdir -p "${INSTALL_DIR}"
else
  echo "[Wevoa] Unable to determine install directory." >&2
  exit 1
fi

INSTALL_PATH="${INSTALL_DIR}/wevoa"
install -m 755 "${BINARY_PATH}" "${INSTALL_PATH}"
echo "[Wevoa] Installed runtime to: ${INSTALL_PATH}"

case ":${PATH}:" in
  *":${INSTALL_DIR}:"*)
    echo "[Wevoa] Install directory is already on PATH."
    ;;
  *)
    if [[ "${INSTALL_DIR}" == "${HOME}/.local/bin" ]]; then
      echo "[Wevoa] Add this to your shell profile:"
      echo "export PATH=\"${INSTALL_DIR}:\$PATH\""
    else
      echo "[Wevoa] Open a new shell if the command is not visible yet."
    fi
    ;;
esac

echo "[Wevoa] Try: wevoa --version"
