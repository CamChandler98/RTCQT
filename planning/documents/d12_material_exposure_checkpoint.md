# D12 Material and Exposure Checkpoints

Date: 2026-10-02, America/New_York.
Branch: `codex/d12-material-exposure`.
Base: D11 complete at `e574d06`.

Status: **In progress**. The Development Editor build and all 22 RTCQT
automation groups pass. Material authoring, rendered readability, fixed
exposure, and fresh saved-scene verification remain pending.

## Implemented contract

One transient `UMaterialInstanceDynamic` belongs to each visualizer actor.
`RebuildInstances`, including each successful evaluation, reuses that material
and refreshes its scalar bounds. Changing the parent replaces the shared
material. Duplicated worlds reconstruct their own material. No material is
allocated per band or per timestamp evaluation.

The material receives `EmissiveMin` and `EmissiveMax` through the documented
constants in `RTCQTMaterialParameters.h`. Non-finite limits become zero,
finite limits clamp to `[0, 100]`, and the bounds are ordered. Defaults are
`[0, 10]`. D11 signed scale semantics remain unchanged. Missing optional
material retains the mesh default and the existing warning behavior.

Custom-data slot `0` remains the D02 response in `[0, 1]`; slot `1` remains
normalized band index. Shader emissive is `bandTint * lerp(min, max, response)`.
Band tint blends the material's LowBandColor and HighBandColor parameters
using slot `1`. Base Color uses `bandTint * lerp(0.25, 1, response)` so quiet
cubes keep a lit surface. The initial material is opaque Default Lit with
metallic `0` and roughness `0.65`.

The public Blueprint methods and runtime dependencies remain unchanged.
The actor still has no tick, clock, smoothing, or previous response state.

## Implementation validation

Three D12 groups pass with zero warnings/errors:

- `SharedSettingsAndBounds`: independent response 0/0.5/1 yields emissive
  0/5/10 and correlated heights; settings replacement, reversed/negative/
  constant/non-finite/extreme limits refresh safely without changing height.
- `SeekingReplacementAndFallback`: reordered queries restore exact custom
  data, reuse the material, handle a distinct replacement parent, fall back
  to the mesh default, and recover with no instance add/remove/clear events.
- `ActorOwnership`: separate actors have separate shared materials and
  changes to one actor's settings do not affect the other.

All 19 existing D08-D11 groups also pass, including the actual ignored D07
analyzer queries and transform captures. The first material-replacement test
used an engine fallback that was the same parent as the cube; its fixture
was corrected to supply a distinct material instance. The final suite passes
all 22 groups.

Retained implementation evidence:

- `planning/evidence/d12/checkpoint1-build-log.txt`
- `planning/evidence/d12/checkpoint1-automation.json`
- `planning/evidence/d12/checkpoint1-automation-log.txt`
- `planning/evidence/d12/checkpoint1-verification.json`

Follow the approved [D12 plan](d12_material_exposure_plan.md) for the material,
exposure, reference captures, and saved-scene checkpoint. D12 must remain
`Now` until those gates pass.
