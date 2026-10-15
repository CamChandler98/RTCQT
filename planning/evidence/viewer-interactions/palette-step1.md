# Palette step 1: native implementation and validation

Validated 2026-10-15 (America/New_York), UE 5.8, authoritative Windows checkout
`C:\Users\camch\source\RTCQT`. Branch: `codex/streamerbot-chat-interactions`.

## Delivered behavior

- `URTCQTPalettePresets` supplies editable low/high linear RGB pairs for Neon,
  Sunset, Ice, and Ember in that cycle order. Neon preserves D12's colors.
- `ARTCQTVisualizer` exposes authored/displayed color getters and runtime
  set/clear operations. Overrides are per actor, transient, and independent
  of geometry, response data, audio playback, and actor scale.
- `URTCQTPaletteComponent` is a native subobject of the Music Lab manager. It
  starts disabled and uses the four built-in defaults when no artist preset
  asset is assigned. Selection, cycling, and reset are Blueprint-callable.
- Transitions use monotonic elapsed time and smooth interpolation over
  0.5 seconds. Interrupted changes start from the colors currently displayed.
  Completed transitions stop ticking and land on the exact target.
- Reset blends to actual authored material values, then clears the override.
  Disabling restores authored colors immediately. Invalid inputs preserve
  the last valid selection. Runtime colors survive normal music evaluation,
  geometry clears, analyzer/band-count changes, and material recreation.

## Build and automation

`RTCQTEditor Win64 Development` built successfully, including UnrealHeaderTool
with warnings treated as errors. The full `RTCQT.` suite then ran headlessly
with `-NullRHI -NoSound` in `/Engine/Maps/Entry`.

The exported report recorded **32 tests: 31 successes, one success with a
warning, zero failures, zero unrun, zero in process**. The single warning was
the existing `RTCQT.MusicLab.Switching` fixture's missing optional material,
also present in the prior `Saved/Automation/Pulse/index.json` run.

All four new tests passed without warnings:

| Test | Coverage |
| --- | --- |
| `RTCQT.Palette.PresetsAndValidation` | Four defaults, cycle order, invalid IDs/names, empty catalogs, and non-finite channels in both colors. |
| `RTCQT.Palette.RuntimeColorsAndIsolation` | Per-actor colors, preserved MID/geometry/response/emissive values, actor scale preservation, invalid input, and reset without clearing unrelated parameters. |
| `RTCQT.Palette.RefreshPlaybackAndMaterialReplacement` | Seeking/evaluation, geometry clear/rebuild, 16/64/32-band playback analyzers, material recreation, actual parent defaults, incompatible materials, and duplication without runtime overrides. |
| `RTCQT.Palette.TransitionsCycleAndReset` | Opt-in setup, smooth timing, interruption continuity, exact endpoints, no-op selection, invalid requests, cycling/wrap, reset after a long frame, and disabling without starting audio. |

Existing D12 material, Music Lab, Sequencer, and Streamer.bot protocol tests
also passed. Source whitespace checks passed. The first sandboxed build
attempt stopped before compilation on UnrealBuildTool AppData cache access;
the successful native build and automation used the required cache access.

Workstation artifacts (ignored by Git):

- `Saved/Logs/Palette-build.log`
- `Saved/Logs/Palette-tests.log`
- `Saved/Automation/Palette/index.json`
- `Saved/Automation/Palette-protected-before.json`
- `Saved/Automation/Palette-protected-after.json`

## Concurrent-work preservation

Cam saved and closed Unreal before the native build. His edited pulse
Blueprint remained outside Dex's changes. SHA-256 hashes matched before and
after the build and automation:

| Asset | SHA-256 |
| --- | --- |
| `Content/Game/RTCQT/Interactions/BP_RTCQTPulseController.uasset` | `A721F894F5E19A4C75423676F28EE77AA0C099D00C0C5BA47B99BEC69D920B2A` |
| `Content/Audio/PrivateTest/MusicLab/L_MusicLab.umap` | `F3CAA8ED00F626522DDC386C0D474AF011A68D218DC62E92380F67D0AAE95EC2` |

No editor asset setup/save script ran. The automation loaded an engine entry
map and created transient fixtures.

## Integration still pending

There is no palette widget, preview-key binding, palette receiver allowlist,
or Streamer.bot palette action in this step. Human review of the four tints,
real music playback, palette changes during Cam's active pulse, and editor
restart behavior remain acceptance gates for the integrated feature. The
native scale-preservation test does not establish those human gates.

After reopening the rebuilt project, local Blueprint calls can use:

```text
Music Lab Manager -> Palette -> Set Palette Enabled(true)
Music Lab Manager -> Palette -> Select Palette("ice")
Music Lab Manager -> Palette -> Next Palette()
Music Lab Manager -> Palette -> Reset Palette()
```

## Repeat the native checks from WSL/Bash

Save and close Unreal first. From Ubuntu/WSL Bash:

```bash
cd /mnt/c/Users/camch/source/RTCQT

'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe' \
  RTCQTEditor Win64 Development \
  '-Project=C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -WaitMutex -NoHotReloadFromIDE \
  '-Log=C:\Users\camch\source\RTCQT\Saved\Logs\Palette-build.log'
```

Only after a successful build:

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' \
  'C:\Users\camch\source\RTCQT\RTCQT.uproject' /Engine/Maps/Entry \
  -unattended -nop4 -nosplash -NullRHI -NoSound \
  '-ExecCmds=Automation RunTests RTCQT.' \
  '-TestExit=Automation Test Queue Empty' \
  '-ReportExportPath=C:\Users\camch\source\RTCQT\Saved\Automation\Palette' \
  '-abslog=C:\Users\camch\source\RTCQT\Saved\Logs\Palette-tests.log'
```

Read the report contents and require all four palette tests plus the existing
suite to pass, with zero failures or unrun tests. Process exit code alone is
not the acceptance gate.
