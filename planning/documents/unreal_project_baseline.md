# D06 Unreal Project Baseline

## Status

D06 is complete. Every project-shell acceptance gate passed on UE 5.8.

## Workstation

- Date: 2026-08-20
- Project: `RTCQT`
- Project location: `C:\Users\camch\source\RTCQT`
- Unreal Engine: 5.8.0, changelist 55116800
- Engine association: `5.8`
- Visual Studio: Community 2022 17.14.37
- Compiler selected by UnrealBuildTool: Visual Studio toolchain 14.44.35228
  from MSVC installation 14.44.35207
- Windows SDK selected by UnrealBuildTool: 10.0.22621.0
- Target: `RTCQTEditor Win64 Development`

## Project Plugins

The project descriptor explicitly enables:

- `AudioSynesthesia`
- `MovieRenderPipeline`

The headless editor log confirms both plugins mounted. It also confirms the
runtime `AudioSynesthesia` module, the Movie Render Pipeline modules, and the
project `RTCQT` module loaded successfully.

Editor modules supplied internally by those engine plugins may load in the
editor. The project runtime module does not depend on their editor modules.

## Runtime Module Boundary

`Source/RTCQT/RTCQT.Build.cs` contains only:

- Public: `Core`, `CoreUObject`, `Engine`
- Private: `AudioSynesthesia`

The legacy root-level `SynRTCQT` source remains reference material and is
outside the new `Source/RTCQT` module.

The D06 private implementations are compile-safe skeletons. D08 implements
layout and instance topology, D09 implements response behavior, and D10-D12
complete analyzer evaluation and visual output.

## Build Evidence

Command:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  RTCQTEditor Win64 Development `
  'C:\Users\camch\source\RTCQT\RTCQT.uproject' `
  -WaitMutex -NoHotReloadFromIDE
```

Result:

- UnrealHeaderTool and all ten build actions completed.
- `UnrealEditor-RTCQT.dll` and its import library linked.
- Result: Succeeded.
- Total execution time: 20.01 seconds.

## Editor-Load Evidence

A headless `UnrealEditor-Cmd` startup recorded:

- Win64 SDK validation: valid.
- `UnrealEditor-RTCQT.dll` loaded.
- Audio Synesthesia mounted and its runtime modules loaded.
- Movie Render Pipeline mounted and its modules loaded.
- Engine initialization completed.
- Open World template map check: zero errors and zero warnings.
- The `Quit` command was accepted after initialization.

The command remained alive during asynchronous shutdown, so it was interrupted
after all startup and load evidence had been written. The next gate is an
ordinary interactive open, close, and reopen.

## Interactive Editor Verification

After the canonical source change, the project:

- Opened normally in Unreal Editor 5.8.
- Showed Audio Synesthesia and Movie Render Pipeline enabled.
- Closed without a project or module error.
- Opened normally a second time.

## Fresh Clone Verification

The committed candidate at `0801436` was cloned to a separate NTFS directory.

- No generated build, cache, or saved directories were present before build.
- UnrealHeaderTool generated eight files from tracked source.
- All twelve Development Editor build actions completed successfully.
- The clean clone loaded `UnrealEditor-RTCQT.dll` and both required plugins.
- Engine initialization and the template map check completed with zero errors
  and zero warnings.

## Storage Policy

- Track `.uproject`, `Config/`, `Source/`, and authored project
  documentation.
- Ignore `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`,
  IDE state, generated solutions, and generated code-workspace files.
- Keep the licensed WAV, imported SoundWave, and audio-bound analyzer assets
  under `Content/Audio/PrivateTest/`, which is ignored.
- Keep recovery provenance, hashes, extraction recipes, and analyzer settings
  in tracked text documents.
- Git LFS is not installed on this workstation. Install and configure it before
  committing other authored `.uasset` or `.umap` files.
- Do not commit the private audio merely because a later Git LFS policy exists.

## D06 Result

D06 meets its definition of done. D07 and D08 are now the two current delivery
cards under the work-in-progress limit.
