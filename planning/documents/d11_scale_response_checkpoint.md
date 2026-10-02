# D11 Scale Response Checkpoints

## Status

Implementation: 2026-10-01 (America/New_York).
Visual verification and closeout: 2026-10-02.
Implementation checkpoint: `11985ef`.
Engine: Unreal 5.8.0-55116800+++UE5+Release-5.8.
Branch: `codex/d11-scale-response`.

**Status: Done.** The Development Editor build and all 19 automation groups
pass: five D11 groups plus 14 D08-D10 regressions. The independent numeric
check, user-built scene's visible height response, exponent-change check,
and fresh-editor Play/render verification all pass. D12 is unblocked.

## Implemented behavior

`EvaluateAtTime` keeps D10's exact-time query and validation. It snapshots
`URTCQTResponseSettings` on every evaluation and passes the normalized bands
through the D09 processor. Each resulting response maps to the authored
`MeshZScaleRange`, whose default is absolute local Z scale `[0.1, 4]`.

Each frame starts with newly generated D03 base transforms. Position, rotation,
and X/Y scale stay at their base values; Z is replaced by the bounded mapping,
not multiplied by the previous frame or base Z. Actor transforms apply once
through Unreal's normal component transform. Signed, reversed, constant, and
non-finite output bounds retain D02's existing map semantics.

The actor uses `BatchUpdateInstancesTransforms` in local space, updates custom
data response slot `0`, preserves normalized band-index slot `1`, then marks
render state dirty once. Rebuild logic remains limited to changed/missing
layout topology. Response settings edits, asset replacement, and timestamp
seeks update existing instances without recreating the actor or component.

All response, band, layout-transform, and applied-transform arrays are local
to the call. No clock, smoothing, previous response, or previous scale is
stored. A transform/custom-data failure clears instances and explicitly
returns `bSucceeded=false`, even if the earlier analyzer query succeeded.
The public actor API, settings fields, and runtime module dependencies do not
change.

## Automated evidence

| Test group | Verified behavior |
| --- | --- |
| `RTCQT.D11.Scale.KnownValuesAndLocalTransforms` | Independent known D02 values, silence/peak/midpoint heights, absolute Z, base X/Y/position/rotation, actor transform once, response and band-index custom data. |
| `RTCQT.D11.Scale.SeekingAndSettingsEdits` | Exact stored-matrix equality after reordered/repeated seeks, exponent/weight/range edits, settings asset replacement, and zero add/remove/clear instance events. |
| `RTCQT.D11.Scale.BoundsAndFailureRecovery` | Reversed, constant, non-finite, and signed ranges; invalid response settings; bounded finite stored transforms; missing settings and count mismatch clear old heights and recover exactly. |
| `RTCQT.D11.Scale.SingleBandAndLayoutRefresh` | One band at the base origin, normalized index zero, changed layout/count rebuild correctly, unchanged rebuild preserves response, clear/re-evaluate restores height. |
| `RTCQT.D11.LocalFixture.D07ScaleReferences` | Actual private 32-band analyzer at all six D07 timestamps; 256 applied transform rows across eight reordered queries; exact repeated matrices and no topology changes. |
| All `RTCQT.D08`, `RTCQT.D09`, and `RTCQT.D10` groups | Existing layout, response, exact-time query, validation, cleanup, and real-asset regressions. |

The real query order is `16.75 -> 0.75 -> 44.25 -> 16.75 -> 2 -> 11.75 ->
33.25 -> 16.75`. Captures use default response settings, line spacing `100`,
origin `(0,0,0)`, and base scale `(0.5,0.5,0.5)`; applied Z replaces `0.5`.

`planning/scripts/validate_d11_scale_capture.py` independently derives the
expected response and Z scale from the D07 baseline with the D02 defaults.
It checks all 256 rows, position, X/Y scale, band ordering, finite values,
query coverage, and exact repeated printed values. Absolute and relative
tolerances are `1e-6`; maximum response difference is about `3.061e-8` and
maximum Z-scale difference about `2.141e-7`. The C++ repeatability checks
compare every stored matrix element exactly within the same runtime run.

The licensed-fixture test is discoverable only where the ignored analyzer
exists. It actually ran at this checkpoint. The four synthetic groups require
no licensed audio.

All 19 groups report zero test warnings and errors. The full engine log has
one startup warning about an incompatible saved editor-layout key, which
Unreal updates before running the tests; the retained excerpt includes it.
There are no error, ensure, or fatal entries in that run.

Evidence:

- [Successful build excerpt](../evidence/d11/checkpoint1-build-log.txt).
- [Automation report](../evidence/d11/checkpoint1-automation.json) and
  [automation log excerpt](../evidence/d11/checkpoint1-automation-log.txt).
- [Applied transform captures](../evidence/d11/checkpoint1-captured-transforms.csv).
- [Independent numeric validation](../evidence/d11/checkpoint1-capture-validation.json).
- [Source/evidence fingerprints and verification record](../evidence/d11/checkpoint1-verification.json).

The successful build links the runtime object compiled in the preceding
attempt; that attempt caught a test-only matrix type mismatch. The corrected
suite uses the engine's actual `FMatrix` storage type, and the final build
compiles the tests and links the module successfully. Full generated logs and
the native report stay under `Saved`. UTC report dates show 2026-10-02 for
this October 1 evening session.

## Saved-level visual and settings verification

The user supplied the first 16.75 Play screenshot, then separate timestamp
runs at 0.75, 44.25, and 16.75 with two further screenshots captured on
October 1 at 23:14:42 and 23:15:52 (America/New_York). Each active Play session
returns success, 32 bands, and reason None with no runtime warning/error.
The Blueprint's cosmetic print prefix remains `D10`; its existing call uses
the rebuilt D11 `EvaluateAtTime` implementation.

The saved settings asset was still at exponent 1 during final inspection.
A native Unreal commandlet loaded the actual level and recorded all 32
transforms at that setting. Codex restored and saved exponent 2 as planned,
retaining the exponent-1 asset under ignored `Saved/Automation/D11` storage.
All 32 interior-band heights were higher at exponent 1; restoration preserves
position and X/Y and matches the initial exponent-2 reference within `1e-6`
(maximum printed-capture Z difference about `4.929e-9`).

The user's latest reopened session loaded the default map rather than the test
level. Final persistence/visual proof therefore comes from a fresh rendered
Unreal process running [the saved-frame capture script](../scripts/capture_d11_saved_frame.py).
It loads the actual saved test level and restored settings, starts Play,
verifies one visualizer/component and 32 instances at 16.75, and records a
successful result with reason None. After ending Play, it evaluates the same
isolated frame on the editor actor, requires exact equality with the played
transforms, and exports the viewport screenshot. It does not save the level
or settings. The final rendered view was reviewed and retains the expected
ordered row and differing heights. Camera/framing differences mean no
pixel-for-pixel equality is claimed for the user screenshots.

Evidence:

- [Initial Play view](../evidence/d11/checkpoint2-first-frame.png) and
  [initial result record](../evidence/d11/checkpoint2-first-frame.json).
- [User capture at 23:14:42](../evidence/d11/checkpoint2-user-231442.png) and
  [user capture at 23:15:52](../evidence/d11/checkpoint2-user-231552.png).
- [User timestamp/session log](../evidence/d11/checkpoint2-user-sessions-log.txt).
- [Exponent-1 saved-level inspection](../evidence/d11/checkpoint2-exponent1-inspection.json),
  [restored inspection](../evidence/d11/checkpoint2-restored-inspection.json), and
  [settings comparison](../evidence/d11/checkpoint2-settings-comparison.json).
- [Fresh Play and matching capture transforms](../evidence/d11/checkpoint2-fresh-play.json),
  [restored rendered frame](../evidence/d11/checkpoint2-restored-frame.png), and
  [native verification log](../evidence/d11/checkpoint2-fresh-verification-log.txt).
- [Completion record and fingerprints](../evidence/d11/checkpoint2-verification.json).

All 12 source/evidence fingerprints from code checkpoint 1 still match. No
runtime or C++ test source changed during closeout, so the successful 19-group
suite was not repeated. The targeted actual-asset inspection, exponent
restoration, numeric comparison, and fresh rendered Play check ran separately.
The initial visual record retains its historical `Now` status; the completion
record above establishes `Done`.

## Manual walkthrough for reproduction

1. Reopen `C:\Users\camch\source\RTCQT\RTCQT.uproject` and load
   `/Game/Audio/PrivateTest/L_D10D11ResponseTest`.
2. Keep the existing visualizer assignments and Blueprint wiring. In Format
   Text, change the printed prefix from `D10` to `D11`. Keep Time Seconds
   `16.75`, compile the Blueprint, and save the level.
3. Play once. Expect `D11 success=true time=16.75 bands=32 reason=None`.
   The cubes should have different heights. This call applies one frozen
   frame; audio playback and sequence-time updates arrive with D13.
4. Capture the viewport, then stop Play. Run separate Play sessions at `0.75`,
   `44.25`, then `16.75`, compiling/saving each changed timestamp. The repeated
   16.75 view should match the first view.
5. Stop Play. Open the assigned `DA_ResponseTEst` asset, change Response
   Exponent from `2` to `1`, save it, and Play at 16.75. Interior band responses
   should rise while the configured bounds remain respected. Stop, restore
   exponent `2`, save, and verify the original 16.75 frame returns.
6. Stop, Save All, close/reopen Unreal, reload the level, and Play at 16.75.
   Retain the fresh log and viewport evidence for the verification record.

The private level, response asset, analyzer, and audio stay ignored under
`Content/Audio/PrivateTest`; source and numeric evidence are tracked.

## Reproduction

Save and close the interactive editor before rebuilding. Use Bash/WSL:

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe' \
  RTCQTEditor Win64 Development \
  '-Project=C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -WaitMutex -NoHotReloadFromIDE \
  '-Log=C:\Users\camch\source\RTCQT\Saved\Logs\D11-build.log'
```

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' \
  'C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -unattended -nop4 -nosplash -NullRHI -NoSound \
  '-ExecCmds=Automation RunTests RTCQT.' \
  '-TestExit=Automation Test Queue Empty' \
  '-ReportExportPath=C:\Users\camch\source\RTCQT\Saved\Automation\D11' \
  '-abslog=C:\Users\camch\source\RTCQT\Saved\Logs\D11-tests.log'
```

Inspect `Saved/Automation/D11/index.json` for every test state and aggregate
failed/unrun counts; process exit alone does not establish a pass. Validate
the retained capture independently from the project root:

```bash
python3 -B planning/scripts/validate_d11_scale_capture.py
```

To validate a fresh generated capture, pass
`--capture Saved/Automation/D11/captured-transforms.csv`.

To repeat the saved-level rendered check with exponent 2 already saved, run:

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' \
  'C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -EnablePlugins=PythonScriptPlugin \
  '-ExecutePythonScript=C:\Users\camch\source\RTCQT\planning\scripts\capture_d11_saved_frame.py' \
  -unattended -nop4 -nosplash -NoSound \
  '-abslog=C:\Users\camch\source\RTCQT\Saved\Logs\D11-fresh-visual.log'
```

This requires a graphics device and exits after writing
`Saved/Automation/D11/saved-frame-restored.png` and its JSON report.
