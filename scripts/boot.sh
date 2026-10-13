#!/bin/sh
set -e
SVC="${1:-all}"
DIR="${AFI_SERVICES:-services}"
case "$SVC" in
  all)
    for f in "$DIR"/*.svc; do
      [ -e "$f" ] || continue
      n=$(basename "$f" .svc)
      afi-ctl -d "$DIR" start "$n"
    done
    ;;
  *)
    afi-ctl -d "$DIR" start "$SVC"
    ;;
esac
