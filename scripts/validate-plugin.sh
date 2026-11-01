#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/cloud-env.sh
mkdir -p out/validation
REELGLUE_VALIDATOR="$REELGLUE_TOOLS/vst3sdk-build/bin/Release/validator"
REELGLUE_PLUGINVAL="$REELGLUE_TOOLS/pluginval/pluginval"
if [[ ! -x "$REELGLUE_VALIDATOR" || ! -x "$REELGLUE_PLUGINVAL" ]]; then
    echo 'Validators are not installed. See docs/VALIDATION.md.' >&2
    exit 1
fi
REELGLUE_X11_DIR=$(mktemp -d /tmp/reelglue-validation.XXXXXX)
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
export DISPLAY=":$(cat "$REELGLUE_X11_DIR/display")"
"$REELGLUE_PLUGINVAL" --strictness-level 5 --random-seed 42 \
    --sample-rates 44100,48000,96000,192000 --block-sizes 1,17,64,512,8192 \
    --vst3validator "$PWD/scripts/vst3-validator.sh" --output-dir "$PWD/out/validation" \
    --validate "$PWD/build/ReelGlue_artefacts/Release/VST3/ATR-102.vst3"
