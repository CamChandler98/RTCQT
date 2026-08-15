# Rebuilding RTCQT in Modern Unreal

This guide is for rebuilding the useful visualizer idea from the old
`SynRTCQT` module in a current Unreal Engine project. It treats the old code as
a reference implementation, not as the foundation that must compile before any
visualizer work can continue.

The important surviving idea is:

```text
audio spectrum bands -> smoothing and shaping -> per-band visual response
```

The old implementation did that with custom C++ Constant-Q analysis,
`ASpectrumManager`, `URTCQTAnalyzer`, `USampleProcessor`,
`USpectrumProcessor`, `AMeshController`, and `ASoundMesh`. In a recent Unreal
version, rebuild that pipeline in smaller layers and only port the custom DSP
after the visual side is working.

## Why not port everything first?

The current UE 5.8 build log shows several categories of engine-version drift:

- `UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_1` is referenced directly, and newer
  Unreal build settings do not define it in this module configuration.
- Copied or customized DSP types in `CoreDSP/` use `SIGNALPROCESSING_API`,
  causing inconsistent DLL linkage when compiled inside the game module.
- `MeshReactor.h` is missing `#pragma once`, so UHT detects the generated
  header being included twice.
- `EFFTWindowType` and `EAudioSpectrumType` are only forward declared by the
  included Audio Synesthesia header in this engine version, so code that uses
  enum values needs the correct defining header or a different settings API.
- `TObjectPtr<FBoolProperty>` is invalid because `FBoolProperty` is reflection
  metadata, not a `UObject`.
- `AudioSynesthesiaEditor` is listed as a public dependency in
  `SynRTCQT.Build.cs`, which is not a good runtime dependency boundary.

Those are fixable, but they are not the shortest path back to a working music
visualizer. The clean path is to recreate the visible behavior with supported
engine systems first, then port old C++ pieces only where they still add value.

## Target architecture

Build the new project around four small layers:

1. **Analysis source**
   Produces one float value per frequency band for a given playback time.

2. **Spectrum model**
   Normalizes, smooths, clamps, scales, and optionally groups those band values.

3. **Visualizer actor**
   Owns the meshes, materials, layout, and camera-facing behavior.

4. **Render or playback driver**
   Keeps audio time, spectrum time, and rendered frames in sync.

The first implementation should use Unreal's built-in Audio Synesthesia
`ConstantQNRT` analyzer as the analysis source. It is precomputed, stable, and
much easier to debug than real-time custom DSP.

## Milestone 1: new project shell

1. Create a new C++ project in the current Unreal version.
2. Enable these plugins:
   - **Audio Synesthesia**
   - **Movie Render Queue**
3. Add a short licensed WAV or test tone to `Content/Audio/`.
4. Create a `ConstantQNRT` asset for that `SoundWave`.
5. Commit or otherwise snapshot this clean baseline before importing old code.

Do not copy `CoreDSP/` into the new project yet. Keep the old repository open
as a reference.

## Milestone 2: first deterministic visualizer

Create a new actor, for example `ANRTSpectrumVisualizer`, that owns:

- a `UConstantQNRT*` or Blueprint-assigned ConstantQNRT asset,
- the associated `USoundWave*`,
- an array of spawned mesh components or instanced static mesh entries,
- one dynamic material instance per band or one material using per-instance
  custom data,
- smoothing settings such as attack, release, min scale, max scale, and
  brightness multiplier.

At runtime:

1. Start audio playback.
2. Store the exact audio start time.
3. On Tick, compute `SongTimeSeconds`.
4. Query ConstantQNRT for band values at `SongTimeSeconds`.
5. Smooth the values.
6. Apply each band to mesh scale, emissive intensity, color, rotation, or
   position.

The first scene can be plain cubes in a line. Make it boring on purpose. The
checkpoint is sync and repeatability, not the final look.

## Milestone 3: recreate the old visual language

The old `AMeshController` and `ASoundMesh` code is still useful as a design
reference. Recreate its features in the new visualizer in this order:

1. Line layout.
2. Circle layout.
3. Spiral layout.
4. Sphere or Fibonacci sphere layout.
5. Per-band Z scale.
6. Per-band emissive brightness.
7. Per-band HSV color shifts.
8. Focus bands or focus ranges.

Prefer `UInstancedStaticMeshComponent` or `UHierarchicalInstancedStaticMeshComponent`
for many repeated meshes. The old version spawned separate `ASoundMesh` actors,
which is simple, but a current rebuild can be much more efficient with
instances.

## Milestone 4: rebuild spectrum shaping

Once the visualizer reacts correctly to ConstantQNRT bands, port the safer parts
of the old processing layer:

- smoothing across adjacent bands,
- attack/release smoothing over time,
- noise floor,
- exponential scaling,
- min/max normalization,
- focus curves,
- color and brightness mapping,
- saved settings.

Good source references:

- `SpectrumManager.cpp`: `SmoothSpectrum`, `SmoothBoundary`, and event flow.
- `AnalyzerComponents/SpectrumProcessor.cpp`: spectrum shaping ideas.
- `AnalyzerComponents/SampleProcessor.cpp`: sample-domain shaping ideas.
- `MeshController/MeshController.cpp`: line/circle/sphere/spiral layout ideas.
- `MeshController/Mesh/SoundMesh.cpp`: material parameter names and response
  methods.

Do not preserve old class boundaries unless they still feel useful. A new
`USpectrumSmoother` or `USpectrumMapper` can be cleaner than reviving every
old settings object.

## Milestone 5: render a finished piece

For a stream-ready output, make rendering deterministic before making anything
networked or interactive.

1. Put the visualizer and camera in a Level Sequence.
2. Match the sequence duration to the audio duration.
3. Render with Movie Render Queue.
4. Combine the original WAV with the rendered frames or video.
5. Check sync at the beginning, middle, and end.
6. Test the finished file in OBS as a Media Source.

This gets the visualizer onto a stream without requiring Unreal to run live
during the show.

## Later: real-time or custom CQT

Only return to the old custom CQT path after the NRT visualizer is satisfying.
At that point, make a separate module or plugin for analysis code instead of
mixing copied engine DSP directly into the game module.

Recommended porting approach:

1. Create a small runtime plugin named something like `RTCQTAnalysis`.
2. Put analysis-only code there; keep visualization in the game module.
3. Remove `SIGNALPROCESSING_API` from copied/custom structs and functions that
   belong to your module, or replace it with your own module API macro.
4. Replace direct references to old include-order macros with guarded checks:

   ```cpp
   #if defined(UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_1) && UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_1
   #include "CoreMinimal.h"
   #endif
   ```

5. Add `#pragma once` to every Unreal header before any includes.
6. Use raw `FProperty*` / `FBoolProperty*` for reflection metadata, not
   `TObjectPtr`.
7. Keep editor modules such as `AudioSynesthesiaEditor` and `PropertyEditor`
   behind `if (Target.bBuildEditor)` in `.Build.cs`.
8. Add one automated test or debug commandlet that feeds a known sine wave or
   sweep into the analyzer and verifies that the expected bands respond.

## Suggested current rebuild checklist

- [ ] Create a clean current-UE C++ project.
- [ ] Enable Audio Synesthesia and Movie Render Queue.
- [ ] Import one short test WAV.
- [ ] Create a ConstantQNRT asset.
- [ ] Build a cube-line visualizer from ConstantQNRT values.
- [ ] Add smoothing and scaling controls.
- [ ] Add circle or spiral layout.
- [ ] Add emissive material response.
- [ ] Render a synced test through Movie Render Queue.
- [ ] Only then decide whether the old custom CQT is worth porting.

## If you still want to make the old module compile

Use this as triage order for the UE 5.8 errors:

1. Add `#pragma once` to `MeshController/Mesh/MeshComponent/MeshReactor.h`.
2. Guard the include-order macro check in `CoreDSP/ConstantQ.h`.
3. Remove or replace `SIGNALPROCESSING_API` on custom copied DSP declarations
   in `CoreDSP/ConstantQ.h` and `CoreDSP/ConstantQAnalyzer.h`.
4. Include the header that defines `EFFTWindowType` and `EAudioSpectrumType`,
   or stop deriving custom runtime settings from `UConstantQNRTSettings`.
5. Change `TObjectPtr<FBoolProperty>` to `FBoolProperty*`.
6. Move `AudioSynesthesiaEditor` out of public runtime dependencies.
7. Rebuild after each fix and address the next first compiler error.

That path may be useful for salvaging code, but it should not block rebuilding
the visualizer experience.
