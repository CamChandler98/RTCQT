# D12 Material and Exposure Checkpoints

Date: 2026-10-02, America/New_York.
Branch: `codex/d12-material-exposure`.
Base: D11 complete at `e574d06`.

Status: **Done**. Implementation checkpoint: `460a429`. The Development
Editor build and all 22 RTCQT automation groups pass. The material compiles,
the saved scene passes a fresh Play check, and all six final captures have
been reviewed for quiet visibility, peak color, and repeated-frame appearance.

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

## Material and saved-scene verification

The tracked `/Game/Materials/M_RTCQT_Bands` has a clean 17-node graph and
Instanced Static Mesh usage enabled. Compilation returns no errors. The
native authoring helper is reproducible: it only rebuilds its D12-tagged
material and deletes old nodes from a snapshot of the expression list.

Low-index tint is linear RGB `(0.01, 0.65, 1.0)`; high-index tint is
`(0.85, 0.01, 0.65)`. Emissive Range is `[0, 10]`, Response Exponent is `2`,
and the D11 response and scale settings remain unchanged.

The existing private `L_D10D11ResponseTest` level assigns the material to the
existing visualizer. One unbound `D12 - Fixed Exposure` volume has priority
100 and blend weight 1. Min EV100 and Max EV100 both equal **2**; exposure
compensation is 0, its curve is unset, and bloom intensity is 0.15. The
camera's exposure override flags are disabled and its other post-process
settings remain unchanged. Original private level/settings backups are in
ignored `Saved/Automation/D12/before-setup`.

A fresh rendered editor session loads that saved configuration, enters Play,
and evaluates `16.75 -> 0.75 -> 2.00 -> 33.25 -> 16.75`. There is one
visualizer, one instanced component, 32 instances, and one active shared
material. Every evaluation succeeds. All queries reuse the same shared
material; returning to 16.75 restores exact custom data and transforms.
Play and editor instances each own their own material and produce matching
responses and transforms.

After ending Play, the helper renders those five exact frames from the
existing camera, then renders a controlled response-0/0.5/1 endpoint fixture
with emissive 0/5/10. The endpoint still is explicitly synthetic, not song
data. The helper does not save any asset or level during capture. Screenshot
freshness is checked so an earlier capture cannot satisfy the gate.

All six final 1920x1080 stills were inspected. The quiet 0.75 frame retains
visible, low-height colored surfaces. Moderate and strong frames show the
height/color relationship; the strongest sampled cyan band and the magenta
endpoint retain distinguishable color. The repeated strong still has the
same layout and appearance. Review at EV100 0 had more whitening, so the
selected fixed value is 2.

Equal exposure limits disable global eye adaptation. Scene framing and floor
brightness remain stable across the final stills; changing emissive can
still affect Lumen lighting. This checkpoint proves deterministic response,
transform, and material inputs, not pixel-identical temporal rendering.
Sequence/MRQ repeatability remains D14's gate.

The independent validator compares 160 band rows against D11's retained
capture using absolute/relative tolerance `1e-6`. Maximum response difference
is `4.851073986955612e-10`; maximum scale difference is
`4.928588648311916e-09`. Exact same-time and Play/editor input comparisons
also pass. All five tested source-file fingerprints remain unchanged since
checkpoint 1, so no additional broad test run was needed for the asset,
helper, and documentation changes.

The final authoring and capture sessions exit successfully and have no
errors, ensures, or fatals. Both have the engine startup warning about
`r.MotionVectorSimulation` being used on the render thread; the warning
precedes the script and is retained in the focused log. All 22 automation
groups themselves report zero warnings and errors.

Retained visual evidence under `planning/evidence/d12`:

- `checkpoint2-material-scene-setup.json`
- `checkpoint2-fresh-frames.json`
- `checkpoint2-capture-validation.json`
- `checkpoint2-native-log.txt`
- `checkpoint2-verification.json`
- `frame-quiet.png`, `frame-moderate.png`, `frame-strong-first.png`
- `frame-bright.png`, `frame-strong-repeat.png`, `frame-controlled-endpoints.png`

The original material is tracked with Git LFS. Licensed audio, analyzer,
response-settings asset, and test level remain ignored. The material README
and [D12 plan](d12_material_exposure_plan.md) document scene reproduction.

D12 is complete. D13 is ready for sequence-time evaluation, synchronized
audio, and the camera/output scene; its delivery card remains `Next`.
