#!/bin/sh
set -e
DIR="${AFI_SERVICES:-services}"
if command -v afi-ctl >/dev/null 2>&1; then
  for n in $(afi-ctl -d "$DIR" dry-run 2>/dev/null | tac); do
    afi-ctl -d "$DIR" stop "$n" || true
  done
else
  echo "afi-ctl not found" >&2
  exit 1
fi
