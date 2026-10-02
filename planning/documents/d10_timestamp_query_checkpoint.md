# D10 Timestamp Query Checkpoints

## Status

Date: 2026-10-01 (America/New_York).
Engine: Unreal 5.8.0-55116800+++UE5+Release-5.8.
Branch: `codex/d10-timestamp-query`.

**Status: Done.** Implementation checkpoint: `0be38f5`.
The Development Editor build and all 14 C++ automation groups pass: four D10
groups plus the ten D08/D09 regressions. The user-built Blueprint call in the
saved test level passes at reordered timestamps and after closing/reopening
Unreal. D11 is unblocked; response-driven mesh scale remains its next task.

## Implemented behavior

`EvaluateAtTime` accepts exactly the caller's float timestamp. It validates
finite time, assigned analyzer and analyzed sound, response settings, mesh,
positive expected count, analysis duration, and queryable channel data. Times
zero and the exact duration are accepted; negative, beyond-duration, and
non-finite times fail. The verified fixture duration is 60 seconds.

The module-private `RTCQTAnalysis::QueryBandsAtTime` helper calls
`GetNormalizedChannelConstantQAtTime` with the unchanged time and channel.
It replaces its output with exactly the configured band count or clears the
output on failure. Missing settings, unready result data, and unavailable
channels report `EmptyAnalyzerResult` before calling Unreal's native query.
A count mismatch preserves the actual observed count in the returned result.

The actor clears stale instances on failure and restores the static layout
on a subsequent valid evaluation. Warnings occur only when entering a different
failure. Missing optional material uses the mesh default. If the member
component reference is lost, cleanup finds the actor-owned `RTCQTInstances`
default subobject so attached geometry is still cleared.

A successful D10 evaluation validates the analyzer frame and static topology.
D11 will consume the call-local band array to apply response and Z scale.
The actor retains only layout identity and failure bookkeeping; there is no
stored song clock, previous spectrum, or previous response.

The private `AudioAnalyzer` and `AudioSynesthesiaCore` dependencies are explicit
because UE 5.8 needs their import libraries for native result metadata and test
fixture construction. No editor module is added. Synthetic analyzer fixtures
and result installation are guarded by editor automation build flags.

## Automated evidence

| Test group | Verified behavior |
| --- | --- |
| `RTCQT.D10.Query.ExactTimeAndOrder` | Known native spectra at endpoints and intermediate timestamps, unchanged requested time, exact repeatability after reordered calls. |
| `RTCQT.D10.Query.InvalidInputs` | Missing analyzer/sound, bad times/counts/channels/duration/settings, empty/unfinished analysis, cleared output, observed mismatch count, recovery. |
| `RTCQT.D10.Visualizer.FailuresAndRecovery` | Actor failure reasons, warning suppression, cleared instances, recovery, invalid layout/missing component, material fallback, fixed count/component, disabled tick. |
| `RTCQT.D10.LocalFixture.D07ReferenceValues` | Real 32-band analyzer at all six D07 timestamps, backward/forward/repeated calls, start/end acceptance, actor result fields. |
| All `RTCQT.D08` and `RTCQT.D09` groups | Existing layout, lifecycle, and response regression coverage. |

The private-fixture comparison verifies 192 reference pairs at six timestamps
across 15 queries. Its 480 captured rows match the logged D07 normalized values
within `1e-6`; repeated queries reproduce every float exactly in the same run.
The C++ report records a maximum difference of zero after parsing both sides
as floats. Captures printed at nine significant digits also pass an independent
CSV comparison at `1e-6`.

The fixture test is discoverable only when the ignored analyzer asset exists.
It actually ran at this checkpoint, as recorded in the retained automation
report. The three synthetic D10 groups require no licensed audio.

Evidence:

- [Successful build excerpt](../evidence/d10/checkpoint1-build-log.txt).
- [Automation report](../evidence/d10/checkpoint1-automation.json) and
  [test log excerpt](../evidence/d10/checkpoint1-automation-log.txt).
- [Captured normalized values](../evidence/d10/checkpoint1-captured-values.csv).
- [Verification record](../evidence/d10/checkpoint1-verification.json).

Generated full logs remain in `Saved/Logs/D10-build.log` and `D10-tests.log`.
The native Unreal report is `Saved/Automation/D10/index.json`. Assess the
report's failed/unrun counts and each test state; process exit alone does not
establish successful automation. Fingerprints use UTF-8 bytes with CRLF
normalized to LF. Report timestamps are retained in their original UTC format.

## Saved-level Blueprint verification

Manual closeout: 2026-10-01 (America/New_York). Native Unreal log timestamps
are UTC and therefore show 2026-10-02 for this evening session.

The user created the settings asset, assigned it and the 32-band analyzer to
the existing D08 visualizer in `/Game/Audio/PrivateTest/L_D10D11ResponseTest`,
and wired BeginPlay to `Evaluate At Time` and a logged result. The actual local
settings asset name is `/Game/Audio/PrivateTest/DA_ResponseTEst`.

| Session | Requested seconds | Succeeded | Observed bands | Failure reason |
| --- | ---: | --- | ---: | --- |
| Before reopening | 16.75 | true | 32 | None |
| Before reopening | 0.75 | true | 32 | None |
| Before reopening | 44.25 | true | 32 | None |
| Before reopening | 16.75 | true | 32 | None |
| Fresh editor, saved level | 16.75 | true | 32 | None |

The log records saving the final 16.75 Blueprint, closing the editor, starting
a fresh editor session, loading the saved level, and the final successful Play
result. No Blueprint compilation was needed between reopening and the result.
Each verified Play session has zero warnings/errors during its active lifetime.
A temporary enum-node compilation error occurred while the user was wiring the
conversion; it was corrected before these successful runs. Editor startup and
shutdown messages are not part of the runtime-query warning count.

These manual timestamps run in separate Play sessions. The unchanged C++
automation supplies the stronger same-actor order-independence and exact band
repeatability checks; the Blueprint logs establish asset assignment, callable
runtime behavior, result fields, and saved-level persistence.

Evidence:

- [Manual result and reopen log excerpt](../evidence/d10/checkpoint2-manual-log.txt).
- [Completion record and private-asset fingerprints](../evidence/d10/checkpoint2-verification.json).

The ten checkpoint 1 source/evidence fingerprints still match. No C++ or test
source changed during closeout, so the successful 14-group automation was not
rerun for these documentation and evidence changes.

## Blueprint setup for reproduction

1. Open `/Game/Audio/PrivateTest/L_D10D11ResponseTest`.
2. In `Content/Audio/PrivateTest`, right-click **Data -> Data Asset** in UE 5.8,
   select class `RTCQTResponseSettings`, name it `DA_ResponseTest`, and save.
   An existing asset of that class can be reused.
3. Select the visualizer. Assign `CQNRT_NoTimeToExplain_32` to Constant QNRT,
   the settings asset to Response Settings, Expected Band Count `32`, and
   Analyzer Channel `0`. Save the level.
4. Add a reference to the selected visualizer in the Level Blueprint. Connect
   BeginPlay to `Evaluate At Time`, set Time Seconds to `16.75`, and connect
   the actor reference to Target.
5. Break the returned evaluation result. Feed Succeeded, Evaluated Time
   Seconds, and Observed Band Count into Success, Time, and Count arguments
   of a Format Text node. Convert Failure Reason with **Enum to String**
   before connecting it to Reason; Format Text does not directly accept
   the custom failure enum. Use this format:
   `D10 success={Success} time={Time} bands={Count} reason={Reason}`.
6. Connect the evaluation execution output to Print String, connect the
   formatted text to its string input, and enable Print to Log.
7. Compile/save and Play once. Require success true, time 16.75, count 32,
   and reason None. The cubes remain static during D10.
8. Stop Play, change the timestamp, compile/save, and Play separately at
   `0.75`, `44.25`, then `16.75`. Save All, close/reopen Unreal, reload this
   level, and Play again at 16.75.

The test level and settings that reference the private fixture stay under the
ignored `Content/Audio/PrivateTest` storage policy. Numeric evidence and source
remain tracked. D11 will apply the processor result to Z scale.

## Reproduction

Save and close the interactive editor before rebuilding. Use Bash/WSL:

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe' \
  RTCQTEditor Win64 Development \
  '-Project=C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -WaitMutex -NoHotReloadFromIDE \
  '-Log=C:\Users\camch\source\RTCQT\Saved\Logs\D10-build.log'
```

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' \
  'C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -unattended -nop4 -nosplash -NullRHI -NoSound \
  '-ExecCmds=Automation RunTests RTCQT.' \
  '-TestExit=Automation Test Queue Empty' \
  '-ReportExportPath=C:\Users\camch\source\RTCQT\Saved\Automation\D10' \
  '-abslog=C:\Users\camch\source\RTCQT\Saved\Logs\D10-tests.log'
```
