# DSP implementation and validation

## Current path

Input → smoothed Record → electronics rail → optional transformer soft clip → normalized record emphasis and calibration shelves → flux-domain tanh tape model → speed-dependent low pass and resonant head bump → replay emphasis and calibration shelves → cubic interpolating tape delay → optional noise → smoothed Reproduce → latency-aligned wet/dry → stereo crosstalk → output.

All scalar modeling constants are in `Source/Dsp/Constants.h`. Delay storage is allocated in `prepare`; `process` uses no locks, heap allocation or I/O. The processor uses `ScopedNoDenormals`. Main coefficients update on parameter changes. Supported buses are mono/mono, mono/stereo and stereo/stereo.

The wow base delay is `ceil(0.5 ms × sampleRate)` and remains present for bypass. Power and Thru fade to delayed dry. Saturation is scaled by the selected calibration flux and the resolved formula's recommended flux. Formula names are slot-resolved; low speeds force quarter-inch heads; 30 IPS resolves to AES. User selections are not destructively rewritten by those dependency rules.

## Remaining specification work

- The current model is a development soft-saturation voice. It has not been calibrated to the specified THD targets. High and Eco use the same core; Jiles–Atherton and oversampling are not implemented yet.
- Record/replay emphasis and calibration shelves are implemented as provisional filters. Standards-matched equalisation, sync-head response, bias dropouts and transformer band splitting remain unvalidated or unimplemented.
- Hiss and hum are approximations, not a measured noise model. The input meter is a block peak estimate, while output metering uses an RMS envelope.
- Paired parameter updates issue host gestures on a processor-owned message-thread timer, including with the editor closed. Audio-side Link/Auto Gain enforcement is immediate; paired stored parameter reflection can lag by one timer interval (25 ms).
- Meters and reel animation are timer driven. Pausing animation never alters audio.
- The document supplied in AGENTS.md omits several referenced numeric tables. The formula slot table is provisional and must be reviewed against an authoritative specification.

## Release gates

Run the Catch2 suite and the processor/render integration check. These exercise configuration resolution, delayed transparent bypass, calibration headroom direction, finite randomized processing and deterministic rendering. They do not substitute for calibrated DSP measurements, a ten-minute automation matrix, pluginval strictness ≥5, the Steinberg validator, real Windows host checks or listening tests.
