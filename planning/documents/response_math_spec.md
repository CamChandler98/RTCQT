# RTCQT Deterministic Response Math

## Status

This document completes delivery card D02. It is the behavior contract for the
stateless response processor described in
`planning/documents/offline_visualizer_architecture.md`.

The old `USpectrumProcessor` had the right broad idea: remove a noise floor,
normalize the signal, weight bands, shape the response, and bound the visual
output. The rebuild keeps those concepts but replaces optional, order-dependent
toggles with one fixed pipeline. It deliberately excludes interpolation,
smoothing, previous-frame buffers, reflection, widgets, and runtime saving.

## Contract Boundary

The response processor accepts one scalar analyzer value per frequency band.
The ConstantQNRT adapter is responsible for supplying a linear value in the
nominal range `[0, 1]`:

- `0` means no useful response.
- `1` means the configured input ceiling.
- Finite values below `0` and above `1` are allowed at the boundary and
  saturate during normalization.
- Decibel conversion, if the selected Unreal query API requires it, belongs in
  the ConstantQNRT adapter and must be documented by D07.

The processor returns a normalized response in `[0, 1]`. A separate range map
converts that response into scale, emissive intensity, or another bounded
visual parameter.

## Settings

| Setting | Initial default | Valid behavior |
| --- | ---: | --- |
| `NoiseFloor` | `0.1` | Finite value clamped to `[0, 1]`; non-finite becomes `0`. |
| `InputCeiling` | `1.0` | Finite value clamped to `[0, 1]`; non-finite becomes `1`. |
| `BandWeight` | `1.0` | Missing becomes `1`; negative or non-finite becomes `0`; positive values may exceed `1`. |
| `ResponseExponent` | `2.0` | Missing, non-finite, zero, or negative becomes the neutral exponent `1`. |
| `OutputMin` | consumer-specific | Missing or non-finite becomes `0`. |
| `OutputMax` | consumer-specific | Missing or non-finite becomes `1`. |

If sanitized `InputCeiling` is less than or equal to sanitized `NoiseFloor`,
the input window is invalid. The processor returns a zero response instead of
dividing by zero or silently inventing a new window.

If `OutputMin` is greater than `OutputMax`, the range mapper swaps them. Equal
bounds intentionally produce a constant output.

## Fixed Operation Order

For input `x`, noise floor `f`, ceiling `c`, band weight `w`, and response
exponent `p`:

### 1. Sanitize the analyzer input

```text
sanitizedInput = isFinite(x) ? x : 0
```

Finite negative and above-ceiling inputs remain unchanged at this stage so the
normalization rule is the only saturation point.

### 2. Normalize against the response window

When `c > f`:

```text
normalized = clamp((sanitizedInput - f) / (c - f), 0, 1)
```

When `c <= f`:

```text
normalized = 0
```

This replaces the old frame-dependent normalization against the maximum value
of the current spectrum. A fixed input window is required because the same
band at the same time must produce the same response regardless of what other
bands are present.

### 3. Apply the per-band weight

```text
weighted = clamp(normalized * sanitizedBandWeight, 0, 1)
```

`BandWeight` covers focus bands and optional high-frequency compensation. A
future settings asset may store one weight per band. If the weight array is
shorter than the analyzer output, missing entries use `1`. Extra entries are
ignored.

Weighting happens before exponentiation so a boosted band can reach the peak
response and a reduced band receives the same response curve at a lower input.

### 4. Shape the response

```text
response = clamp(pow(weighted, sanitizedExponent), 0, 1)
```

Exponent behavior:

- `p = 1` is linear.
- `p > 1` suppresses middle values and emphasizes peaks.
- `0 < p < 1` raises middle values.
- Invalid `p` becomes `1`, avoiding the surprising `pow(0, 0) = 1` case.

### 5. Map response to a visual range

After sanitizing and ordering the output bounds:

```text
mappedOutput = outputMin + (outputMax - outputMin) * response
```

Examples:

```text
meshScale = MapResponse(response, MinScale, MaxScale)
emissive  = MapResponse(response, MinEmissive, MaxEmissive)
```

The normalized response is shared by scale and material output so both remain
visually synchronized.

## Reference Pseudocode

```text
function ProcessBand(input, settings, optionalBandWeight):
    x = finite(input) ? input : 0

    floor = clamp(finite(settings.noiseFloor) ? settings.noiseFloor : 0, 0, 1)
    ceiling = clamp(finite(settings.inputCeiling) ? settings.inputCeiling : 1, 0, 1)

    if ceiling <= floor:
        normalized = 0
    else:
        normalized = clamp((x - floor) / (ceiling - floor), 0, 1)

    if optionalBandWeight is missing:
        weight = 1
    else if optionalBandWeight is non-finite or negative:
        weight = 0
    else:
        weight = optionalBandWeight

    exponent = settings.responseExponent
    if exponent is missing, non-finite, or not positive:
        exponent = 1

    weighted = clamp(normalized * weight, 0, 1)
    response = clamp(pow(weighted, exponent), 0, 1)
    return response
```

## Determinism Requirements

- The result depends only on the current input, settings, and band weight.
- The implementation stores no prior input or output.
- Evaluation order across bands does not affect any result.
- Repeated calls with equivalent inputs return equivalent outputs within the
  documented floating-point tolerance.
- Array-wide maxima or minima must not affect an individual band response.
- Frame rate, playback direction, and prior seek history must not affect the
  result.

## Golden Test Vectors

`planning/test-vectors/response_golden_vectors.csv` is the machine-readable
source of expected results. It records each intermediate stage so failures can
identify whether sanitization, normalization, weighting, exponentiation, or
range mapping changed.

The required comparison tolerance is:

```text
absolute tolerance = 0.000001
relative tolerance = 0.000001
```

The vectors cover:

- Silence and values below the floor.
- The exact noise-floor boundary.
- Midpoint and peak values.
- Above-ceiling and negative input.
- `NaN`, positive infinity, and negative infinity.
- Missing, zero, reduced, boosted, negative, and non-finite weights.
- Linear, peak-emphasizing, mid-raising, and invalid exponents.
- Invalid response windows.
- Invalid, reversed, and constant output ranges.

Run them without Unreal:

```bash
python3 planning/scripts/validate_response_vectors.py
```

The validator is a reference model, not production Unreal code. D09 must
implement the same contract in C++ and run the same vectors against that
implementation.

## Explicitly Deferred

- Attack/release smoothing.
- Linear or cubic interpolation across frames.
- Median or neighboring-band smoothing.
- Frame-relative normalization against the current spectrum maximum.
- Automatic frequency-based weight generation.
- Artistic scale and emissive defaults.

Temporal response may be added only through a seek-safe look-behind query or a
pre-baked response asset. It must not change this stateless single-sample
contract.
