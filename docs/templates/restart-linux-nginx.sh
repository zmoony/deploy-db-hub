#!/usr/bin/env bash
# Deploy Hub restart template: static files + Nginx (symlink current model)
# Install: copy to <remoteBaseDir>/restart.sh and chmod +x
#
# Deploy Hub uploads to releases/<version>/ then switches current -> that directory.
# restart only validates config and reloads Nginx; no file copy (version-dir == current-dir).

set -euo pipefail

# --- CONFIG (edit before use) ---
NGINX_TEST_CMD="nginx -t"
NGINX_RELOAD_CMD="systemctl reload nginx"
# --- END CONFIG ---

APP=""; VERSION_DIR=""; CURRENT_DIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --app) APP="$2"; shift 2 ;;
    --version-dir) VERSION_DIR="$2"; shift 2 ;;
    --current-dir) CURRENT_DIR="$2"; shift 2 ;;
    --artifact) shift 2 ;;
    *) echo "unknown arg: $1"; exit 2 ;;
  esac
done

[[ -n "$VERSION_DIR" && -n "$CURRENT_DIR" ]] || { echo "missing dirs"; exit 2; }
[[ -d "$CURRENT_DIR" ]] || { echo "current dir missing: $CURRENT_DIR"; exit 2; }

# Optional sanity check: current should resolve to the same tree as version-dir
if [[ "$(readlink -f "$VERSION_DIR" 2>/dev/null || realpath "$VERSION_DIR")" != "$(readlink -f "$CURRENT_DIR" 2>/dev/null || realpath "$CURRENT_DIR")" ]]; then
  echo "warn: version-dir and current-dir differ; ensure Nginx root points at current"
fi

eval "$NGINX_TEST_CMD"
eval "$NGINX_RELOAD_CMD"

echo "nginx reload ok: app=$APP"
exit 0
