# D11 Scale Response Code Checkpoint

## Status

Date: 2026-10-01 (America/New_York).
Engine: Unreal 5.8.0-55116800+++UE5+Release-5.8.
Branch: `codex/d11-scale-response`.

**Code checkpoint ready; delivery card remains in progress.** The Development
Editor build and all 19 automation groups pass: five D11 groups plus 14 D08-D10
regressions. The independent numeric check also passes. The user-built saved
level's visible height changes, settings edit, and fresh-editor visual check
remain the manual checkpoint before D11 closeout and D12.

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

## Next manual step

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
   Retain the fresh log and viewport evidence before marking D11 Done.

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
