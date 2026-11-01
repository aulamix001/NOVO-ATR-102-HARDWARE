# Validation record

## ATR-102 visual refinement

The renamed ATR-102 Release VST3 and Standalone were rebuilt and the Catch2 suite rerun successfully. Native closed/expanded/resized rendering and the existing audio/state/control integration checks passed. pluginval v1.0.4 strictness 5, including the official Steinberg validator, passed again on `ATR-102.vst3` across the complete sample-rate/block-size matrix below. The host scanner reports **ATR-102**.

Current screenshots: [closed](screenshots/atr-102.png) and [expanded](screenshots/atr-102-expanded.png). Knobs now use individual grip flutes, domed faces, machined rings and raised markers; keys use recessed sockets, beveled molded caps and latch travel.

## Baseline checks

Validated in the managed Debian 13 x86-64 environment on 2026-11-01. These results apply to the Linux development build, not Windows binaries or the complete physical-model specification.

| Check | Result |
|---|---|
| Release VST3 + Standalone build | Passed with GCC 14.2 and JUCE 8.0.15 |
| Incremental build and reusable setup script | Passed; no source compiler warnings in final build |
| Catch2 DSP suite | 5 cases, 1,627,826 assertions passed |
| Native UI integration | Closed, expanded and resized snapshots passed |
| Processor integration | Audio processing, state parameter restoration, Link/Auto Gain and Auto Cal passed |
| Steinberg VST3 validator, SDK v3.8.1_build_84 | 47 tests passed, 0 failed |
| pluginval v1.0.4, strictness 5, seed 42 | Passed including official validator and GUI tests |
| pluginval processing/automation matrix | 44.1 / 48 / 96 / 192 kHz × 1 / 17 / 64 / 512 / 8192 samples passed |
| Windows MSVC / installer / commercial hosts | Not executed; workflow and Inno Setup script provided |
| Ten-minute automation, calibrated THD/response, subjective listening | Not executed; full release criteria remain open |

## Reproduce in the prepared cloud machine

```bash
cd /workspace/NOVO-ATR-102-HARDWARE
bash scripts/setup-cloud.sh
bash scripts/render-ui.sh
bash scripts/validate-plugin.sh
```

UI images and validator reports are written under `out/`. Representative images are retained under `docs/screenshots/`.

The validator script needs the prepared tools at `/workspace/tooling/pluginval/pluginval` and `/workspace/tooling/vst3sdk-build/bin/Release/validator`. To rebuild them in a new machine after the main cloud setup:

```bash
source scripts/cloud-env.sh
mkdir -p /workspace/tooling/pluginval
curl -fsSL https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip -o /workspace/tooling/pluginval/pluginval.zip
unzip -o /workspace/tooling/pluginval/pluginval.zip -d /workspace/tooling/pluginval
chmod +x /workspace/tooling/pluginval/pluginval

git clone --depth 1 --branch v3.8.1_build_84 https://github.com/steinbergmedia/vst3sdk.git /workspace/tooling/vst3sdk
git -C /workspace/tooling/vst3sdk submodule update --init --depth 1 base cmake pluginterfaces public.sdk
cmake -S /workspace/tooling/vst3sdk -B /workspace/tooling/vst3sdk-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/workspace/tooling/sysroot/usr \
  -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=OFF -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF \
  -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF -DSMTG_CREATE_PLUGIN_LINK=OFF
cmake --build /workspace/tooling/vst3sdk-build --target validator --parallel 3
```

Keep the inherited authenticated proxy, TLS verification and Debian signature verification enabled. Use the platform-approved network execution context when needed. Clone only if the tool checkout is absent; preserve existing checkouts and local changes.

pluginval 1.0.4 passes the inner `.so` to an external validator on Linux. SDK 3.8.1 requires the outer `.vst3` directory. `scripts/vst3-validator.sh` normalizes that argument and invokes the real validator without skipping tests. The first combined run failed on this path mismatch; the corrected combined run passed.

Publication, restoration in a fresh cloud task, Windows CI execution and installation have not been verified.
