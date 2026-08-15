# RTCQT Offline Visualizer Architecture

## Decision

Rebuild RTCQT as a deterministic, non-real-time Unreal visualizer driven by
Audio Synesthesia `ConstantQNRT` analysis and Sequencer time.

The first release will not port the custom Constant-Q transform, PCM decoder,
live audio capture, sliding-window analysis, reflection-driven controls, or
runtime save-game configuration. Those systems remain historical reference
only unless a later visual comparison proves that one of them provides a
specific benefit that supported Unreal tools cannot reproduce.

## Product Goal

Given an approved audio track and a sequence time, the project produces the
same band values, mesh transforms, and material parameters every time. The
result can be scrubbed in Sequencer and rendered through Movie Render Queue
without depending on real-time audio callbacks or frame history.

The first observable finish line is:

> A 30-second track drives one instanced cube per frequency band, produces the
> same result at selected timestamps after seeking, and renders cleanly through
> Movie Render Queue.

## Data Flow

```text
SoundWave + ConstantQNRT asset
             |
             v
      authoritative song time
             |
             v
   queried frequency-band values
             |
             v
  deterministic response processor
             |
             v
 layout transforms + visual parameters
             |
             v
 Instanced Static Mesh Component
             |
             v
  Level Sequence -> Movie Render Queue
```

## Runtime Components

### `ARTCQTVisualizer`

One actor owns the complete visualization runtime.

Responsibilities:

- Reference the `SoundWave` and `ConstantQNRT` assets.
- Accept an authoritative song time in seconds.
- Query one value per configured frequency band at that time.
- Pass raw values through the response processor.
- Generate layout transforms for the current band count.
- Update instance transforms and per-instance visual data.
- Reset deterministically after asset, layout, settings, or time discontinuity
  changes.
- Fail visibly but safely when a required asset is missing.

It must not:

- Capture live audio.
- Decode PCM bytes.
- Use wall-clock or accumulated `DeltaTime` as song time.
- Spawn one actor per band.
- Save user settings through `USaveGame`.

The actor should expose a small Blueprint-callable surface so Sequencer or a
Blueprint coordinator can set time and request a refresh without duplicating
the analysis or response logic.

### `URTCQTResponseSettings`

A data asset stores artist-facing response controls:

- Noise floor.
- Response exponent.
- Input and output minimum/maximum.
- Per-band gain or focus weights.
- Optional high-frequency compensation.
- Optional deterministic temporal response parameters.
- Mesh scale limits.
- Emissive intensity limits.

All settings require safe editor ranges. Invalid values must be clamped or
rejected before processing.

Presets are ordinary Unreal assets. Runtime save-game persistence is not part
of the render workflow.

### `FRTCQTResponseProcessor`

A plain C++ type transforms raw analyzer values into normalized visual response
values. It should not derive from `UObject` and should not know about meshes,
materials, widgets, or audio playback.

Initial operations, in order:

1. Reject non-finite input and substitute zero.
2. Apply the noise floor.
3. Normalize to a documented range.
4. Apply per-band gain or focus weight.
5. Apply the response exponent.
6. Clamp to the output range.

The initial implementation should be stateless. This guarantees that output at
time `T` is independent of frame rate, playback direction, and seek history.

Attack/release smoothing is deferred until it has a seek-safe design. Valid
later options are:

- Querying and processing a fixed look-behind interval for every requested
  time.
- Baking smoothed band values into an asset before rendering.

Accumulating smoothing state once per rendered frame is not acceptable because
it makes scrubbing and isolated frame renders inconsistent.

### `FRTCQTLayout`

Plain deterministic layout functions create a transform for each band index.
The initial API should support:

- Line.
- Circle.
- Fibonacci sphere.

Spiral is deferred until its intended shape is specified. The old spiral code
offsets all three coordinates and does not describe a conventional spiral.

Each function accepts explicit inputs such as instance count, spacing, radius,
base scale, orientation, and origin. It must not read actor state or mutate the
world. Invalid counts and dimensions return an empty or safely clamped result.

### Rendering

Use `UInstancedStaticMeshComponent` or
`UHierarchicalInstancedStaticMeshComponent` instead of one `ASoundMesh` actor
per band.

Per-instance output should include:

- Transform or Z scale.
- Response value for material use.
- Optional normalized band index.
- Optional focus weight.

Prefer per-instance custom data for emissive and color control. Material
parameter names and ranges must be constants or documented settings rather
than repeated string literals.

## Authoritative Time Contract

The visualizer consumes an explicit song time; it does not own a free-running
clock.

For every update:

1. Clamp or reject time outside the analysis duration.
2. Query `ConstantQNRT` using that exact time.
3. Run stateless response processing.
4. Apply the resulting values to instances.

Calling the update twice with the same assets, settings, layout, and time must
produce equivalent output.

Seeking backward, jumping forward, restarting, or rendering a single isolated
frame must not retain visual state from a prior time. Audio playback can use
the same Sequencer timeline for preview, but audible playback is not the clock
source.

## Editor and Module Boundaries

The Stream MVP needs one runtime module. Runtime code may depend on supported
runtime Audio Synesthesia, Engine, Core, and rendering modules. It must not
depend on `AudioSynesthesiaEditor`, `PropertyEditor`, or other editor modules.

If custom editor tooling becomes useful later, it belongs in a separate editor
module guarded by editor-only build rules.

The exact Unreal Engine version and enabled plugin versions must be recorded
when the new project shell is created.

## Old-Code Disposition

| Existing area | Disposition | Reason |
| --- | --- | --- |
| `CoreDSP/` | Archive/reference | Replaced by supported `ConstantQNRT` analysis. |
| `AnalyzerComponents/RTCQTAnalyzer.*` | Retire | Owns custom analysis and reflection-heavy setup. |
| `AnalyzerComponents/Sampler.*` | Retire | PCM conversion is unnecessary and incorrect for several formats. |
| `AnalyzerComponents/SampleProcessor.*` | Retire | Real-time sample filtering is outside the offline MVP. |
| `AnalyzerComponents/SpectrumProcessor.*` | Rewrite selectively | Preserve useful response concepts as stateless plain C++. |
| `SpectrumManager.*` | Replace | Combines analysis, buffering, saving, UI, and broadcast concerns. |
| `MeshController/` layouts | Port selectively | Line, circle, and Fibonacci sphere are useful visual concepts. |
| `ASoundMesh` per-band actors | Retire | Instancing is simpler and more efficient. |
| Mesh reactor/function toggles | Retire | Direct typed response updates are easier to validate. |
| Widget property interfaces | Retire | Data assets and actor properties cover render-time tuning. |
| `SaveSettings/` | Retire for MVP | Unreal assets provide recoverable presets. |

Do not delete the old source until the rebuild reproduces the desired visual
language. Keep it outside the new runtime module so it cannot affect builds.

## Failure Behavior

The visualizer must handle these conditions without crashing:

- Missing `SoundWave`, analyzer, mesh, material, or response settings.
- Zero bands or a mismatch between configured and returned band counts.
- Negative, non-finite, or out-of-range analyzer values.
- A requested time outside the analyzed duration.
- A layout change that changes the instance count.

Failures should produce one actionable log message and a safe empty or baseline
visual state. Avoid per-frame warning spam.

## Verification Strategy

### Pure-code checks

- Noise-floor mapping at below, equal, and above threshold inputs.
- Exponent behavior for zero, midpoint, and maximum values.
- Clamping and handling of `NaN`/infinity.
- Per-band weight behavior and mismatched weight lengths.
- Layout counts, finite transforms, and expected symmetry/radius.
- Identical input produces identical output.

### Unreal integration checks

- A missing analyzer asset does not crash the actor.
- Instance count matches the analyzer band count.
- Three updates at the same timestamp produce matching values and transforms.
- Seeking away and back produces the original result.
- Scrubbing to known timestamps matches playback-time queries.
- Movie Render Queue can render a short range twice with matching checkpoints.

## Implementation Milestones

### Milestone 1: recoverable project shell

- Choose and record the Unreal Engine version.
- Create the C++ project and runtime module.
- Enable Audio Synesthesia and Movie Render Queue.
- Commit `.uproject`, target files, `Config/`, and new source structure.
- Confirm a clean Development Editor build and reopen.

### Milestone 2: static instanced layout

- Add `ARTCQTVisualizer` with one instanced mesh component.
- Implement line layout first.
- Create the exact configured number of instances.
- Validate missing mesh/material behavior.

### Milestone 3: deterministic analyzer query

- Import an approved 30-60 second WAV.
- Create its `ConstantQNRT` asset.
- Drive updates from an explicit song-time input.
- Record raw values at three reference timestamps.

### Milestone 4: response and material

- Add the stateless response processor and data asset.
- Drive bounded mesh scale and emissive custom data.
- Lock exposure and document material parameter ranges.
- Verify seek equivalence at the reference timestamps.

### Milestone 5: offline proof

- Create a Level Sequence matching the test range.
- Render a 30-second proof twice through Movie Render Queue.
- Compare start, middle, and end checkpoints.
- Mux the approved audio once and verify synchronization.

Circle and Fibonacci-sphere layouts can follow after the line-layout proof is
deterministic. Live audio, custom CQT, Pixel Streaming, and runtime controls do
not enter the plan until the offline proof is complete.

## First Workstation Session Checklist

- [ ] Record the installed Unreal Engine version.
- [ ] Create and reopen a clean C++ project.
- [ ] Enable Audio Synesthesia and Movie Render Queue.
- [ ] Confirm the runtime module builds without editor dependencies.
- [ ] Import one approved 30-60 second WAV.
- [ ] Create a 32- or 48-band `ConstantQNRT` asset.
- [ ] Query and record values at three timestamps.
- [ ] Create a line of static mesh instances matching the band count.
- [ ] Save the build log and screenshots as milestone evidence.

## Explicitly Deferred

- Custom real-time Constant-Q analysis.
- PCM decoding and live audio capture.
- Attack/release behavior based on accumulated frame state.
- Pixel Streaming or multiplayer delivery.
- Runtime settings UI and save games.
- Editor customization modules.
- Final artistic layout and music selection.
