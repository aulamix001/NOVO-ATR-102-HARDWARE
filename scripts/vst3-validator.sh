#!/usr/bin/env bash
set -euo pipefail
# pluginval 1.0.4 sends the inner Linux .so; SDK 3.8.1 expects the bundle root.
# Normalize the location, then run every official validator test unchanged.
REELGLUE_VALIDATOR_ARGS=("$@")
for i in "${!REELGLUE_VALIDATOR_ARGS[@]}"; do
    if [[ "${REELGLUE_VALIDATOR_ARGS[$i]}" == *.vst3/Contents/x86_64-linux/*.so ]]; then
        REELGLUE_VALIDATOR_ARGS[$i]="${REELGLUE_VALIDATOR_ARGS[$i]%%/Contents/x86_64-linux/*}"
    fi
done
exec /workspace/tooling/vst3sdk-build/bin/Release/validator "${REELGLUE_VALIDATOR_ARGS[@]}"
