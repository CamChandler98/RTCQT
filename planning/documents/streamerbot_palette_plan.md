# Unreal palette switching: implementation and concurrent work plan

Prepared 2026-10-15 for `C:\Users\camch\source\RTCQT\RTCQT.uproject`, UE 5.8.
Status: steps 1 and 2 are implemented and passed native builds and the full
34-test RTCQT automation suite on 2026-10-15. Local UI, receiver palette
allowlisting, asset setup, and human visual/playback review remain later
integration work.

Cam continues [the pulse walkthrough](streamerbot_pulse_command.md). Dex builds
the Unreal palette feature alongside that work. Both features compose on the
same visualizer: palette changes its two material colors; the pulse Blueprint
changes its actor scale.

## Implementation progress

Step 1 includes the runtime material API, the four editable starter preset
definitions, and the opt-in half-second blend component on the Music Lab
manager. This also supplies the preset/component foundation described in
implementation step 2 below. A missing Preset Asset uses the four built-in
defaults; artists can supply a DataAsset later without changing source.

Four focused tests under `RTCQT.Palette.` cover catalog validation, runtime
color isolation, material/playback persistence, and interrupted transitions,
cycling, and reset. All four passed, along with the existing RTCQT tests:
31 successes, one success with the existing Music Lab material-fallback
warning, zero failures, and zero unrun tests. Dex did not edit pulse source,
Blueprints, maps, or Streamer.bot actions. Hashes of Cam's saved pulse
Blueprint and Music Lab level matched before and after native validation.
See the [step 1 evidence](../evidence/viewer-interactions/palette-step1.md).

After a successful full native build, the Blueprint API is:

```text
Music Lab Manager -> Palette
  -> Set Palette Enabled(true)
  -> Select Palette("neon" | "sunset" | "ice" | "ember")
  -> Next Palette()
  -> Reset Palette()
```

These calls are the local integration points; no preview keys or palette
widgets are installed in this step. The component starts disabled. Each
selection blends from the currently displayed colors for 0.5 seconds, and
reset clears the override after blending to the material's authored defaults.
Disable restores authored colors immediately. `Last Error` reports invalid
setup or input. The visualizer itself retains its non-ticking music pipeline.

Step 2 adds a viewer-only request path, a separate Viewer Palettes Enabled
gate, a 15-second palette cooldown, and one receiver listener per component
lifetime. Local selection/cycling/reset bypass the viewer lock and cooldown
without clearing the last accepted viewer timestamp. An accepted transition
finishes through a viewer lock or receiver disconnect. EndPlay unbinds the
receiver and clears session state. The component ignores pulse/test events.
Setup validates authored material parameters without depending on which
actor begins play first; early requests before the displayed material is
ready are rejected without changing the selection or consuming cooldown.

The new `RTCQT.Palette.ViewerGateCooldownAndLifecycle` test covers those rules.
`RTCQT.Palette.MusicLabSwitchRestartAndPause` exercises the manager's actual
track-selection, restart, pause/resume, and completion methods with synthetic
16/64/32-band audio/analyzer pairs, including a switch mid-blend. Both passed
without warnings in the full 34-test run: 33 clean successes, one success with
the existing Music Lab material-fallback warning, zero failures or unrun
tests. See the [step 2 evidence](../evidence/viewer-interactions/palette-step2.md).
The receiver still accepts only test and pulse payloads; adding its palette
allowlist remains implementation step 4.

## Starting point checked

- Branch: `codex/streamerbot-chat-interactions`; inspected HEAD: `9998d28`.
  The working tree was clean when this plan was prepared.
- The receiver already accepts `test` and `pulse/gentle`. Pulse protocol
  coverage and a pulse Blueprint asset are committed. Their presence does not
  establish that the remaining Blueprint or real Twitch checks have passed.
- D12 exposes `LowBandColor` and `HighBandColor`. The authored starter pair is
  cyan/magenta. The visualizer owns one shared dynamic material per actor.
- Every successful analysis evaluation calls `RebuildInstances`, which calls
  `RefreshMaterial`, even when the topology is cached. Runtime colors must
  survive that path and recreation of the dynamic material.
- `SetPlaybackAnalyzer` clears geometry when a track changes. Palette state
  must remain independent of that geometry and the audio component.
- The [interaction plan](viewer_chat_interactions_plan.md) already proposes
  Neon, Sunset, Ice, and Ember, half-second blends, and editable preset assets.

## Proposed behavior

| Operation | Result |
| --- | --- |
| Local palette selection | Blend to Neon, Sunset, Ice, or Ember over 0.5 seconds. |
| Local Next | Cycle Neon -> Sunset -> Ice -> Ember -> Neon. From authored defaults, first Next selects Neon. |
| Another local choice during a blend | Start from the colors currently displayed, with no color jump. Cycle relative to the last selected target. |
| Local Reset palette | Blend back to the authored material defaults and clear the runtime override. |
| Track switch, same-track restart, pause/resume, or playback completion | Keep the selected palette. A valid in-progress blend can finish while playback is paused or stopped. |
| Receiver disconnect/reconnect | Keep the palette and finish an already accepted blend; do not queue or replay commands. |
| Stop PIE and start again, or restart Unreal | Start from authored defaults; runtime selection is transient. |
| Unknown name or invalid preset data | Keep the last valid palette and cooldown state. Report a concise reason. |
| Chat palette request | Require the receiver to be Ready, viewer palettes enabled, and the independent 15-second palette cooldown clear. |
| Local controls while viewer palettes are locked | Remain available and bypass the chat cooldown. They do not reset the chat cooldown. |

Palette requests can run while music is paused or stopped. Pulse retains its
own playback requirement, five-second cooldown, and cancellation rules.
Selecting an already selected target is an unchanged result; it does not
restart the blend or consume a new cooldown.

The initial colors are editable linear RGB pairs. Neon starts from D12's
existing cyan/magenta pair; Sunset uses orange/pink, Ice uses blue/pale cyan,
and Ember uses deep red/orange. Final tint choices need Cam's visual review.
Changing palettes preserves the existing music-driven emissive range.

## Ownership while we work in the same checkout

| Owner | Files / work |
| --- | --- |
| Cam | `BP_RTCQTPulseController`, its graphs and pulse-only keys P/O/L; the pulse walkthrough and Streamer.bot pulse setup. |
| Cam | All saves to `Content/Audio/PrivateTest/MusicLab/L_MusicLab.umap` and pulse-related manager/level settings. |
| Cam until the integration handoff | `Source/RTCQT/Private/RTCQTStreamerBotComponent.cpp` and `Source/RTCQT/Private/Tests/RTCQTStreamerBotTests.cpp`, if still following the native pulse steps. |
| Dex | New palette preset, component, and test files; `RTCQTVisualizer.h/.cpp`; `RTCQTMaterialParameters.h`; `RTCQTMusicLabManager.h/.cpp`; `RTCQTMusicLabControls.h/.cpp`; dedicated palette setup/verification scripts and documentation. |
| Dex after Cam finishes native pulse edits | One additive palette allowlist change to the receiver `.cpp`, plus a separate palette protocol test and any required test access declaration in its header. |

Do not replace the pulse receiver function with an older copy after the
palette allowlist is integrated. Dex will preserve both existing commands.
Keep the existing pulse test's hard-coded counts independent by putting
palette protocol cases in a new test file with its own receiver fixture.

Concurrent source writing and Blueprint editing are workable. Native builds,
editor restarts, and saves to shared Unreal assets are serialized. Neither
person switches the shared branch, resets the checkout, stages the other's
files, or runs a broad asset regeneration during the overlap.

Cam remains the level owner through integration. Dex's scripts create or
verify palette assets only; they do not regenerate Music Lab or save its map.
The private Music Lab directory is ignored by Git, so publishing source alone
will not publish the workstation's level/preset settings. Retain a rerunnable
palette setup script and a literal setup handoff for those settings.

## Implementation sequence for Dex

### 1. Add a runtime color API to the visualizer

Add named constants for both existing color parameters and Blueprint-callable
operations to read displayed colors, set a runtime low/high pair, and clear
that override. Keep override state transient and independent for each actor.

`RefreshMaterial` reapplies the runtime pair when an override exists. With no
override, preserve the authored material values. Clearing an override restores
the authored parent values, including on an already existing dynamic material.
Read those defaults from the assigned material, rather than assuming Neon.
Reject non-finite colors and incompatible materials before mutating state.

Keep the visualizer's existing non-ticking evaluation contract. Updating a
color must not evaluate analysis, rebuild instances, change custom response
data, change transforms, restart audio, or modify the saved material/settings.
Instance rebuilding and material replacement must retain the override.

### 2. Add editable presets and the palette component

Create `RTCQTPalettePresets.h/.cpp` for a DataAsset containing stable preset
IDs, display names, and low/high color pairs. Validate missing/duplicate IDs
and invalid colors before enabling the feature.

Create `RTCQTPaletteComponent.h/.cpp` and attach it as a native default
subobject of `ARTCQTMusicLabManager`. The manager supplies its existing
Visualizer and StreamerBot references. Expose shared entry points for named
selection, cycling, resetting the palette, and enabling viewer palettes.
Default to disabled until the Music Lab instance is explicitly configured.

The component owns the current target, displayed/from/target color pairs,
blend start time, and palette chat cooldown. Use monotonic elapsed time for a
bounded smooth interpolation that reaches the exact target at 0.5 seconds.
Tick only while a transition needs display updates. The audio mixer remains
the sole source of song position.

Bind once to `OnCommandReceived` and unbind on EndPlay. Only handle palette
commands. An absent reference or invalid configuration leaves the authored
visualizer usable, with a clear setup diagnostic.

### 3. Add local controls and repeatable asset setup

Extend the native Music Lab widget with a palette dropdown, Next, Reset
palette, palette status/brief acceptance feedback, and a clearly labeled
Viewer palettes toggle. All controls use the component entry points. Keep
the existing track/playback controls and pulse keys available.

Add `planning/scripts/setup_palette_presets.py` to create the dedicated
Music Lab preset DataAsset without loading/saving the level. Reruns preserve
artist-edited colors; updating starter colors requires an explicit option.
Provide the exact asset path and instance settings in the implementation
handoff. Do not run `setup_music_lab.py` as part of palette setup.

The pulse guide's L toggle currently controls viewer pulses only. The new
toggle is labeled for palettes only. A shared master viewer lock can be wired
to both setters in a later coordinated change; neither individual switch is
presented as a master lock in this release.

### 4. Extend the Unreal command contract at the handoff

After Cam finishes editing/building the pulse receiver, add exactly:

```text
command = palette
value   = next | neon | sunset | ice | ember
```

Keep schema version 1, topic `rtcqt.visuals`, unique request IDs, UTC
freshness, deduplication, existing envelope parsing, and game-thread delivery.
The existing `test` and `pulse/gentle` behavior must continue to work.

The eventual Streamer.bot action maps bare `!palette` to `next` and a named
command such as `!palette ice` to `ice`. This plan delivers the Unreal handler
and payload contract. Streamer.bot palette command/action authoring is a
separate follow-up after local Unreal validation. Its proposed global/user
cooldowns remain 15/30 seconds.

### 5. Build once, integrate, and verify the pair

Cam saves Blueprint/level work and closes Unreal for the native build. Dex
rechecks the working diff, builds `RTCQTEditor Win64 Development`, and runs
the existing `RTCQT.` suite plus focused palette tests. Read the exported
report and require zero failed or unrun selected tests. Use separate
`Saved/Automation/Palette/` reports and palette log names so pulse evidence
is preserved. Implementation handoff supplies WSL/Bash commands.

After the successful build, create/verify the preset asset while the editor
is closed using an isolated commandlet session. Then Cam reopens Unreal,
compiles the affected manager/widget Blueprints if required, assigns the
preset asset, enables the component on the placed manager, and saves the
existing level. Recheck the pulse controller's references after the reload.

## Acceptance gates

| Gate | Required evidence |
| --- | --- |
| Palette state and timing | Named/cycle/reset behavior, exact endpoint, smooth interrupted transition, independent cooldown, invalid-input preservation, and transient session state. |
| Material persistence | Colors survive normal evaluation, geometry clear/rebuild, material recreation, track switches, and restarts; clearing restores actual authored defaults. Verify actor isolation and unchanged transforms/response custom data. |
| Receiver regression | Existing test and pulse cases pass; every allowed palette value, invalid values, duplicate/stale requests, and disconnected delivery are covered with a separate fixture. |
| Existing project regression | Native build and the complete current RTCQT automation suite pass, including existing D12 material and Music Lab behavior. |
| Local visual review | Cam sees four distinct smooth palettes and a correct reset, with music continuing through selection, pause/resume, switching, and restart. |
| Concurrent effects | Change palette during an active pulse: colors blend, scale returns to BaseScale, and the selected colors remain. Pulse cancellation and both independent cooldowns still behave correctly. |
| Restart persistence | After a saved setup and editor restart, controls/presets are configured, session colors start at authored defaults, and palette plus pulse work again. Cam confirms music and visuals. |

Receiver automation or mock delivery can establish the Unreal command path;
real Twitch delivery remains a separately named gate until the palette bot
action exists and has been exercised. Keep logs, screenshots, and a short
combined-effects recording under `planning/evidence/viewer-interactions/`.

## First handoff

Cam can continue the pulse walkthrough. Dex can first implement the preset
types, visualizer API, component, local controls, and tests in the files above.
The receiver edit, native build, asset setup, and level configuration wait for
a coordinated pause in the pulse work. Source progress is recorded above;
level and Blueprint asset integration still belongs to Cam.
