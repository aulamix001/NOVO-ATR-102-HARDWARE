Hand this whole file to Codex. Save it in the repo root as AGENTS.md (or paste it as the first prompt). Language of code, comments, commit messages and docs: English.
Build ReelGlue, an original VST3 audio plugin for Windows 10/11 x64 that emulates the behaviour of a classic 2-track mastering/mixdown open-reel tape recorder (the class of machine exemplified by the Ampex ATR-102): tape saturation and compression, head bump, selectable tape speed / head width / tape formula / emphasis EQ / calibration level, calibration controls, wow & flutter, hiss & hum, crosstalk, transformer colour, VU metering, Auto-Gain and L/R linking.

Work autonomously in milestones (section 14). After each milestone: build, run tests, commit. Do not stop at a skeleton: the end state is an installable, validated plugin.
1. Legal / trademarks. Do not use the names "Ampex", "ATR-102" or "UAD/Universal Audio" anywhere in the product, UI, file names, plugin IDs or installer. Product name: ReelGlue. Do not copy artwork, UI layouts, preset names or code from any commercial plugin. The GUI must be original and vector-drawn.
2. No GPL contamination. Do not copy code from GPL projects (e.g. ChowTapeModel is GPL-3). Implement the Jiles-Atherton model from the published literature (see section 16).
3. Real-time safety in processBlock: no allocation, no locks, no file/network I/O, no exceptions, no std::function allocation, no logging. Use juce::ScopedNoDenormals.
4. Deterministic sample-rate behaviour: supported 44.1 kHz – 192 kHz, block sizes 1 – 8192, mono/stereo.
5. Everything tunable lives in one header (Source/Dsp/Constants.h) with comments, so values can be re-tuned by ear/measurement without touching algorithms.
- Windows 10/11, Visual Studio 2022 (MSVC v143, C++20), CMake ≥ 3.22, Ninja or VS generator, Git.
- JUCE 8.x via FetchContent pinned to an exact tag (pick the latest 8.0.x tag; do not use master).
- Formats: VST3 and Standalone (Standalone is for dev/testing only).
- Test deps: Catch2 v3 via FetchContent. Validators: Steinberg validator.exe (from VST3 SDK, built by JUCE) and pluginval (Tracktion) at strictness 5+.
- Baseline CPU target: x86-64 with SSE2 (no mandatory AVX2). Optional: runtime-dispatched fast paths later.
```
ReelGlue/
├─ AGENTS.md                  (this file)
├─ CMakeLists.txt
├─ cmake/                     (helper modules)
├─ Source/
│  ├─ PluginProcessor.{h,cpp}
│  ├─ PluginEditor.{h,cpp}
│  ├─ Parameters/
│  │   ├─ ParameterIDs.h
│  │   ├─ ParameterLayout.{h,cpp}
│  │   └─ ConfigResolver.{h,cpp}      (dependency rules, section 4.2)
│  ├─ Dsp/
│  │   ├─ Constants.h                 (ALL tunables)
│  │   ├─ TapeConfig.h                (enums + resolved config struct)
│  │   ├─ TapeChannel.{h,cpp}         (per-channel chain)
│  │   ├─ EmphasisEQ.{h,cpp}
│  │   ├─ TapeNonlinearity.{h,cpp}    (Eco waveshaper + Jiles-Atherton)
│  │   ├─ HeadModel.{h,cpp}           (bump, gap, spacing)
│  │   ├─ CalibrationModel.{h,cpp}    (ideal tables, deviation mapping)
│  │   ├─ WowFlutter.{h,cpp}
│  │   ├─ NoiseGenerator.{h,cpp}      (hiss + hum)
│  │   ├─ TransformerModel.{h,cpp}
│  │   ├─ Crosstalk.{h,cpp}
│  │   ├─ VuMeter.{h,cpp}
│  │   ├─ Biquad.h, SmoothedGain.h
│  ├─ Gui/                            (LookAndFeel, Knob, Meter, ReelsComponent…)
├─ Tests/                     (Catch2: unit + DSP measurement tests)
├─ Installer/ReelGlue.iss     (Inno Setup)
├─ docs/                      (DSP notes, measurement plots, CHANGELOG)
└─ .github/workflows/windows.yml
```
cmake
```cmake
cmake_minimum_required(VERSION 3.22)
project(ReelGlue VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG <pin 8.0.x>)
FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.5.4)
FetchContent_MakeAvailable(JUCE Catch2)

juce_add_plugin(ReelGlue
  COMPANY_NAME "YourCompany"
  PLUGIN_MANUFACTURER_CODE Ycmp
  PLUGIN_CODE RgL1
  FORMATS VST3 Standalone
  PRODUCT_NAME "ReelGlue"
  VST3_CATEGORIES Fx Dynamics
  IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)
# link: juce_audio_utils juce_dsp juce_gui_extra + recommended flags
# MSVC: /W4 /permissive- /fp:fast is NOT allowed for DSP core (use /fp:precise); enable /O2 in Release.
```
Add a CMake target ReelGlueTests (console app linking the DSP sources, not the editor) and register it with CTest.
Per channel (L and R processed by two TapeChannel instances; crosstalk couples them):
```
in
 → [Power/Thru gate]
 → x (1/a0)                     ; a0 = 10^(-12/20): internal 0 VU = -12 dBFS
 → RecordGain (smoothed)
 → InputTransformer (opt.)
 → ElectronicsRail soft clip  (sets Clip LED)
 → Record EQ: Emphasis(record = inverse of replay curve) + HF EQ + Shelf EQ   [Repro & Sync paths]
 → Oversample ↑ (Quality-dependent)
 → Tape nonlinearity (flux domain; Eco shaper or Jiles-Atherton)
 → Oversample ↓
 → Head losses: gap loss LPF + spacing loss + head bump  [Repro & Sync]
 → WowFlutter (modulated fractional delay)                [Repro & Sync, if enabled]
 → + Hiss (tape+head amp)                                  [Repro & Sync; reduced on Input]
 → Repro EQ: Replay emphasis (NAB/CCIR/AES) + Repro HF + Repro LF   [Repro; Sync = limited]
 → Crosstalk mix (L↔R)  (if enabled)
 → + Hum (50/60 Hz harmonics)
 → OutputTransformer (opt.)
 → ReproduceGain (smoothed)
 → ×a0
 → Dry/Wet mix (latency-compensated dry)
 → out
```
Valid combinations (slot-based; the UI shows the formula name resolved from the slot):
Calibration levels: {0, +3, +6, +9} dB, default +6.
Manufacturer-recommended pairing (cal → flux at 0 VU):
Model: Φref(cal) = the flux column above for the cal level (0→77, +3→251, +6→355, +9→502). Each formula has a 3 % THD flux Φ3(formula) = Φref(recommended cal of that formula) × 10^(10/20) (i.e. the tape reaches 3 % THD 10 dB above 0 VU when calibrated per the manufacturer). Therefore running a formula at a lower cal than recommended gives extra headroom (cleaner), and at a higher cal pushes it into saturation (grittier).
Replay curve H(s) = (1 + s·τLF) / (1 + s·τHF); if τLF is "none" use H = 1/(1+s·τHF). Record pre-emphasis is the inverse. Normalise each to 0 dB at 1 kHz. Discretise with bilinear transform + pre-warping.
(Verify against the NAB/IEC/AES standards; keep in Constants.h.)
Peaking biquad on the replay side: f = v / λ, with λ ≈ 6.9 mm → approx.
Also add the classic low "ripple" as a shallow notch at 0.5 × f_bump (−0.5 dB, Q 2) — optional toggle in Constants.
Clamp the cutoff to 0.45 × sampleRate (and bypass if above). Add spacing loss as a very gentle 1st-order shelf cut (−1 dB @ 10 kHz at 3.75 IPS, scaling down with speed).
- Hiss floor at 15 IPS, 1/4", cal +6, formula 456: −68 dB re 0 VU (≈ −80 dBFS), pink-tilted (white noise → −3 dB/oct above 1 kHz → HP 30 Hz), injected before Repro EQ so the replay curve shapes it naturally.
- Speed offset: −3 dB × log2(speed/15) (faster = quieter).
- Head width offset: 1/4" 0 dB, 1/2" −3 dB, 1" −6 dB.
- Cal offset: −20·log10(Φref(cal)/355) dB.
- Hum: fundamental 50 or 60 Hz (per emphasis) at −90 dBFS, 2nd −96, 3rd −100, 4th −106 dBFS (random phase per instance).
- Hiss & Hum is not user-adjustable (only on/off). Fade noise in/out over 50 ms when toggled.
Peak pitch deviation at 15 IPS ≈ 0.02 % (2·10⁻⁴). Delay modulation for a sine of frequency fm: D_peak = dev / (2π·fm).
- Wow: 0.5 Hz and 1.3 Hz sines + 0.3 Hz smoothed random walk. dev ≈ 0.012 % → e.g. D ≈ 38 µs @ 0.5 Hz.
- Flutter: 7 Hz and 13 Hz sines + bandpassed noise (4–20 Hz). dev ≈ 0.012 %.
- Speed scale on dev: {30: 0.7, 15: 1.0, 7.5: 1.8, 3.75: 3.0}.
- Implementation: fractional delay line with cubic (Lagrange-4) or Hermite interpolation, fixed base delay D0 = ceil(0.5 ms · fs) plus modulation. Report D0 as latency (constant whether W&F is on or off, so toggling never shifts timing). Crossfade parameter changes over 20 ms.
- Not user-adjustable (only on/off).
Not adjustable (on/off only). Bleed gain −52 dB, low-passed at 5 kHz, tiny phase inversion; applied L→R and R→L on the post-tape signal. Does not vary with speed/head (only its tonal result does).
- Input: HPF 1st-order @ 12 Hz; low-band saturation: split with LR2 @ 180 Hz, low' = tanh(1.6·low)/1.6 (mild, adds 3rd harmonic in bass), recombine; HF −0.3 dB @ 20 kHz (gentle shelf).
- Output: same structure, drive 1.3, HPF @ 8 Hz.
- "Transformer" bool disables both with a 20 ms crossfade. Input path still runs through the electronics rail clip.
Electronics hard-ish limit at +20 dB re 0 VU (soft knee 2 dB). Clip LED lights (hold 250 ms) when the rail clip engages. Tape overload never lights the Clip LED.
All params live in one AudioProcessorValueTreeState. IDs are stable forever (never rename after first release — host automation depends on them). Left/right pairs exist for Record and Reproduce.
Because the five calibration knobs have a combo-dependent ideal position, the stored default is "normalised-ideal 0.5"; the real ideal is computed (section 7). On first instantiation set them to ideal(default combo).
Never mutate the user's stored choice just because it is currently invalid; compute an effective config each block (cached, recomputed only when a param changed — use std::atomic change flags set by parameterChanged, consumed in processBlock). Rules:
```
effSpeed   = speed
effHead    = (effSpeed <= 7.5) ? Quarter : head
effEmph    = (effSpeed == 30) ? AES : (effSpeed == 3.75 ? NAB : emphasis)   // Q1
effFormula = table[effSpeed][effHead].slot[tapeSlot]
effCalFlux = fluxForCal(cal)
humHz      = (effEmph == CCIR) ? 50 : 60
```
Changing speed/head/emphasis while autoCal is on must snap the five cal knobs to ideal(effConfig) on the message thread (via parameterChanged → MessageManager::callAsync → set values with proper gesture begin/end so hosts record automation correctly).
- Link on: the left control is the master. Moving recR/repR is allowed in UI but rewrites to the left value; when Link is switched on, copy L → R (offset lost, as documented in the reference behaviour). While linked, automation is read from L only.
- Link off: channels fully independent.
- Autogain on: keep recX_dB + repX_dB = 0 per channel. When the user moves Record by Δ, set Reproduce = clamp(−Record). When the user moves Reproduce, set Record = clamp(−Reproduce). When Autogain is enabled, keep Record and set Reproduce = −Record.
- Mono-in/mono-out: L/R always linked. Mono-in/stereo-out: duplicate the input to both chains.
- Meter In/Out and Autogain are logically global even when Link is off.
All nonlinearity operates in the flux domain.

Level mapping
```
v        = sample / a0                       // 1.0 at 0 VU
Φ_in     = v · recGainLin · Φref(cal)        // recGainLin from Record knob (+ rail/transformer first)
y_flux   = tapeNL(Φ_in, formula, biasDeviation, ...)   // small-signal gain = 1
v_out    = y_flux / Φref(cal)                // unity gain at small signal, independent of cal
```
→ Cal changes headroom/saturation without changing small-signal unity gain.
```
g      = K3 / Φ3(formula)                    // K3 ≈ 0.6 initially, solved numerically (below)
y      = tanh(g·Φ + a·(g·Φ)²) / g           // a = asymmetry from bias deviation (even harmonics), |a| ≤ 0.12
```
Add frequency-dependent saturation (tape saturates HF earlier because of record emphasis): this is already provided by running the waveshaper after the record emphasis/HF EQ. Add a one-pole "memory" term (leaky integrator of |Φ|, τ ≈ 20 ms) that lowers g's effective threshold by up to 0.7 dB at sustained high level → gentle "tape compression".

Calibration target (must be enforced by a test): with a 1 kHz sine at 0 VU the 3rd-harmonic THD ≈ 0.3 %; at +10 dB re 0 VU with the recommended cal for that formula, THD (2nd–5th) = 3.0 % ± 0.3 %. Solve K3 per quality mode with a bisection routine in the test harness and hard-code the results in Constants.h.
Implement the Jiles-Atherton model for ferromagnetic hysteresis as used in real-time tape modelling (see Chowdhury, DAFx-19). Requirements:
- Parameters per formula: Ms, a, k, c, α (normalised units) stored in Constants.h, hand-tuned so that (a) the THD calibration target of 5.1 holds, (b) formula 111 / 35-90 saturate softer and earlier, GP9/900 later and harder.
- Integrate with a stable scheme (e.g. trapezoidal or RK4 on the magnetisation ODE with a Newton or fixed iteration cap of 8), 8× oversampling in High mode.
- Guard against NaN/Inf: reset state and fade if detected.
- A/B regression test comparing the High output to Normal below +4 dB re 0 VU (should match within 1 dB gain and 1 % THD).
devBias = (calBias − idealBias(combo)) · 2 ∈ [−1, +1].
- Over-bias (dev > 0): saturation threshold Φ3 × 10^(+0.8·dev/20)... (more headroom) and a softer knee (tanh drive reduced), HF shelf cut at 8 kHz of −4·dev dB ("tape compression, warmer").
- Under-bias (dev < 0): earlier/harder distortion (3rd harmonic ↑), HF boost +3·|dev| dB, asymmetry a grows, and for dev < −0.9 add random short dropouts (gain dips of 6–20 dB, 2–30 ms, Poisson rate up to 3/s).
Real calibration is relative to the tape/speed/head/emphasis combo. Model it explicitly:
- For each effConfig compute ideal[5] = ideal knob positions for {hfEq, shelfEq, reproHf, reproLf, bias} using CalibrationModel::idealFor(cfg) (smooth deterministic placeholder formulas in Constants.h, e.g. ideal = 0.5 + w_formula[f] + w_speed·log2(speed/15) + w_head[h], all weights within ±0.15, so different combos genuinely need different knob positions).
- The DSP never reads absolute knob values; it reads deviation dev_i = (knob_i − ideal_i) · 2.
- Knob → physical mapping (applied to deviation; zero deviation = flat, calibrated response):
- Auto Cal ON: snap the 5 knobs to ideal(cfg) and re-snap whenever speed, head, formula slot or emphasis changes. LED green if all five are within ε=0.002 of ideal; LED red if any differs (the user moved a knob). Moving a knob back to ideal returns the LED to green.
- Auto Cal OFF (default): knobs keep their absolute positions when the combo changes, therefore the machine can become mis-calibrated (this is intended and part of the character).
- Clicking a knob's label resets that knob to ideal(cfg).
- Warn in the UI/tooltip that enabling Auto Cal discards manual calibration (suggest saving a preset first).
- VU ballistics: 300 ms to 99 % rise, ≤ 1.5 % overshoot (critically-damped 2nd-order or standard rectifier + 300 ms RC pair). Update the UI at 30–60 Hz from an atomic written by the audio thread (no locks).
- Reference: kMeterRefDbfs = −12.0 → 0 VU when a −12 dBFS signal is applied and Reproduce is at 0 dB (see open question Q3 on sine-peak vs RMS).
- Meter In = pre-processing, Meter Out = after Reproduce gain. Thru mode: meters show input.
- Display scale −20…+6 VU; peak-hold dot optional.
- Clip LED per channel, hold 250 ms (section 3.10).
- Report total latency to the host: D0 (W&F delay base) + oversampling filter latency. Use juce::dsp::Oversampling with filterHalfBandPolyphaseIIR (Normal/Eco/High) or FIR equiphase if you want exactly linear phase — whichever you choose, report the real value and keep it constant regardless of other parameters; if quality changes, call setLatencySamples and updateHostDisplay.
- Dry path for mix is delayed by the same latency.
- All parameter-driven coefficients are smoothed (juce::SmoothedValue, 20–50 ms; ramps in dB for gains). Coefficient recomputation at control rate (e.g. every 32 samples), never per-sample trig.
- No heap allocation after prepareToPlay. Pre-allocate for max block size.
- GUI thread ↔ audio thread: atomics only.
- Support isBusesLayoutSupported: mono/mono, mono/stereo, stereo/stereo only.
- getStateInformation/setStateInformation via AudioProcessorValueTreeState::copyState XML, with a version attribute and a migration function stub.
- Ship ≥ 12 factory presets (JUCE program list) with original names, e.g.: Clean 30 IPS Master, 15 IPS Glue, 15 IPS Warm 456, Hot GP9 Drive, Over-Biased Compression, Vintage 7.5 IPS, Lo-Fi 3.75, Mono Drum Bus, No Transformer Clean, ADT Doubler (mix 50 %, wow/flutter on), Mastering Subtle, Tape Delay-less Wide. Each preset must set speed, head, slot, cal, emphasis, path, record/repro levels, and the five calibration knobs.
- Preset recall must restore link last (so L/R offsets saved while unlinked survive).
Original design, resizable (min 900×420, aspect lock, scale 75–200 %), vector-drawn (juce::Path, gradients; no bitmap dependencies), High-DPI aware.

Closed view (primary): two VU meters (L/R) with Clip LEDs; Record and Reproduce knobs (L/R, linked pairs shown as concentric or twin knobs) with 0–10 arbitrary scale ticks (display only; values are dB internally, tooltip shows dB); Autogain, Link, Meter In/Out; Speed selector (knob or 4-way switch labelled 3.75/7.5/15/30); Tape slot button (shows resolved formula name highlighted); Cal button; Head button; NAB/CCIR LEDs (both dark at 30 IPS); Path Select (Sync/Repro/Input LEDs); Thru; Power; animated reels (click capstan/reel to pause animation).

Open view (secondary): Crosstalk, Wow&Flutter, Hiss&Hum, Transformer, Auto Cal (green/red LED), HF EQ, Shelf EQ, Repro HF, Repro LF, Bias, Mix, Quality.

Interaction: double-click resets to default; Alt/Option-click label resets to 0 dB (Record) / ideal (cal knobs); fine-adjust with Shift; mouse wheel; full keyboard accessibility; host automation gestures via SliderAttachment/ButtonAttachment; all controls have tooltips.
1. Config rules: exhaustive test of every (speed, head, emphasis, slot) combination → effective config matches section 4.2/3.1.
2. Link/Autogain logic: sequences of moves leave rec+rep = 0 per channel when autogain on; link copies L→R.
3. Thru/Power: output is bit-identical to input (after latency compensation) within −140 dBFS.
4. Unity gain: −24 dBFS 1 kHz sine, Record 0, Repro 0 → output within ±0.1 dB for every combo (cal must not change small-signal gain).
5. THD calibration (5.1 targets) for every formula at recommended cal, for Eco/Normal/High.
6. Frequency response: with all cal knobs at ideal and Auto Cal ON, response at −24 dBFS is within ±0.5 dB from 100 Hz–10 kHz for 15 and 30 IPS; head bump visible within spec table ±1.5 dB; HF limits per 3.5 ±1 dB.
7. Headroom behaviour: lowering cal by 3 dB reduces THD at a fixed hot input; raising increases it.
8. Noise: measured hiss re 0 VU within ±2 dB of 3.6 formulas; hum frequency 50/60 Hz per emphasis.
9. W&F: measured pitch deviation (FM demod of a 3 kHz sine) within ±30 % of 3.7 at 15 IPS.
10. Crosstalk: hard-panned 1 kHz → opposite channel at −52 dB ±3 dB.
11. Stability: 10 min of random parameter automation + noise input at 44.1/48/96/192 kHz, block sizes {1, 17, 64, 512, 8192}: no NaN/Inf/denormals, no clicks (|Δsample| spike detector on parameter changes), CPU < 8 % of one 3 GHz core per instance in Normal @ 48 kHz stereo (record the measured value in docs).
12. State round-trip: save → load → identical parameter values and identical audio.
13. Sample-rate invariance: same test signal at 44.1 vs 96 kHz → spectrum difference < 1 dB within 20 Hz–15 kHz.
14. pluginval strictness ≥ 5 and Steinberg validator pass with zero failures.
- Local: cmake -S . -B build -G "Visual Studio 17 2022" -A x64 then cmake --build build --config Release --target ReelGlue_VST3 and ctest --test-dir build -C Release.
- GitHub Actions windows-latest: configure, build Release, run tests, run validator + pluginval (download latest release), upload ReelGlue.vst3 artifact.
- Installer: Inno Setup script installs to C:\Program Files\Common Files\VST3\ReelGlue.vst3, includes uninstaller, version from CMake. Optional code-signing step (placeholders for certificate in CI secrets; do not commit secrets).
- Produce docs/CHANGELOG.md, docs/DSP_NOTES.md (block diagrams, tuned constants, measurement plots), README.md.
- Builds clean (/W4, no warnings) on VS 2022.
- All tests of section 11 pass.
- Loads and runs in at least: Reaper, Cubase/Nuendo or Studio One, and the JUCE AudioPluginHost; automation of every parameter works; no clicks on parameter jumps; state recall correct.
- Subjective checks (document in docs/LISTENING_NOTES.md): at 15 IPS, 1/2", 456, Cal +6, Record +4 dB the mix gains density and low-end weight (head bump) without obvious distortion on a drum loop; at Record +9 dB it audibly saturates; 3.75 IPS is audibly lo-fi; Auto Cal ON vs OFF after switching formula sounds different (mis-calibration is audible); Thru is transparent.
1. M1 – Skeleton: CMake, JUCE plugin builds as VST3 + Standalone, parameter layout complete (4.1), ConfigResolver + unit tests, passthrough with latency reporting, validator passes.
2. M2 – Core audio: level mapping, Record/Reproduce gains with smoothing, Link/Autogain logic, VU meters, Power/Thru crossfades, Clip LED, mono/stereo handling.
3. M3 – Tape voicing: emphasis pairs, Eco/Normal waveshaper with THD calibration (5.1), head bump + gap loss, formula/cal/flux tables, path-select behaviours.
4. M4 – Calibration & imperfections: CalibrationModel + Auto Cal behaviour (section 6), bias model with dropouts, W&F, hiss/hum, crosstalk, transformers.
5. M5 – High quality mode: Jiles-Atherton, 8× oversampling, regression test vs Normal; performance tuning (SIMD-friendly loops, coefficient update at control rate).
6. M6 – GUI: all views, reels animation, meters, tooltips, resize, High-DPI, presets browser (prev/next + menu).
7. M7 – Presets, docs, installer, CI, release candidate, full test + validator + pluginval pass.
At the end of each milestone print a short report: what was done, test results, known issues.
- Q1 – Emphasis at 7.5 IPS: reference documentation is inconsistent (text says NAB or CCIR is selectable at 7.5/15 IPS; the combination table lists only NAB at 7.5). Default: allow both at 7.5 IPS (hardware had both circuits), behind a constant kAllowCcirAt75Ips = true.
- Q2 – Flux for formula 111 at cal +0: table says 77 nWb/m, which does not match the usual 185 nWb/m NAB reference. Default: use the table value; make it a constant so it can be changed.
- Q3 – Meter reference: is "−12 dBFS = 0 VU" a sine peak or RMS? Default: treat as RMS of a sine (sine peak −8.99 dBFS), constant kMeterRefIsRms.
- Q4 – Absolute head-bump/HF numbers in sections 3.4/3.5 are estimates; if reference measurements of any real machine/tape become available, update only Constants.h.
- Jiles & Atherton, "Theory of ferromagnetic hysteresis", J. Magn. Magn. Mater., 1986.
- J. Chowdhury, "Real-time Physical Modelling for Analog Tape Machines", DAFx-19 (for algorithm ideas only; do not copy GPL code).
- Bertram, "Theory of Magnetic Recording"; Mallinson, "The Foundations of Magnetic Recording" (head bump, gap & spacing loss).
- NAB, IEC 60094 (CCIR/IEC1, NAB/IEC2) and AES tape equalisation standards (time constants).
- Zölzer (ed.), "DAFX: Digital Audio Effects" (fractional delay, oversampling); Välimäki & Laakso on Lagrange/Hermite interpolation.
- Steinberg VST3 documentation and JUCE docs/tutorials (parameters, state, oversampling).
- Operational reference for behaviour: the publicly available manual of Universal Audio's Ampex ATR-102 plug-in (help.uaudio.com) and the scanned Ampex ATR-100/101/102/104 user & maintenance manual (vintagedigital.com.au) — behavioural reference only, d