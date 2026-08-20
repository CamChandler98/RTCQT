# RTCQT Unreal Runtime API and Source Contract

## Status

This document completes delivery card D04. It turns the offline architecture,
the D02 response contract, and the D03 layout contract into implementation-ready
Unreal interfaces for the new `RTCQT` runtime module.

The headers in `planning/header-drafts/` are reviewed design evidence. D06 will
copy them into a clean Unreal project, replace `RTCQT_API` only if the created
module uses a different name, and verify them against the exact installed engine
version. That compile check may correct engine include details, but it must not
change the ownership, evaluation, response, layout, or failure contracts here
without a deliberate architecture update.

## Decisions

- The rebuild uses one runtime module named `RTCQT`.
- `ARTCQTVisualizer` is the only runtime actor and owns one
  `UInstancedStaticMeshComponent`.
- `UConstantQNRT` is the analysis asset and the source of the associated
  `USoundWave` and analysis duration. The actor does not store a second sound
  reference that could disagree with the analyzer asset.
- `URTCQTResponseSettings` is an artist-authored `UDataAsset`.
- `FRTCQTResponseProcessor` and `FRTCQTLayout` are stateless plain C++ types.
- `EvaluateAtTime(TimeSeconds)` is the only authoritative time-driven update.
- The visualizer does not tick to advance song time and does not read audio
  playback position, wall-clock time, or accumulated `DeltaTime`.
- Layout output and instance transforms are local to the instanced-mesh
  component.
- The Stream MVP uses line layout. Circle and Fibonacci sphere share the API
  but remain D19 implementation work.
- The old `SynRTCQT` source stays as reference and is not a dependency of the
  new runtime module.

## Non-goals

D04 does not implement or restore:

- Custom Constant-Q DSP or PCM decoding.
- Live audio capture.
- Stateful attack/release or frame interpolation.
- One actor or dynamic material instance per frequency band.
- Runtime settings widgets or save games.
- Editor customization modules.
- Sequencer tracks, Movie Render Queue setup, or OBS integration.

## Ownership and asset references

| Owner | Member | Reference type | Responsibility |
| --- | --- | --- | --- |
| `ARTCQTVisualizer` | `InstanceComponent` | Default subobject, strong `TObjectPtr` | Owns every rendered band instance and its per-instance custom data. |
| `ARTCQTVisualizer` | `ConstantQNRT` | Editor-assigned strong `TObjectPtr` | Supplies normalized Constant-Q values, its analyzed sound, and analysis duration. |
| `ARTCQTVisualizer` | `ResponseSettings` | Editor-assigned strong `TObjectPtr` | Supplies D02 parameters and bounded scale/emissive ranges. |
| `ARTCQTVisualizer` | `InstanceMesh` | Editor-assigned strong `TObjectPtr` | Supplies the reusable geometry for every band. |
| `ARTCQTVisualizer` | `InstanceMaterial` | Optional editor-assigned strong `TObjectPtr` | Overrides the mesh material for the Stream MVP; the mesh default is the safe fallback. |
| `ARTCQTVisualizer` | `LayoutSettings` | Reflected value | Supplies the explicit D03 layout inputs other than count. |
| `ARTCQTVisualizer` | `ExpectedBandCount` | Reflected value | Defines the static D08 instance count and the exact count D10 expects from the analyzer. |
| `ARTCQTVisualizer` | `AnalyzerChannel` | Reflected value | Selects the ConstantQNRT channel; the initial mono/downmixed asset uses channel `0`. |

The actor does not own the analyzer's `USoundWave`. `UConstantQNRT` inherits the
`Sound` and `DurationInSeconds` properties from `UAudioAnalyzerNRT`. A missing
analyzed sound is therefore reported through the analyzer reference rather than
through a duplicate property.

## Shared reflected types

### `ERTCQTLayoutMode`

Selects `Line`, `Circle`, or `FibonacciSphere`. `Line` is the initial default.
The enum is stable across D08 and D19 so adding the later layouts does not
change actor ownership or response processing.

### `FRTCQTScalarRange`

Stores a minimum and maximum visual output. The response processor sanitizes
both bounds using D02 rules and swaps reversed finite bounds. Equal bounds
produce a constant output.

### `FRTCQTLayoutSettings`

Stores `Mode`, `Origin`, `BaseScale`, `Spacing`, and `Radius`. Instance count is
not stored in the layout settings because it is a band-pipeline invariant owned
by the visualizer.

### `FRTCQTEvaluationResult`

Returns `bSucceeded`, a typed `FailureReason`, the requested time, and the
observed band count. Blueprint callers can branch on the enum without parsing a
log message.

## Response settings API

`URTCQTResponseSettings` owns:

- `NoiseFloor`, initially `0.1`.
- `InputCeiling`, initially `1.0`.
- `BandWeights`, with missing entries interpreted as `1.0`.
- `ResponseExponent`, initially `2.0`.
- `MeshZScaleRange`, the absolute local Z scale applied by D11.
- `EmissiveRange`, the bounded material range consumed by D12.

`MakeProcessorParameters()` copies only the D02 scalar parameters and weights
into an ordinary `FRTCQTResponseParameters`. The plain response processor never
accepts or retains a `UObject`.

## Response processor API

`FRTCQTResponseProcessor` has three public operations:

```cpp
static float ProcessBand(
    float Input,
    const FRTCQTResponseParameters& Parameters,
    int32 BandIndex);

static void ProcessBands(
    TConstArrayView<float> Inputs,
    const FRTCQTResponseParameters& Parameters,
    TArray<float>& OutResponses);

static float MapResponse(
    float Response,
    float OutputMin,
    float OutputMax);
```

`ProcessBand` and `ProcessBands` implement D02 exactly. They store no previous
input or output. `ProcessBands` replaces `OutResponses`; it does not append.
`MapResponse` performs the D02 output-bound sanitization and ordering.

## Layout API

`FRTCQTLayout::Build(Settings, Count, OutTransforms)` is the single public
layout entry point. It replaces `OutTransforms` and returns one
`FRTCQTLayoutTransform` per band for every valid positive count.

Each layout result preserves the D03 values directly:

- `Translation`.
- `Forward`, the local positive-X direction.
- `Up`, the local positive-Z direction.
- `Scale`.

`FRTCQTLayoutTransform::ToTransform()` converts those axes with
`FRotationMatrix::MakeFromXZ(Forward, Up)` when Unreal instance transforms are
needed. Keeping the axes in the result lets automation tests compare the D03
golden vectors without relying on quaternion component signs.

## Visualizer actor API

The public actor surface intentionally has only three operations:

```cpp
FRTCQTEvaluationResult EvaluateAtTime(float TimeSeconds);
bool RebuildInstances();
void ClearVisualization();
```

### `EvaluateAtTime`

Evaluates one isolated visual frame at the exact caller-supplied song time. It
does not advance or remember a clock. Repeated calls at the same time with the
same assets, settings, layout, and band count must produce equivalent outputs.

The final D10-D12 implementation order is:

1. Reject a non-finite time.
2. Require the analyzer, its analyzed sound, response settings, mesh, and a
   positive expected band count.
3. Require `0 <= TimeSeconds <= ConstantQNRT->DurationInSeconds`.
4. Call `GetNormalizedChannelConstantQAtTime` with exactly `TimeSeconds` and
   `AnalyzerChannel`.
5. Require exactly `ExpectedBandCount` returned values.
6. Rebuild instances only if the count or layout inputs changed.
7. Convert settings to `FRTCQTResponseParameters` and process all bands.
8. Begin with each D03 base transform, preserve its X and Y scale, and replace
   local Z scale with the bounded `MeshZScaleRange` mapping.
9. Update existing instance transforms in local space.
10. Write per-instance custom data and mark render state dirty once after the
    batch.
11. Return a successful `FRTCQTEvaluationResult` and clear the prior failure
    state.

Steps 7-10 are introduced incrementally by D09-D12, but their final ownership
and order are fixed by D04.

### `RebuildInstances`

Builds the configured D03 transforms and recreates instances only when layout
inputs or `ExpectedBandCount` changed. It requires a mesh and positive band
count but does not require an analyzer or response settings. This lets D08
prove the static line layout before D07 and D10 are connected.

Each new instance starts with its D03 base transform, a response custom-data
value of `0`, and a normalized band-index custom-data value. A single band uses
normalized index `0` to avoid division by zero.

### `ClearVisualization`

Removes all instances and clears cached layout identity. It does not modify
assigned assets or artist settings.

## Authoritative time and seeking

Sequencer or a Blueprint coordinator must call `EvaluateAtTime` with time
derived from the sequence evaluation position. Audio playback may share the
same Level Sequence, but it is not queried as the visual clock.

The actor stores only the last failure reason and cached layout identity. It
does not store previous analyzer values or responses. Layout caching may decide
whether instance topology needs rebuilding; it must never affect the numerical
result for a requested time.

The initial time-boundary policy is rejection rather than silent clamping:

- Negative and non-finite times fail.
- Times greater than the analyzer duration fail.
- `0` and the exact analyzer duration are accepted.

This makes sequence-range mistakes visible during D10 and D13.

## Instance and material contract

The actor uses one `UInstancedStaticMeshComponent`; it never spawns per-band
actors.

Per-instance custom-data slots are stable constants:

| Slot | Name | Range | Consumer |
| ---: | --- | --- | --- |
| `0` | Response | `[0, 1]` | D12 scale-correlated emissive/color material logic. |
| `1` | Normalized band index | `[0, 1]` | D12 deterministic color variation. |

`NumCustomDataFloats` is therefore `2` before instances are created. D12 may
use one material or one shared dynamic material instance, but it must not create
one material instance per band.

A missing material is non-fatal: the component uses the mesh's default
material, writes custom data normally, and logs one warning when entering that
condition. A missing mesh is fatal because no safe visible instance topology
can be built.

## Failure and logging contract

The runtime module declares `LogRTCQT`. Failures are logged when the actor
enters a different failure reason, not every time the same bad frame is
evaluated. A successful evaluation clears the remembered failure so a later
recurrence can be reported once again.

| Condition | Result | Visual state |
| --- | --- | --- |
| Missing analyzer or analyzed sound | Failure | Clear instances. |
| Missing response settings during evaluation | Failure | Clear instances. |
| Missing mesh | Failure | Clear instances. |
| Missing optional material | Warning, evaluation may succeed | Use mesh default material. |
| Non-positive expected band count | Failure | Clear instances. |
| Non-finite or out-of-range time | Failure | Preserve no stale response; clear instances. |
| Empty analyzer result | Failure | Clear instances. |
| Returned/configured band mismatch | Failure | Clear instances and report both counts. |
| Invalid layout output count or non-finite transform | Failure | Clear instances. |
| Instance update failure | Failure | Clear instances. |
| Non-finite analyzer sample | Not an actor failure | D02 sanitizes it to zero. |
| Finite analyzer sample outside `[0, 1]` | Not an actor failure | D02 normalization saturates it. |

Failures never leave a response from another timestamp visible.

## Runtime source tree

```text
Source/RTCQT/
|-- RTCQT.Build.cs
|-- Public/
|   |-- RTCQTTypes.h
|   |-- RTCQTVisualizer.h
|   |-- RTCQTResponseSettings.h
|   |-- RTCQTResponseProcessor.h
|   `-- RTCQTLayout.h
`-- Private/
    |-- RTCQTModule.cpp
    |-- RTCQTVisualizer.cpp
    |-- RTCQTResponseSettings.cpp
    |-- RTCQTResponseProcessor.cpp
    |-- RTCQTLayout.cpp
    `-- Tests/
        |-- RTCQTResponseTests.cpp
        `-- RTCQTLayoutTests.cpp
```

Production source is created by D06 and later implementation cards. The old
root-level `SynRTCQT.Build.cs`, `CoreDSP/`, `AnalyzerComponents/`,
`MeshController/`, `Widget/`, and `SaveSettings/` directories are not copied
into this module.

## Module dependencies

Initial build rules:

| Visibility | Module | Reason |
| --- | --- | --- |
| Public | `Core` | Core types, arrays, views, math, and logging. |
| Public | `CoreUObject` | Reflected enums, structs, data asset, and actor declarations. |
| Public | `Engine` | Actor, data asset, mesh, material, and instanced-mesh types. |
| Private | `AudioSynesthesia` | `UConstantQNRT` query implementation and asset access. |

`AudioSynesthesiaEditor`, `PropertyEditor`, `UnrealEd`, `Slate`, `UMG`,
`SignalProcessing`, `AudioMixer`, and the legacy `AudioAnalyzer` integrations
are not runtime dependencies for this design. If the installed engine requires
a direct `AudioAnalyzer` dependency for inherited NRT members, D06 may add it
as a private runtime dependency and record why.

Movie Render Queue is a project plugin used by D13-D14, not a dependency of
the `RTCQT` visualizer module.

## Workstation verification gate

D06 must verify these engine-facing assumptions against the installed engine:

1. Record the exact Unreal version and Audio Synesthesia plugin version/state.
2. Confirm `#include "ConstantQNRT.h"` and the `AudioSynesthesia` runtime module.
3. Confirm the normalized query signature accepts time, channel, and an output
   `TArray<float>`.
4. Confirm `Sound` and `DurationInSeconds` remain available on the NRT asset.
5. Confirm the current instanced-mesh batch update and custom-data calls.
6. Compile the header skeleton with no editor modules.

If only an include path or function overload differs, D06 updates the draft to
the installed API. A change to time ownership, result count, response order,
layout math, or failure behavior requires an explicit contract revision.

## Acceptance mapping

| D04 acceptance criterion | Evidence |
| --- | --- |
| Every public property and method has one documented responsibility. | The class sections above and doc comments in every header draft. |
| Runtime code has no editor-module dependency. | Module dependency table and `planning/header-drafts/RTCQT.Build.cs`. |
| API supports isolated frame evaluation and backward seeks. | `EvaluateAtTime`, authoritative-time rules, and absence of response history. |
| Implementation can proceed without inventing new architecture. | Ownership table, fixed evaluation order, failure table, source tree, and D08-D12 handoff boundaries. |

## D08-D12 implementation handoff

- D08 implements `FRTCQTLayout::Build` for line mode plus
  `RebuildInstances`; circle and sphere may return an explicit unsupported
  failure until D19.
- D09 implements `FRTCQTResponseProcessor` and ports D02 vectors into Unreal
  automation tests.
- D10 implements validation, the timestamped ConstantQNRT query, and band-count
  checks inside `EvaluateAtTime`.
- D11 maps the shared response to local Z scale without changing X/Y or
  rebuilding topology.
- D12 consumes custom-data slots `0` and `1`, configures bounded emissive/color,
  and locks exposure.

## Engine references checked for D04

- [Epic: Audio Synesthesia overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-synesthesia-in-unreal-engine)
- [Epic: `UConstantQNRT` API, Unreal Engine 5.8](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/AudioSynesthesia/UConstantQNRT?lang=en-US)
- [Epic: `UAudioAnalyzerNRT` API, Unreal Engine 5.8](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/AudioAnalyzer/UAudioAnalyzerNRT?lang=en-US)
- [Epic: `UInstancedStaticMeshComponent` API, Unreal Engine 5.8](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UInstancedStaticMeshComponent?lang=en-US)
