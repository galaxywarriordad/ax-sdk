#!/usr/bin/env bash
set -euo pipefail

EXPECTED_TAG="v1_36_2"
REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
REF="${1:-HEAD}"
OUT_DIR="${2:-${REPO_ROOT}/dist}"

cd "$REPO_ROOT"

if ! git rev-parse --verify "${REF}^{commit}" >/dev/null 2>&1; then
  printf "[FAIL] Invalid git ref: %s\n" "$REF" >&2
  exit 1
fi

REPO_NAME="$(basename "$REPO_ROOT")"
SHORT_SHA="$(git rev-parse --short=12 "$REF")"
CLEAN_TAG="$(git describe --tags --exact-match "$REF" 2>/dev/null || printf "%s" "$SHORT_SHA")"
ARCHIVE_BASE="${REPO_NAME}-${CLEAN_TAG}"
ARCHIVE_PATH="${OUT_DIR}/${ARCHIVE_BASE}.tar.gz"
CHECKSUM_PATH="${ARCHIVE_PATH}.sha256"

if [[ -f "${REPO_ROOT}/CMakeLists.txt" ]] && grep -qiE 'ax_sdk|ax-sdk' "${REPO_ROOT}/CMakeLists.txt"; then
  if ! grep -qE "GIT_TAG[[:space:]]+${EXPECTED_TAG}([[:space:]]|\))" "${REPO_ROOT}/CMakeLists.txt"; then
    printf "[FAIL] CMakeLists.txt does not pin ax-sdk to %s\n" "$EXPECTED_TAG" >&2
    exit 1
  fi
  printf "[PASS] Verified %s SDK tag pin in CMakeLists.txt\n" "$EXPECTED_TAG"
fi

mkdir -p "$OUT_DIR"

git archive --format=tar --prefix="${ARCHIVE_BASE}/" "$REF" | gzip -9 > "$ARCHIVE_PATH"

if command -v sha256sum >/dev/null 2>&1; then
  (cd "$OUT_DIR" && sha256sum "$(basename "$ARCHIVE_PATH")" > "$(basename "$CHECKSUM_PATH")")
elif command -v shasum >/dev/null 2>&1; then
  (cd "$OUT_DIR" && shasum -a 256 "$(basename "$ARCHIVE_PATH")" > "$(basename "$CHECKSUM_PATH")")
else
  printf "[FAIL] Neither sha256sum nor shasum found\n" >&2
  exit 1
fi

printf "[PASS] Created snapshot: %s\n" "$ARCHIVE_PATH"
printf "[PASS] Wrote checksum:  %s\n" "$CHECKSUM_PATH"
