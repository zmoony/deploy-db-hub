#!/usr/bin/env bash
# Deploy Hub restart template: Java jar + systemd
# Install: copy to <remoteBaseDir>/restart.sh and chmod +x
# Exit codes: 0=ok 1=start failed 2=config error 3=health check failed

set -euo pipefail

# --- CONFIG (edit before use) ---
SYSTEMD_UNIT="demo.service"          # systemd unit name
JAR_NAME="app.jar"                   # jar filename under current-dir
HEALTH_URL=""                        # optional, e.g. http://127.0.0.1:8080/health
HEALTH_TIMEOUT_SEC=30
# --- END CONFIG ---

APP=""; VERSION_DIR=""; CURRENT_DIR=""; ARTIFACTS=()

usage() {
  echo "usage: $0 --app <name> --version-dir <path> --current-dir <path> --artifact <path> [--artifact ...]"
  exit 2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --app) APP="$2"; shift 2 ;;
    --version-dir) VERSION_DIR="$2"; shift 2 ;;
    --current-dir) CURRENT_DIR="$2"; shift 2 ;;
    --artifact) ARTIFACTS+=("$2"); shift 2 ;;
    -h|--help) usage ;;
    *) echo "unknown arg: $1"; exit 2 ;;
  esac
done

[[ -n "$VERSION_DIR" && -n "$CURRENT_DIR" && ${#ARTIFACTS[@]} -gt 0 ]] || usage
[[ -d "$VERSION_DIR" && -d "$CURRENT_DIR" ]] || { echo "directory missing"; exit 2; }

SRC="${ARTIFACTS[0]}"
[[ -f "$SRC" ]] || { echo "artifact not found: $SRC"; exit 2; }

mkdir -p "$CURRENT_DIR"
cp -f "$SRC" "$CURRENT_DIR/$JAR_NAME"

if ! systemctl restart "$SYSTEMD_UNIT"; then
  echo "systemctl restart failed: $SYSTEMD_UNIT"
  exit 1
fi

if [[ -n "$HEALTH_URL" ]]; then
  deadline=$((SECONDS + HEALTH_TIMEOUT_SEC))
  until curl -fsS "$HEALTH_URL" >/dev/null 2>&1; do
    if (( SECONDS >= deadline )); then
      echo "health check timeout: $HEALTH_URL"
      exit 3
    fi
    sleep 2
  done
fi

echo "restart ok: app=$APP unit=$SYSTEMD_UNIT"
exit 0
