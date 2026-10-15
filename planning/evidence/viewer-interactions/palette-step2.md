# Palette step 2: component controls and playback persistence

Validated 2026-10-15 (America/New_York), UE 5.8, authoritative Windows checkout
`C:\Users\camch\source\RTCQT`. Status: native implementation, build, and the
full 34-test automation suite passed. Human audiovisual integration gates
remain pending.

## Implementation

The existing preset/blend foundation is extended with:

- A viewer-only request method, `TryViewerPalette`, accepting canonical
  `next`, `neon`, `sunset`, `ice`, and `ember` values. It requires the palette
  feature enabled, Viewer Palettes Enabled, an active component lifetime,
  the bound receiver to be the manager's current receiver, and status Ready.
- A 15-second monotonic cooldown per palette component, independent of
  pulse's five-second limit. Failed, invalid, and unchanged requests do not
  consume it. Local selections, resets, and feature disable/re-enable do not
  erase the last accepted viewer timestamp.
- Local selection, cycling, and reset remain usable through the viewer lock
  and disconnection. A previously accepted blend can finish after either.
- An idempotent `OnCommandReceived` subscription from BeginPlay. The handler
  ignores non-palette commands. EndPlay removes its binding, restores authored
  colors, and clears viewer session state. No receiver connection or endpoint
  settings are changed by the palette component.
- Readable cooldown time, accepted viewer count, selected ID, and Last Error
  for the later local controls and acceptance checks.
- Setup validates authored configuration even if the visualizer has not
  initialized its displayed dynamic material yet. It retains the opt-in in
  either BeginPlay order; early requests are safely rejected without consuming
  cooldown and a fresh request works after the visualizer initializes.
- Canonical payload values, palette event names, and Ready status use explicit
  case-sensitive comparison. Local named selection remains a separate API;
  the later bot action can normalize case-insensitive Twitch commands before
  emitting the canonical payload.

The source changes are confined to `RTCQTPaletteComponent.h/.cpp` and
`RTCQTPaletteTests.cpp`. Documentation records this handoff. No pulse source,
Blueprint, Music Lab level, credential file, or Streamer.bot action is edited
by Dex. Cam is working on pulse step 10, which owns the endpoint and level
settings.

## New regression tests

| Test | Passing coverage |
| --- | --- |
| `RTCQT.Palette.ViewerGateCooldownAndLifecycle` | Display material not ready at startup, pre-play rejection, unique listener, pulse/test isolation, exact canonical values/status, cooldown boundary/no-op/burst behavior, viewer lock, local bypass, cooldown preservation, disconnect/reconnect, missing receiver/preset, invalid catalogs, teardown, and a fresh session. |
| `RTCQT.Palette.MusicLabSwitchRestartAndPause` | Manager playback methods with synthetic track pairs, 16/64/32 bands, switching during a blend, same-track restart, pause/resume, invalid track preservation, natural completion, restart after completion, actor-scale preservation, and palette reset without restarting audio or advancing analysis. |

## Validation results

`RTCQTEditor Win64 Development` built successfully, including header generation
with warnings treated as errors. The final full `RTCQT.` suite ran headlessly
with `/Engine/Maps/Entry -NullRHI -NoSound` and exported **34 tests: 33 clean
successes, one success with a warning, zero failures, zero unrun, zero in
process**. All six palette tests and the existing Streamer.bot protocol test
passed without warnings. The single warning remains the existing optional
material fallback in `RTCQT.MusicLab.Switching`.

The first automation run caught uppercase payload acceptance from Unreal's
default string comparisons. Explicit case-sensitive checks fixed it, and
the final run also covers startup before the visualizer's material is ready.
The report was inspected directly; Unreal's process exit code alone did not
identify the first run's assertion failure.

Final workstation outputs (ignored by Git):

- `Saved/Logs/Palette-step2-build.log`
- `Saved/Logs/Palette-step2-tests.log`
- `Saved/Automation/PaletteStep2/index.json`
- `Saved/Automation/PaletteStep2-protected-before.json`
- `Saved/Automation/PaletteStep2-protected-after.json`

The initial failure report/log are retained as
`Saved/Automation/PaletteStep2-first-run.json` and
`Saved/Logs/Palette-step2-first-run-tests.log`. The previous 32-test report
remains under `Saved/Automation/Palette/`. Source whitespace checks passed.

## Concurrent-work preservation

Cam saved and closed Unreal before the build. SHA-256 checks matched before
and after all builds and automation for his saved pulse Blueprint, Music Lab
level, receiver header/implementation, and receiver protocol test source.
The saved asset hashes are:

| Asset | SHA-256 |
| --- | --- |
| `Content/Game/RTCQT/Interactions/BP_RTCQTPulseController.uasset` | `88539F3912A3B6FA4D5FC3DD0D357FF239CE6BD8B43D6629151D3E531688D9CF` |
| `Content/Audio/PrivateTest/MusicLab/L_MusicLab.umap` | `1B0A9099C72E186AFE92024758EFBD82E02DA16FA8393170B5533501C2F69D08` |

No asset setup/save script ran, and native automation used transient fixtures.

The tests call the component's viewer path and broadcast synthetic delegate
events. They do not establish WebSocket or Twitch palette delivery: the
receiver's palette allowlist is still deferred to implementation step 4.
They also do not establish that real music and the combined palette/pulse
effects look and sound correct. Those human checks remain pending.

## Local API for the next integration step

```text
Music Lab Manager -> Palette
  Set Palette Enabled(true)                # Overall feature opt-in
  Select Palette("ice") / Next Palette()  # Local preview
  Reset Palette()                         # Local reset to authored colors
  Set Viewer Palettes Enabled(false)      # Block new viewer palette requests
  Get Viewer Palette Cooldown Remaining() # Seconds until a new viewer change
```

The pulse Blueprint's existing L control remains pulse-specific. The new
viewer gate controls palettes only; neither is presented as a master lock.
