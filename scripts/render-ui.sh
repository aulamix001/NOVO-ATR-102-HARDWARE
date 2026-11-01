#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/cloud-env.sh
mkdir -p out
# Select a free display instead of interfering with an existing X server.
REELGLUE_X11_DIR=$(mktemp -d /tmp/reelglue-x11.XXXXXX)
"$REELGLUE_TOOLS/sysroot/usr/bin/Xvfb" -displayfd 3 -screen 0 1800x2200x24 -nolisten tcp \
    3>"$REELGLUE_X11_DIR/display" >"$REELGLUE_X11_DIR/server.log" 2>&1 &
REELGLUE_XVFB_PID=$!
trap 'kill "$REELGLUE_XVFB_PID" 2>/dev/null || true; rm -rf "$REELGLUE_X11_DIR"' EXIT
for attempt in {1..30}; do
    [[ -s "$REELGLUE_X11_DIR/display" ]] && break
    sleep 0.1
done
if [[ ! -s "$REELGLUE_X11_DIR/display" ]]; then
    cat "$REELGLUE_X11_DIR/server.log"
    exit 1
fi
DISPLAY=":$(cat "$REELGLUE_X11_DIR/display")" build/ReelGlueRender_artefacts/Release/ReelGlueRender "$PWD/out/reelglue.png"
