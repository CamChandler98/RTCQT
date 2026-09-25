# D09 Stateless Response Processor

## Result

**Status: Done.** C++ build and automation run: 2026-09-24 (America/New_York).
Evidence and roadmap finalized: 2026-09-25.
Engine: Unreal 5.8.0-55116800+++UE5+Release-5.8.
Implementation checkpoint: `d591d54` on `codex/d09-response-processor`.

`FRTCQTResponseProcessor` now converts each normalized ConstantQNRT control
value into a finite response in `[0, 1]` using the fixed D02 operation order.
All 32 D02 reference cases pass against C++. The combined automation run passes
all six D09 and four D08 test groups, with zero test errors or warnings.

This completes the math required by D11. D10 still supplies exact-time analyzer
queries, D11 applies response to instance scale, and D12 supplies material
output. This test run verifies the processor and D08 regressions; it does not
establish audio synchronization, moving cubes, or OBS output.

## Implementation review

- `RTCQTResponseProcessor.cpp` sanitizes input and settings, validates the input
  window, normalizes, applies the indexed weight, clamps before exponentiation,
  then bounds the response. D07 normalized analyzer values are the intended
  input; no decibel conversion or spectrum-wide normalization occurs here.
- `ProcessBands` preserves index order and replaces the destination with one
  output per input. Empty input clears prior output. A temporary result array
  keeps reads valid when the input view or weight array aliases the destination.
  This introduces one transient result buffer, with no retained frame history.
- `MapResponse` sanitizes and orders the output bounds. A direct non-finite
  response uses zero; finite responses clamp to `[0, 1]`. Double intermediates
  and stable interpolation preserve finite results and exact endpoints across
  extreme float ranges. Equal bounds yield a constant output.
- The processor accepts plain values and arrays. It retains no UObject,
  renderer, playback clock, previous frame, or mutable global state. The
  existing `MakeProcessorParameters` method copies authored values and weights;
  changing the asset afterward leaves an earlier snapshot independent.
- Public method signatures, module dependencies, and the saved D08 level are
  unchanged. The D02/API documents clarify direct range-map input handling and
  batch aliasing without changing the valid-input operation order.

## Verification

| Check | Evidence |
| --- | --- |
| Development Editor build succeeds | [Build excerpt](../evidence/d09/build-log.txt) |
| Ten test groups succeed; none fail or remain unrun | [Automation report](../evidence/d09/automation.json), [log excerpt](../evidence/d09/automation-log.txt) |
| Silence, midpoint, peaks, operation order, replacement, and empty batches | `RTCQT.D09.Response.BasicCalculations` |
| All 32 authoritative D02 CSV cases: normalization, weighting, response, and mapping | `RTCQT.D09.Response.GoldenVectors` |
| Missing/extra weights, invalid indices, overlapping arrays, preserved separate input | `RTCQT.D09.Response.BatchAndWeights` |
| Reordered and interleaved calls, independence from another band's extrema | `RTCQT.D09.Response.OrderIndependence` |
| Non-finite values, tiny valid windows, extreme weights/exponents/ranges, exact map endpoints | `RTCQT.D09.Response.ExtremeValuesAndBounds` |
| Asset edits cannot mutate an existing parameter snapshot | `RTCQT.D09.Response.SettingsSnapshot` |
| Existing line vectors, invalid layout inputs, safe failures/recovery, topology/cache | All four `RTCQT.D08` groups in the same run |
| Source, vector, and retained evidence fingerprints | [Verification record](../evidence/d09/verification.json) |

The golden test reads `planning/test-vectors/response_golden_vectors.csv`
directly. Expected values come from the CSV, not another C++ implementation.
Absolute and relative tolerances are both `1e-6`. Separate determinism checks
require exact float equality when input and settings repeat in the same run;
this is not a claim of bit-identical results across every platform or compiler.

The standalone Python D02 reference validator also passed all 32 cases before
the implementation checkpoint. Production correctness is established by the
Unreal C++ suite above.

The automation report has no test warnings. The full editor log has one startup
warning about an incompatible saved editor-layout key; Unreal replaces that
key before tests run. The retained log excerpt includes it. No error, ensure,
or fatal entries occurred, and the log records an exit status of zero.

The retained build excerpt is the final incremental build that compiled the
expanded test suite and linked it with the unchanged processor from `d591d54`.
The source and golden CSV fingerprints use UTF-8 text with CRLF normalized to
LF, so Windows checkout line-ending conversion does not invalidate the record.
Full generated logs and the native Unreal report remain under `Saved` and are
not versioned.

## Reproduction

Save and close the interactive editor before rebuilding the project DLL. Use
Bash/WSL with the installed Windows Unreal 5.8 toolchain:

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe' \
  RTCQTEditor Win64 Development \
  '-Project=C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -WaitMutex -NoHotReloadFromIDE \
  '-Log=C:\Users\camch\source\RTCQT\Saved\Logs\D09-build.log'
```

Run the combined D08/D09 C++ suite:

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' \
  'C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -unattended -nop4 -nosplash -NullRHI -NoSound \
  '-ExecCmds=Automation RunTests RTCQT.' \
  '-TestExit=Automation Test Queue Empty' \
  '-ReportExportPath=C:\Users\camch\source\RTCQT\Saved\Automation\D09' \
  '-abslog=C:\Users\camch\source\RTCQT\Saved\Logs\D09-tests.log'
```

Inspect `Saved/Automation/D09/index.json` for every test's state and the aggregate
failure/unrun counts. The `RTCQT.` filter covered ten groups at this checkpoint;
future cards may add more. To isolate this card, use `RTCQT.D09` as the filter.
These tests require no private audio asset or graphics capture.
