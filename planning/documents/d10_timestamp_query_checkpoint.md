# D10 Timestamp Query Code Checkpoint

## Status

Date: 2026-10-01 (America/New_York).
Engine: Unreal 5.8.0-55116800+++UE5+Release-5.8.
Branch: `codex/d10-timestamp-query`.

**Code checkpoint ready; delivery card remains in progress.** The Development
Editor build and all 14 C++ automation groups pass: four D10 groups plus the
ten D08/D09 regressions. The saved test level's user-built Blueprint call is
the remaining manual checkpoint before D10 closeout and D11.

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

## Next manual step

1. Reopen the project and `/Game/Audio/PrivateTest/L_D10D11ResponseTest`.
2. In `Content/Audio/PrivateTest`, create a Data Asset of class
   `RTCQTResponseSettings`, named `DA_ResponseTest`, and save its defaults.
3. Select the visualizer. Assign `CQNRT_NoTimeToExplain_32` to Constant QNRT,
   `DA_ResponseTest` to Response Settings, Expected Band Count `32`, and
   Analyzer Channel `0`. Save the level.
4. Add a reference to that selected visualizer in the Level Blueprint. Connect
   BeginPlay to `Evaluate At Time`, set Time Seconds to `16.75`, and connect
   the actor reference to Target.
5. Break the returned evaluation result and print its fields with a Format Text
   node feeding Print String. Enable Print to Log. A useful format is:
   `D10 success={Success} time={Time} bands={Count} reason={Reason}`.
6. Compile/save the Blueprint and Play once. Require success true, time 16.75,
   observed count 32, and failure reason None. The cubes are static during D10.
7. Capture the fresh editor log, repeat at other reference timestamps, then
   save/reopen and verify the call again for the second D10 checkpoint.

The test level and settings that reference the private fixture stay under the
ignored `Content/Audio/PrivateTest` storage policy. Numeric evidence and source
remain tracked.

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
